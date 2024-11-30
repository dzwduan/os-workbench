#include <assert.h>
#include <bits/getopt_core.h>
#include <string.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <ctype.h>

#define FILE_TYPE 8
#define DIR_TYPE  4

#define MAX_SIZE 1024

int pid_list[MAX_SIZE] = {}; 
char * pid_names[MAX_SIZE] = {};
int ppid_list[MAX_SIZE] = {};
static int pid_idx = 0;

const char *short_opts = "pnV";
struct option long_opts[] = {{"show-pids", no_argument, NULL, 'p'},
                             {"numeric-sort", no_argument, NULL, 'n'},
                             {"version", no_argument, NULL, 'V'}};

void show_usage() {
  printf("Usage: pstree [-pnV]\n"
         "Display a tree of process.\n"
         "-p, --show-pids      show all pid's pid num\n"
         "-n, --numeric-sort   dump pid children by pid num from small to big\n"
         "-V, --version        dump version info");
}


void print_version() {
  fprintf(stderr, "pstree 0.0.1\n");
}

int is_digit(char * s) {
  if (s==NULL) return 0;
  for(int i = 0; s[i]; i++) {
    if (!isdigit(s[i])) return 0;
  }
  return 1;
}

char * get_name_by_pid(pid_t pid) {
  char pid_path[32];
  char buf[128];
  char *name = malloc(128);
  if (name == NULL) exit(1);
  sprintf(pid_path, "/proc/%d/status", pid);
  FILE* fp = fopen(pid_path, "r");
  if (fp==NULL) exit(1);
  if(fgets(buf, sizeof(buf)-1, fp) != NULL) {
    sscanf(buf, "Name:   %s", name);
  }

  fclose(fp);
  //printf("pid = %d, name = %s\n", pid, name);
  return name;
}

pid_t get_ppid_by_pid(pid_t pid) {
  char pid_path[32];
  char buf[128];
  pid_t ppid = -1;
  sprintf(pid_path, "/proc/%d/status", pid);
  FILE* fp = fopen(pid_path, "r");
  if (fp == NULL) exit(1);
  while (fgets(buf, sizeof(buf) - 1, fp) != NULL) {
    if (sscanf(buf, "PPid: %d", &ppid) == 1) {
      break;
    }
  }
  fclose(fp);
  return ppid;
}

void show_pids() {
 struct dirent *f;
  DIR * dirp = opendir("/proc");
  if (dirp==NULL) {
    printf("failed open /proc\n");
    exit(1);
  }

  while((f = readdir(dirp))!=NULL) {
    if (f->d_type==DIR_TYPE) {
      if (is_digit(f->d_name)) {
        int pid = atoi(f->d_name);
        char name[32];
        pid_list[pid_idx] = pid;
        ppid_list[pid_idx] = get_ppid_by_pid(pid);
        pid_names[pid_idx] = get_name_by_pid(pid);
        pid_idx++;
      }
    }
  }

  //TODO: dump tree by pid parent and children

  closedir(dirp);
  for (int i = 0; i<pid_idx; i++) {
    printf("%s - %d \n", pid_names[i], pid_list[i]);
  }
}

void sort_pids() {

}

int main(int argc, char *argv[]) {

  int ch;
  char *opt;
  while ((ch = getopt_long(argc, argv, short_opts, long_opts, NULL)) != -1) {
    switch (ch) {
    case 'p':
      show_pids();
      break;
    case 'n':
      sort_pids();
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
