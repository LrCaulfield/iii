#include <stdint.h>
#include <malloc.h>
#include <string.h>
#include <sys/types.h>
#include "cmsis_gcc.h"
#include "system_stm32f1xx.h"
#include "list.h"
#include "thrd.h"
#include "schel.h"
#include "syshdlr.h"
#include "usr/shell.h"
#include "usr/test.h"

extern char _estack;

void executor(void);


void *schel(void *psp)
{
    static bool booted;
    if (!booted) {
        booted = true;
        __set_CONTROL(0b11);
        return  thrd_run->psp;
    }

    thrd_run->psp = psp;
    listExch((struct Node *)thrd_run->prev);

    for (uint8_t pri = 0; pri < PRI_N; pri++) {
        if (thrd_heads[pri].next == &thrd_heads[pri]) continue;

        struct Thrd *thrd = (struct Thrd *)&thrd_heads[pri];
        do {
            thrd = thrd->next;
            if (thrd->sig) goto run;
            switch (thrd->stat) {
            case (DIE):
                thrdDel(thrd);
                continue;
            case (RUN):
run:
                thrd_run = thrd;
                return thrd_run->psp;
                break;
            default:
            }
        } while (thrd != (struct Thrd *)&thrd_heads[pri]);
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
    thrd_run = thrdCreat(shell, argv, 0);
    thrd_run->pare = NULL;
    thrd_run->chil = NULL;
    thrd_run->sibl = NULL;

    thrd_run->stat = RUN;
    return thrd_run->psp;
}
