#include <assert.h>
#include <bits/getopt_core.h>
#include <ctype.h>
#include <dirent.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define FILE_TYPE 8
#define DIR_TYPE 4

#define MAX_SIZE 1024

// int pid_list[MAX_SIZE] = {};
// char *pid_names[MAX_SIZE] = {};
// int ppid_list[MAX_SIZE] = {};
static int pid_idx = 0;
int tree_index_table[MAX_SIZE] = {};

const char *short_opts = "pn:V";
struct option long_opts[] = {{"show-pids", no_argument, NULL, 'p'},
                             {"numeric-sort", required_argument, NULL, 'n'},
                             {"version", no_argument, NULL, 'V'}};

void show_usage() {
  printf("Usage: pstree [-pnV]\n"
         "Display a tree of process.\n"
         "-p, --show-pids      show all pid's pid num\n"
         "-n, --numeric-sort   dump pid children by pid num from small to big\n"
         "-V, --version        dump version info");
}

void print_version() { fprintf(stderr, "pstree 0.0.1\n"); }

int is_digit(char *s) {
  if (s == NULL)
    return 0;
  for (int i = 0; s[i]; i++) {
    if (!isdigit(s[i]))
      return 0;
  }
  return 1;
}

char *get_name_by_pid(pid_t pid) {
  char pid_path[32];
  char buf[128];
  char *name = malloc(128);
  if (name == NULL)
    exit(1);
  sprintf(pid_path, "/proc/%d/status", pid);
  FILE *fp = fopen(pid_path, "r");
  if (fp == NULL)
    exit(1);
  if (fgets(buf, sizeof(buf) - 1, fp) != NULL) {
    sscanf(buf, "Name:   %s", name);
  }

  fclose(fp);
  // printf("pid = %d, name = %s\n", pid, name);
  return name;
}

pid_t get_ppid_by_pid(pid_t pid) {
  char pid_path[32];
  char buf[128];
  pid_t ppid = -1;
  sprintf(pid_path, "/proc/%d/status", pid);
  FILE *fp = fopen(pid_path, "r");
  if (fp == NULL)
    exit(1);
  while (fgets(buf, sizeof(buf) - 1, fp) != NULL) {
    if (sscanf(buf, "PPid: %d", &ppid) == 1) {
      break;
    }
  }
  fclose(fp);
  return ppid;
}

int *get_ppid_list(int ppid_len) {
  int *ppid_list = malloc(ppid_len * sizeof(int));
  if (ppid_list == NULL) {
    perror("malloc");
    exit(1);
  }
  for (int i = 0; i < ppid_len; i++) {
    ppid_list[i] = get_ppid_by_pid(i);
  }

  return ppid_list;
}

int *get_pid_list(int pid_len) {
  int *pid_list = malloc(pid_len * sizeof(int));
  if (pid_list == NULL) {
    perror("malloc");
    exit(1);
  }
  for (int i = 0; i < pid_len; i++) {
    pid_list[i] = i;
  }

  return pid_list;
}

char **get_pid_name_list(int pid_len) {
  // 默认name最大长度128
  char **pid_name_list = malloc(pid_len * sizeof(char *) * 128);
  if (pid_name_list == NULL) {
    perror("malloc");
    exit(1);
  }
  for (int i = 0; i < pid_len; i++) {
    pid_name_list[i] = get_name_by_pid(i);
  }

  return pid_name_list;
}

int get_pid_len() {
  struct dirent *f;
  DIR *dirp = opendir("/proc");
  if (dirp == NULL) {
    printf("failed open /proc\n");
    exit(1);
  }

  int pid_len = 0;
  while ((f = readdir(dirp)) != NULL) {
    if (f->d_type == DIR_TYPE) {
      if (is_digit(f->d_name)) {
        pid_len++;
      }
    }
  }

  closedir(dirp);
  return pid_len;
}

int get_root(int *pid_list, int *ppid_list) {
  int root_pid = -1;
  for (int i = 0; i < pid_idx; i++) {
    for (int j = 0; j < pid_idx; j++) {
      if (ppid_list[i] == pid_list[j]) {
        break;
      }
      if (j == pid_idx - 1) {
        root_pid = pid_list[i];
      }
    }
  }
  return root_pid;
}

void show_pids() {
  int pid_len = get_pid_len();
  int *pid_list = get_pid_list(pid_len);
  int *ppid_list = get_ppid_list(pid_len);
  char **pid_names = get_pid_name_list(pid_len);


  for (int i = 0; i < pid_idx; i++) {
    printf("%s - %d \n", pid_names[i], pid_list[i]);
  }
}

// dump tree by num pid as root
void sort_pids(int num) {}

int main(int argc, char *argv[]) {
  assert(argc >= 1);
  if (argc == 1) {
    show_usage();
    return 0;
  }
  int ch;
  char *opt;
  while ((ch = getopt_long(argc, argv, short_opts, long_opts, NULL)) != -1) {
    switch (ch) {
    case 'p':
      show_pids();
      break;
    case 'n':
      assert(is_digit(optarg));
      sort_pids(atoi(optarg));
    case 'V':
      print_version();
      break;
    default:
      printf("parse args failed, input ./pstree [p / n / V]\n");
      return -1;
    }
  }

  return 0;
}
