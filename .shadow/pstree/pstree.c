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

char pid_list[MAX_SIZE] = {}; 
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
  printf("pstree 0.0.1\n");
}

int is_digit(char * s) {
  if (s==NULL) return 0;
  for(int i = 0; s[i]; i++) {
    if (!isdigit(s[i])) return 0;
  }
  return 1;
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
        printf("%s ", f->d_name);
        pid_list[pid_idx++] = atoi(f->d_name);
      }
    }
  }

  for (int i = 0; i<pid_idx; i++) {
    printf("%d ", pid_list[i]);
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
