#pragma once
#include <stddef.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "knl/syshdlr.h"

ssize_t iiiRead(int fd, char *ptr, size_t len);
ssize_t iiiWrite(int fd, char *ptr, size_t len);
void iiiYield(void);
pid_t iiiNew(void *func, char **argv, uint8_t pri);
pid_t iiiWait(pid_t pid, int *_Nullable wstat);
void iiiExit(int stat);
int iiiKill(pid_t pid, enum Sig sig);
void iiiDywd(enum Sig sig, void (*func)(void));
void *mallocSv(size_t size);

caddr_t _sbrk(int incr);
int _fstat(int file, struct stat *st);
int _isatty(int file);
int _lseek(int file, int ptr, int dir);
int _close(int file);
void _exit(int stat);
void _read(int fd, char *buf, int len);
void _write(int fd, char *ptr, int len);
