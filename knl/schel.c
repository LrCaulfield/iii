#include <stdint.h>
#include <malloc.h>
#include <sys/types.h>
#include "cmsis_gcc.h"
#include "system_stm32f1xx.h"
#include "thrd.h"
#include "schel.h"
#include "syshdlr.h"
#include "usr/init.h"

void shell(int argc, char **argv);
void test(int argc, char **argv);

void *schel(void *psp)
{
    static bool boot;
    if (!boot) {
        boot = true;
        __set_CONTROL(0b11);

        for (uint8_t i = 0; i < PRI_N; i++) {
            thrd_heads[i] = malloc(sizeof(struct Thrd));
            thrd_heads[i]->next = thrd_heads[i];
            thrd_heads[i]->pid = 0;
        }
        char *argv[2] = { "shell" };
        thrd_run[0] = creatThrd((void *)shell, argv, 0);
        thrd_run[0]->stat = RUN;
        return thrd_run[0]->psp;
    }

    thrd_run[pri_run]->psp = psp;

    for (uint8_t pri = 0; pri < PRI_N; pri++) {
        if (thrd_run[pri] == NULL) continue;
        struct Thrd *thrd = thrd_run[pri]->next;
        do {
            if (thrd->pid == 0) thrd = thrd->next;
            if (thrd->stat == RUN || thrd->is_homicide) {
                thrd->is_homicide = false;
                thrd_run[pri] = thrd;
                return thrd_run[pri]->psp;
            } else if (thrd->stat == DIE) {
                struct Thrd *thrd_kill = thrd;
                thrd = thrd->next;
                delThrd(thrd_kill);
                continue;
            }
            thrd = thrd->next;
        } while (thrd != thrd_run[pri]->next);
    }
    for (;;);
}
