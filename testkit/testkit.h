/*
 *  _________  _______   ________  _________  ___  __    ___  _________
 * |\___   ___\\  ___ \ |\   ____\|\___   ___\\  \|\  \ |\  \|\___   ___\
 * \|___ \  \_\ \   __/|\ \  \___|\|___ \  \_\ \  \/  /|\ \  \|___ \  \_|
 *      \ \  \ \ \  \_|/_\ \_____  \   \ \  \ \ \   ___  \ \  \   \ \  \
 *       \ \  \ \ \  \_|\ \|____|\  \   \ \  \ \ \  \\ \  \ \  \   \ \  \
 *        \ \__\ \ \_______\____\_\  \   \ \__\ \ \__\\ \__\ \__\   \ \__\
 *         \|__|  \|_______|\_________\   \|__|  \|__| \|__|\|__|    \|__|
 *                         \|_________|
 * 
 * TestKit: Writing test cases fearlessly, wherever you go!
 * 
 * - Include `testkit.h` (and link with `testkit.c`) is all you need.
 * - Set TK_RUN environment variable (regardless of its value), all test
 *   cases will automatically run after the (normal) program exits.
 * - Set TK_VERBOSE will print program outputs for failed test cases.
 * 
 * Minimal Example (test.c):
 * 
 *   #include "testkit.h"
 *   
 *   UnitTest(put_me_anywhere) {
 *     tk_assert(114514 == 0x114514, "This will not do.");
 *   }
 * 
 *   int main() {}  // cc test.c testkit.c && TK_VERBOSE= ./a.out
 */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

/** Maximum number of allowed test cases. */
#define TK_MAX_TESTS       1024
/** Time limit (in seconds) for each test case. */
#define TK_TIME_LIMIT_SEC  1
/** Output limit (bytes) for output capture in struct tk_result. */
#define TK_OUTPUT_LIMIT    (1 << 20)

/** Environment variables for enabling TestKit. */
#define TK_RUN     "TK_RUN"
#define TK_VERBOSE "TK_VERBOSE"

/** System test run result: exit status and combined stdout and stderr. */
struct tk_result {
    int exit_status;
    const char *output;
};

/**
 * A test case with initialization, test, and finalization functions.
 * Unit tests are "one-time" function runners; system tests are invocation
 * of main() function with argument vector.
 */
struct tk_testcase {
    const char *name; // name of test case; must be a valid C identifier
    const char *loc; // the program location of this test case
    void (*init)(void); // pre-test setup function (optional)
    void (*fini)(void); // post-test cleanup function (optional)

    // For unit tests:
    void (*utest)(void); // unit test body

    // For system tests:
    void (*stest)(struct tk_result *); // test body
    int argc;
    const char **argv;
};

/**
 * Evaluates the condition (cond); if false, prints an error message
 * via printf format. Use this instead of standard assert() to get more
 * error details. Example:
 * 
 *   tk_assert(list != NULL, "NULL list, found %p", ptr);
 */
#define tk_assert(cond, fmt, ...) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, "Assertion failed: (%s)\n" \
                            "    In %s of %s:%d\n", \
                    #cond, __func__, __FILE__, __LINE__); \
            fprintf(stderr, "    " fmt, ##__VA_ARGS__); \
            fprintf(stderr, "\n"); \
            abort(); \
        } \
    } while (0)

#ifdef assert
    // Override system "assert": it uses fd instead of stderr
    #undef assert
#endif
#define assert(cond) tk_assert(cond, "Assertion violated")

/**
 * Declares a unit test function that will run once during testing.
 * 
 * Parameters:
 * 
 * - name: Test case name.
 * - Variadic arguments: Additional named fields (such as .init, .fini)
 *   that customize the test case.
 * - Must be followed by the test case body.
 * 
 * Example:
 * 
 *   UnitTest(example_test, .init = setup, .fini = cleanup) {
 *     // Your test code here.
 *     // tk_assert(1 == 1, "This should never fail.");
 *   }
 * 
 * Notes:
 * 
 * - The test body function is automatically registered so that it runs
 *   as part of the test suite.
 * - The post-test cleanup function is called even if the test crashes.
*/
#define UnitTest(name, ...) \
    __tk_testcase(name, void, utest, __VA_ARGS__)

/**
 * Declares a system test that involves invoking the main() function with a
 * custom argument vector.
 * 
 * Parameters:
 * 
 * - name: Test case name.
 * - argv_: A list/array of arguments (const char *[]) that are passed to
 *   main() during testing.
 * - Variadic arguments: Additional named initializations (for example,
 *   .init, .fini) for further customization.
 * 
 * Example:
 * 
 *   SystemTest(sys_test_example,
 *              ((const char *[]){ "program", "--flag" }),
 *              .init = setup, .fini = cleanup) {
 *     // Access the "result" variable of type "struct tk_result *" to
 *     // check the program results.
 *     tk_assert(result->exit_status == 0,
 *               "Expected exit status 0, got %d", result->exit_status);
 *     tk_assert(strstr(result->output, "succ"),
 *               "Program should report successful.");
 *   }
 *
 * Notes:
 * 
 * - result->output **only capture default printf() and fprintf()
 *   results** for the test body; it does not get outputs to
 *   STDOUT_FILENO/STDERR_FILENO.
 * - Automatically computes argc based on the provided argv_ array.
 * - Simulates real command-line invocations of your program.
 * - The post-test cleanup function is called even if the test crashes.
 */
#define SystemTest(name, argv_, ...) \
    __tk_testcase(name, \
        struct tk_result *result, stest, \
        .argc = sizeof(argv_) / sizeof(void *), \
        .argv = (const char **)argv_, \
        __VA_ARGS__)

// ------------------------------------------------------------------------
// Below are helpers.

/** Concatenates two tokens x and y using the token-pasting operator. */
#define TK_CONCAT_DETAIL(x, y) x##y

/** Expands arguments before concatenating them */
#define TK_CONCAT(x, y) TK_CONCAT_DETAIL(x, y)

/**
 * Generates a unique identifier by concatenating "__tk_", the provided
 * prefix, an underscore, and the current line number (__LINE__), to make
 * sure that the name is unique within the code.
 */
#define TK_UNIQUE_NAME(prefix) TK_CONCAT(__tk_##prefix##_, __LINE__)

/** Converts x to a string literal (without expanding macros) */
#define TK_STRINGIFY(x) #x

/** Expands x if it's a macro, then converts it to a string literal */
#define TK_TOSTRING(x) TK_STRINGIFY(x)

/**
 * Internal helper macro that abstracts test definition and
 * auto-registration logic. It creates the actual test function
 * (with name like __tk_utest_<name_>) and registers it by calling
 * tk_add_test().
 * 
 * This is macro magic. We all hate it because it makes debugging hard.
 * (We need procedural marcos!)
 * 
 * Parameters:
 * 
 * - name_: The name token of the test case.
 * - body_arg: The argument signature for the test function (e.g., void for
 *   unit tests or struct tk_result * for system tests).
 * - test: Specifies whether this is a unit/system test (utest/stest).
 * - Variadic arguments: Extra fields to initialize the tk_testcase
 *   structure: .init, .fini, .argc, .argv, etc..
 * 
 * How It Works?
 * 
 * - Declare a static test function with a prefixed name __tk_test to
 *   avoid naming collisions.
 * - Register the test case at startup using a constructor function.
 * - The registration function calls tk_add_test() to add the test case to
 *   the suite.
 * 
 * Example:
 * 
 *   UnitTest(inf_loop) { while (1); }
 * 
 * will be expanded to:
 * 
 *   // The "test" run function, whose name is generated by TK_UNIQUE_NAME
 *   // using line number.
 *   static void __tk_inf_loop_10(void);
 * 
 *   // The hidden constructor function, which is automatically called
 *   // before main().
 *   __attribute__((constructor))
 *   void __tk_reginf_loop_10() {
 *     void tk_add_test(struct tk_testcase t);
 *     tk_add_test( (struct tk_testcase) {
 *       .name = "inf_loop",
 *       .utest = __tk_inf_loop_10,
 *     });
 *   }
 * 
 *   static void __tk_inf_loop_10(void)
 *   // "UnitTest(inf_loop)" expands to the code above.
 *   { while (1); }
 * 
 * TestKit will stop this loop after TK_TIME_LIMIT_SEC seconds.
 */
#define __tk_testcase(name_, body_arg, test, ...) \
    /* Declare the test function, e.g., __tk_test_example. */ \
    static void TK_UNIQUE_NAME(name_)(body_arg); \
    \
    /* Define a pre-main constructor to register this test case. */ \
    __attribute__((constructor)) \
    void TK_UNIQUE_NAME(reg##name_)() { \
        void tk_add_test(struct tk_testcase t); \
        \
        /* Call tk_add_test() to register. */ \
        tk_add_test( (struct tk_testcase) { \
            .name = #name_, \
            .loc = __FILE__ ":" TK_TOSTRING(__LINE__),\
            .test = TK_UNIQUE_NAME(name_), \
            /* Variadic arguments are like: */ \
            /* .init = ..., .argv = ... */ \
            __VA_ARGS__ \
        } ); \
    } \
    \
    /* Define the test function. */ \
    static void TK_UNIQUE_NAME(name_)(body_arg)
    // Followed by the test case body { ... }
