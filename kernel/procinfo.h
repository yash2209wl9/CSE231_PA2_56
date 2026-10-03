#ifndef PROCINFO_H
#define PROCINFO_H
struct procinfo {
  int pid;
  int ppid;
  uint64 sz;
  char name[16];
};
#endif
