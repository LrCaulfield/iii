#include <stdint.h>
#include <malloc.h>
#include <string.h>
#include <sys/types.h>
#include "cmsis_gcc.h"
#include "system_stm32f1xx.h"
#include "list.h"
#include "thrd.h"
#include "sched.h"
#include "syshdlr.h"
#include "usr/shell.h"
#include "usr/test.h"

extern char _estack;

void executor(void);


void *sched(void *psp)
{
    static bool booted;
    if (!booted) {
        booted = true;
        __set_CONTROL(0b11);
        return  thrd_run->psp;
    }

    thrd_run->psp = psp;
    while (thrd_run->stat == DIE) {
        struct Thrd *thrd_die = thrd_run;
        thrd_run = thrdSwt(thrd_run);
        thrdDel(thrd_die);
    }
    struct Thrd *thrd = thrdSwt(thrd_run);

    for (uint8_t pri = 0; pri < PRI_N; pri++) {
        if (thrd_heads[pri].next == &thrd_heads[pri]) continue;

        do {
            if (thrd->sig) goto run;
            switch (thrd->stat) {
            case (DIE):
                struct Thrd *thrd_die = thrd;
                thrd = thrdSwt(thrd);
                thrdDel(thrd_die);
                continue;
            case (RUN):
run:
                thrd_run = thrd;
                return thrd_run->psp;
                break;
            default:
            }
            thrd = thrdSwt(thrd);
        } while (thrd != thrd_run->next);
    }
    for (;;);
}

void *iiiInit()
{
    for (uint8_t pri = 0; pri < PRI_N; pri++) {
        thrd_heads[pri].prev = &thrd_heads[pri];
        thrd_heads[pri].next = &thrd_heads[pri];
    }
    char *argv[2] = { "shell", NULL, };
    thrd_run = thrdCreat(shell, argv, 0, 0x400);
    while (!thrd_run);
    thrd_run->pare = NULL;
    thrd_run->chil = NULL;
    thrd_run->sibl = NULL;

    thrd_run->stat = RUN;
    return thrd_run->psp;
}
