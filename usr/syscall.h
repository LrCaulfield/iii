#pragma once
#include <stddef.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "knl/syshdlr.h"

ssize_t iiiRead(int fd, char *ptr, size_t len);
ssize_t iiiWrite(int fd, char *ptr, size_t len);
void *mallocSv(size_t size);

caddr_t _sbrk(int incr);
int _fstat(int file, struct stat *st);
int _isatty(int file);
int _lseek(int file, int ptr, int dir);
int _close(int file);
void _exit(int stat);
ssize_t _read(int fd, char *buf, size_t len);
ssize_t _write(int fd, char *ptr, size_t len);

static inline uint32_t
syscall(uint32_t sys_num, uint32_t arg0, uint32_t arg1,
        uint32_t arg2, uint32_t arg3)
{
    register uint32_t sys_numR __asm__("r7") = sys_num;
    register uint32_t arg0R __asm__("r0") = arg0;
    register uint32_t arg1R __asm__("r1") = arg1;
    register uint32_t arg2R __asm__("r2") = arg2;
    register uint32_t arg3R __asm__("r3") = arg3;
    register uint32_t ret __asm__("r0");
    __asm__ volatile (
        "svc #0 \n"
        : "=r"(ret)
        : "r"(sys_numR), "r"(arg0R), "r"(arg1R), "r"(arg2R), "r"(arg3R)
        : "memory"
    );
    return ret;
}

static inline void iiiYield(void)
{
    syscall(SYS_YIELD, 0, 0, 0, 0);
}

static inline pid_t
iiiNew(void *func, char **argv, uint8_t pri, size_t stk_sz)
{
    return syscall(SYS_NEW, (uint32_t)func, (uint32_t)argv,
                   pri, stk_sz);
}

static inline pid_t iiiWait(pid_t pid, int *_Nullable wstat)
{
    return syscall(SYS_WAIT, pid, (uint32_t)wstat, 0, 0);
}

static inline void iiiExit(int stat)
{
    syscall(SYS_EXIT, stat, 0, 0, 0);
}

static inline int iiiKill(pid_t pid, enum Sig sig)
{
    return syscall(SYS_KILL, pid, sig, 0, 0);
}

static inline void iiiDywd(enum Sig sig, void (*func)(void))
{
    syscall(SYS_DYWD, sig, (uint32_t)func, 0, 0);
}
