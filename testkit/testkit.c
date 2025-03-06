#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include "testkit.h"

static int ntests;
static struct tk_testcase tests[TK_MAX_TESTS];

/**
 * Add a test case to the test suite. Handles both system tests (calling
 * main with command-line arguments) and unit tests. This is the only
 * externally visible function in TestKit.
 */
void tk_add_test(struct tk_testcase t) {
    // Only add the test case when TestKit is enabled.
    if (!getenv(TK_RUN) && !getenv(TK_VERBOSE)) {
        return;
    }

    if (t.argv) {
        // This is a system test that calls main().
        tk_assert(t.stest, "Only system tests can have argv");

        // Test cases specify args via in-place arrays like:
        //   (char *[]){"first argument", "second argument"})
        // whose space is stack-allocated. Allocate space and copy.

        // Make space for argv[0] and trailing NULL.
        int argc = t.argc + 1;
        const char **argv = malloc(sizeof(char *) * (argc + 1));
        argv[argc] = NULL;

        // Hack: bash and zsh will put executable path in "_".
        argv[0] = getenv("_");
        tk_assert(argv[0] != NULL,
                  "TestKit requires shell put executable in environ; "
                  "try run with bash");

        for (int i = 1; i < argc; i++) {
            // String literals are compile-time constants; we are safe to
            // do only a shallow copy.
            argv[i] = t.argv[i - 1];
        }

        t.argc = argc;
        t.argv = argv;
    }

    tk_assert(ntests < TK_MAX_TESTS,
              "TestKit supports up to %d test cases", TK_MAX_TESTS);
    tests[ntests++] = t;
}

// ------------------------------------------------------------------------
// Below are testkit internal functions for running test cases.

static int run_testcase(struct tk_testcase *t, char *buf) {
    int r = 0;

    if (t->init) {
        // Run test setup
        t->init();
    }

    // Redirect both stdout and stderr to a memory buffer. This only
    // affects calls to printf() and fprintf() to stdout and stderr.
    // Writes to file descriptors will not be captured, nor will writes
    // to redirected file descriptors.

    FILE *fp = fmemopen(buf, TK_OUTPUT_LIMIT, "w+");
    tk_assert(fp, "fmemopen() should succeed");
    setbuf(fp, NULL);
    stdout = stderr = fp;

    if (t->stest) {
        // Run system test: call main() manually
        int main(int, const char **, const char **);
        extern const char **environ;

        r = main(t->argc, t->argv, environ);

        // Runt the bottom-half (test code).
        t->stest(&(struct tk_result) {
            .exit_status = r,
            .output = buf,
        });
    } else {
        // Run unit test: just run the test code.
        t->utest();
    }

    fclose(fp);
    return r;
}

static void run_cleanup(struct tk_testcase *t) {
    if (t->fini) {
        pid_t fini_pid = fork();
        if (fini_pid == 0) {
            // Cleanup function may also timeout.
            alarm(TK_TIME_LIMIT_SEC);
            t->fini();
            exit(0);
        } else {
            waitpid(fini_pid, NULL, 0);
        }
    }
}

static char *pcol(const char *s, int color) {
    // This is a single-threaded one-call per expression hack.
    static char buf[64];

    if (isatty(STDOUT_FILENO)) {
        snprintf(buf, sizeof(buf), "\033[0;%dm%s\033[0;0m", color, s);
    } else {
        snprintf(buf, sizeof(buf), "%s", s);
    }

    return buf;
}

static bool check_results(struct tk_testcase *t, int status) {
    // Print test result according to process exit status.
    bool succ = false;

    if (WIFEXITED(status)) {
        // Normal exit.
        succ = true;
        printf("- [%s] %s (%s)\n", pcol("PASS", 32), t->name, t->loc);
    } else {
        // Killed/stopped by a signal.
        printf("- [%s] %s (%s)", pcol("FAIL", 31), t->name, t->loc);
        const char *msg = pcol("unknown error", 31);

        if (WIFSIGNALED(status)) {
            int sig = WTERMSIG(status);
            switch (sig) {
                case SIGALRM: msg = pcol("Timeout", 33); break;
                case SIGABRT: msg = pcol("Assertion fail", 35); break;
                case SIGSEGV: msg = pcol("Segmentation fault", 36); break;
                default: msg = pcol(strsignal(sig), 31);
            }
        }
        printf(" - %s\n", msg);
    }

    return succ;
}

static void run_all_testcases(void) {
    if (ntests == 0) {
        // Don't bother non-testing runs.
        return;
    }

    // There are test cases only if there's TK_RUN or TK_VERBOSE.
    bool verbose = getenv(TK_VERBOSE) != NULL;

    // Creating subprocesses may cause multiple atexit flushes to the stdio
    // buffers. Clean them immediately and set stdout to non-buffered mode.
    fflush(stdout);
    fflush(stderr);
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);
    printf("\nTestKit\n");

    int passed = 0;

    for (int i = 0; i < ntests; i++) {
        struct tk_testcase *t = &tests[i];

        char *buf = mmap(NULL,
            TK_OUTPUT_LIMIT,
            PROT_READ | PROT_WRITE,
            MAP_SHARED | MAP_ANONYMOUS, -1, 0);
        tk_assert(buf != MAP_FAILED, "mmap() should succeed");

        // Run test case in a separated process.
        pid_t pid = fork();
        if (pid == 0) {
            // Child: run test case for TIME_LIMIT.
            alarm(TK_TIME_LIMIT_SEC);
            exit(run_testcase(t, buf));
        } else {
            // Parent: wait for child and run t->fini().
            int status;
            waitpid(pid, &status, 0);

            if (check_results(t, status)) {
                passed++;
            } else if (verbose) {
                printf(pcol("%s", 90), buf);
                if (!buf[0] || buf[strlen(buf) - 1] != '\n') {
                    printf("\n");
                }
            }

            // Cleanup code is also ran in a separate process.
            run_cleanup(t);
        }

        munmap(buf, TK_OUTPUT_LIMIT);
    }

    printf("- %d/%d test cases passed.\n", passed, ntests);

    for (int i = 0; i < ntests; i++) {
        struct tk_testcase *t = &tests[i];

        if (t->argv) {
            // Keep in mind that every malloc should eventually reach
            // its free. (But subprocesses have no chance to free them...)
            // Nevertheless; subprocesses are hack.
            free(t->argv);
            t->argv = NULL;
        }
    }
}

__attribute__((constructor))
void tk_register_hook(void) {
    // To run all tests before main() after all test case registrations,
    // we use a hack: run all tests at program exit.
    atexit(run_all_testcases);
}
