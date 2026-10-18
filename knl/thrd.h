#pragma once
#include <sys/types.h>
#include "signal.h"

#define MAX_THRD_N 10
#define PRI_N 1

enum Stat {
    RUN,
    PEND,
    ZOMBIE,
    DIE,
};

struct Wait {
    pid_t pid;
    int *wstat;
};

struct Rsn {
    union { struct Wait wait; int io; } val;
    enum { WAIT, IO, } type;
};

struct Thrd {
    struct Rsn rsn;
    pid_t pid;  // offset == 12
    void *psp;
    void *ptr;
    int argc;
    char **argv;
    enum Stat stat;
    int *wstat;
    void (*dywd)(void); // offset == 40
    bool is_homicide;
    struct Thrd *prev;
    struct Thrd *next;
};

struct Frm {
    uint32_t r4, r5, r6, r7,
             r8, r9, r10, r11,
             r0, r1, r2, r3,
             r12;
    void *lr, *pc;
    uint32_t psr;
};

extern uint8_t pri_run;
extern struct Thrd *thrd_run[];
extern struct Thrd *thrd_heads[];

struct Thrd *creatThrd(void *func, char **argv, uint8_t pri);
void delThrd(struct Thrd *thrd);
void rotatThrdList(uint8_t pri);
struct Thrd *findThrd(pid_t pid);
void printStat(struct Thrd *thrd);
