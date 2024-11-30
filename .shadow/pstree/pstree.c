#include <assert.h>
#include <bits/getopt_core.h>
#include <getopt.h>
#include <stdio.h>
#include <unistd.h>

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

int main(int argc, char *argv[]) {

  int ch;
  char *opt;
  while ((ch = getopt_long(argc, argv, short_opts, long_opts, NULL)) != -1) {
    switch (ch) {
    case 'p':
    case 'n':
    case 'V':
      opt=optarg;
      break;
    default:
      printf("parse args failed, input p / n / V\n");
      return -1;
    }
  }

  return 0;
}
