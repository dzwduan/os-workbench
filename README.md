# M1: 打印进程树 (pstree)

[lab manual](https://jyywiki.cn/OS/2024/labs/M1.md)

## pstree功能

把系统中的进程按照父亲-孩子的树状结构打印到终端。

1. -p 或 --show-pids: 打印每个进程的进程号。
1. -n 或 --numeric-sort: 按照pid的数值从小到大顺序输出一个进程的直接孩子。
1. -V 或 --version: 打印版本信息。

你可以在命令行中观察系统的 pstree 的执行行为 (如执行 pstree -V、pstree --show-pids 等)。这些参数可能任意组合，但你不需要处理单字母参数合并的情况，例如 -np。

```
pstree: unrecognized option '--help'
Usage: pstree [-acglpsStTuZ] [ -h | -H PID ] [ -n | -N type ]
              [ -A | -G | -U ] [ PID | USER ]
   or: pstree -V

Display a tree of processes.

  -a, --arguments     show command line arguments
  -A, --ascii         use ASCII line drawing characters
  -c, --compact-not   don't compact identical subtrees
  -C, --color=TYPE    color process by attribute
                      (age)
  -g, --show-pgids    show process group ids; implies -c
  -G, --vt100         use VT100 line drawing characters
  -h, --highlight-all highlight current process and its ancestors
  -H PID, --highlight-pid=PID
                      highlight this process and its ancestors
  -l, --long          don't truncate long lines
  -n, --numeric-sort  sort output by PID
  -N TYPE, --ns-sort=TYPE
                      sort output by this namespace type
                              (cgroup, ipc, mnt, net, pid, time, user, uts)
  -p, --show-pids     show PIDs; implies -c
  -s, --show-parents  show parents of the selected process
  -S, --ns-changes    show namespace transitions
  -t, --thread-names  show full thread names
  -T, --hide-threads  hide threads, show only processes
  -u, --uid-changes   show uid transitions
  -U, --unicode       use UTF-8 (Unicode) line drawing characters
  -V, --version       display version information
  -Z, --security-context
                      show security attributes

  PID    start at this PID; default is 1 (init)
  USER   show only trees rooted at processes of this user
```
思考：
1. 打印方式如何呈现？-u --uid-changes这种格式如何呈现
1. 打印的格式？如何分辨父进程和子进程
1. pstree -V > /dev/null 发现没有输出到/dev/null, 如何解决？

## 问题分解
1. 得到命令行的参数，根据要求设置标志变量的数值； (get_opt)
1. 得到系统中所有进程的编号 (每个进程都会有唯一的编号) 保存到列表里； (查阅操作系统中的API, pid_t来访问看，以及procfs)
1. 对列表里的每个编号，得到它的的父亲是谁；
1. 在内存中把树建好，按命令行参数要求排序；
1. 把树打印到终端上。

## 需要实现的结果
```
shell >./pstree-32 / or ./pstree-64
systemd─┬─2*[agetty]
        ├─cron
        ├─dbus-daemon
        ├─init-systemd(Ub─┬─SessionLeader───Relay(467)───sh───sh───sh───node─┬─node─┬─bash
        │                 │                                                  │      ├─bash───pstree
        │                 │                                                  │      └─12*[{node}]
        │                 │                                                  ├─node─┬─clangd.main───17*[{clangd.main}]
        │                 │                                                  │      ├─node───10*[{node}]
        │                 │                                                  │      └─13*[{node}]
        │                 │                                                  ├─node───12*[{node}]
        │                 │                                                  └─10*[{node}]
        │                 ├─SessionLeader───Relay(1135)───node───6*[{node}]
        │                 ├─SessionLeader───Relay(1144)───node───6*[{node}]
        │                 ├─init───{init}
        │                 ├─login───bash
        │                 └─{init-systemd(Ub}
        ├─rsyslogd───3*[{rsyslogd}]
        ├─systemd───(sd-pam)
        ├─systemd-journal
        ├─systemd-logind
        ├─systemd-resolve
        ├─systemd-udevd
        ├─unattended-upgr───{unattended-upgr}
        └─wsl-pro-service───7*[{wsl-pro-service}]
```

## 思路
1. 主要卡的地方是打印进程树，一开始的思路是多建几个数组，然后产生父子的映射关系，再额外建一张表，代表在树的第几层，但是复杂度过高，占用空间也很大
1. 参考代码用的是二叉树结构代表整个进程树，需要先将树的结构存起来，后边就方便了
1. ./pstree-64 -n 不是默认会生成按pid排序的吗？ 查看 man pstree发现还能够根据type进行排序，因此需要预留一个cmp接口

        -n, --numeric-sort  sort output by PID
        -N TYPE, --ns-sort=TYPE
                      sort output by this namespace type
                              (cgroup, ipc, mnt, net, pid, time, user, uts)

1. 如何生成强劲的测试？Why not deepseek



## 参考的一些案例
1. https://github.com/phoulx/nju-os-workbench/blob/main/M1%20-%20pstree/pstree.c
2. https://github.com/Michael1015198808/os-workbench/blob/master/pstree/pstree.c