#include <assert.h>
#include <ctype.h>
#include <dirent.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define RED "\033[31m"
#define GREEN "\033[32m"
#define ORI "\033[0m"
#define LEN(_array) (sizeof(_array) / sizeof(_array[0]))
#define Assert(_con, _fmt, ...)                                                \
  do {                                                                         \
    if (!(_con)) {                                                             \
      fprintf(stderr, RED "Assertion failed @ %s:%d " _fmt, __FILE__,          \
              __LINE__, ##__VA_ARGS__);                                        \
      exit(-1);                                                                \
    }                                                                          \
  } while (0)

#define safe_printf(_str, ...)                                                 \
  do {                                                                         \
    int _written = snprintf(_str, sizeof(_str) - 1, __VA_ARGS__);              \
    Assert(_written > 0, "name len overflow\n");                               \
  } while (0)

enum CMP {
  CMP_PID,
  CMP_TYPE_IPC,
  CMP_TYPE_USER,
  CMP_TYPE_CGROUP,
  CMP_TYPE_MNT
};

bool show_pid = false;
uint8_t cmp = CMP_PID;

typedef struct proc {
  pid_t pid;
  pid_t ppid;
  uint8_t cnt;
  char *name;
  struct proc *child;
  struct proc *bro;
} proc_t;
//     bro     bro    bro     bro
// Proc*-->Proc*-->...-->Proc*|-->NULL(forbidden)
//  ^                        |
//  |child                   |
//  |                        |
// Proc*                      |

void show_usage() {
  printf("Usage: pstree [-pnV]\n"
         "Display a tree of process.\n"
         "-p, --show-pids      show all pid's pid num\n"
         "-n, --numeric-sort   dump pid children by pid num from small to big\n"
         "-V, --version        dump version info\n");
}

void print_version() { fprintf(stderr, "pstree 0.0.1\n"); }

void parse_args(int argc, char *argv[]) {
  const char *short_opts = "pn:V";
  struct option long_opts[] = {{"show-pids", no_argument, NULL, 'p'},
                               {"numeric-sort", required_argument, NULL, 'n'},
                               {"version", no_argument, NULL, 'V'},
                               {NULL, 0, NULL, 0}};
  int opt;
  Assert(argc >= 1, "argc < 1\n");
  if (argc == 1) {
    show_usage();
    return;
  }

  while ((opt = getopt_long(argc, argv, short_opts, long_opts, NULL)) != -1) {
    switch (opt) {
    case 'p':
      show_pid = true;
      break;
    case 'n':
      cmp = CMP_PID;
      break;
    case 'V':
      print_version();
      break;
    default:
      printf("parse args failed, input ./pstree [p / n / V]\n");
      return;
    }
  }
}

// read /proc/pid/status info
proc_t *create_proc(int pid, char *name, int ppid) {
  proc_t *proc = (proc_t *)malloc(sizeof(proc_t));
  proc->pid = pid;
  proc->ppid = ppid;
  proc->name = malloc(strlen(name) + 1);
  strncpy(proc->name, name, sizeof(proc->name) - 1);
  proc->cnt = 0;
  proc->child = NULL;
  proc->bro = NULL;
  return proc;
}

proc_t *read_proc_info(pid_t pid) {

  char info_path[128];
  char line[128];
  char name[128];
  pid_t ppid = 0;
  int ret = snprintf(info_path, sizeof(info_path), "/proc/%d/status", pid);
  Assert(ret > 0 && ret < sizeof(info_path), "read proc info error\n");

  FILE *fp = fopen(info_path, "r");
  if (!fp) {
    perror("fopen");
    return NULL;
  }
  while (fgets(line, sizeof(line), fp)) {
    if (strncmp(line, "Name:", 5) == 0) {
      sscanf(line, "Name: %s", name);
    }
    if (strncmp(line, "PPid:", 5) == 0) {
      sscanf(line, "PPid: %d", &ppid);
      break;
    }
  }

  fclose(fp);
  return create_proc(pid, name, ppid);
}

proc_t *find_proc(proc_t *root, int pid) {
  if (!root) return NULL;

  if (root->pid == pid) return root;

  proc_t *proc = find_proc(root->bro, pid);
  if (proc) return proc;

  return find_proc(root->child, pid);
}

void free_tree(proc_t * root) {
  if (!root) return;

  free_tree(root->child);
  free_tree(root->bro);
  free(root);
}

void build_tree(proc_t *root) {
  struct dirent *entry;
  DIR *dir = opendir("/proc");
  Assert(dir, "open /proc failed\n");

  while ((entry = readdir(dir))) {
    if (entry->d_type != DT_DIR)
      continue;
    if (!isdigit(entry->d_name[0]))
      continue;

    pid_t pid = atoi(entry->d_name);
    proc_t *proc = read_proc_info(pid);
    if (!proc)
      continue;
    proc_t * parent = find_proc(root, proc->ppid);
    Assert(parent, "find parent failed\n");

    // 有空间则直接插入
    if (!parent->child) {
      parent->child = proc;
    } else {
    // 否则插入到最后一个bro后面
      proc_t *last = parent->child;
      while (last->bro) {
        last = last->bro;
      }
      last->bro = proc;
    }
  }

  closedir(dir);
}


void print_tree(proc_t *root, int depth, char* prefix) {
  if (!root) return;

  printf("%s", prefix);
  if (depth > 0) {
    printf(root->bro == NULL ? "└──── " : "├──── ");
  }

  if (show_pid) {
    printf("%s(%d)\n", root->name, root->pid);
  } else {
    printf("%s\n", root->name);
  }

  char new_perfix[128];
  strncpy(new_perfix, prefix, sizeof(new_perfix) - 1);
  if (depth > 0) {
    strcat(new_perfix, root->bro == NULL ? "     " : "│    ");
  }

  print_tree(root->bro, depth, prefix);
  print_tree(root->child, depth+1, new_perfix);
}

int main(int argc, char *argv[]) {

  parse_args(argc, argv);

  proc_t *root = create_proc(0, "idle", 0);

  build_tree(root);

  print_tree(root, 0, "");

  free_tree(root);

  return 0;
}
