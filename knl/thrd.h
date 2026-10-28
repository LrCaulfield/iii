#pragma once
#include <sys/types.h>
#include "syshdlr.h"
#include "list.h"

#define MAX_THRD_N 10
#define PRI_N 2
#define THRD_STACK_SIZE 0x200

enum Stat { RUN, BLK, ZMB, DIE, };

struct Thrd {
    struct Thrd *prev;
    struct Thrd *next;

    pid_t pid;
    void *psp;
    void *ptr;
    int argc;
    char **argv;
    int *wstat;
    void (*dywd[10])(void);

    struct Thrd *pare;
    struct Thrd *chil;
    struct Thrd *sibl;

    pid_t wpid;
    enum Stat stat;
    enum Sig sig;
};

struct Frm {
    uint32_t r4, r5, r6, r7,
             r8, r9, r10, r11,
             r0, r1, r2, r3,
             r12;
    void *lr, *pc;
    uint32_t psr;
};

extern struct Thrd *thrd_run;
extern struct Node thrd_heads[PRI_N];

struct Thrd *thrdCreat(void *func, char **argv, uint8_t pri);
void thrdDel(struct Thrd *thrd);
struct Thrd *thrdSrch(pid_t pid);
struct Thrd *thrdSwt(struct Thrd *thrd);
void printStat(struct Thrd *thrd);
