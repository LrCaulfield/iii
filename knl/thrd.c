#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "thrd.h"
#include "usr/init.h"

uint8_t pri_run;
struct Thrd *thrd_run[PRI_N];
struct Thrd *thrd_heads[PRI_N];

static pid_t pidalloc(void)
{
    for (pid_t pid = 1; pid < MAX_THRD_N; pid++) {
        for (uint8_t i = 0; i < PRI_N; i++) {
            for (
                struct Thrd *thrd = thrd_heads[i]->next;
                thrd != thrd_heads[i];
                thrd = thrd->next
            ) if (thrd->pid == pid) goto inc_pid;
        }
        return pid;
inc_pid:
    }
    return 0;
}

struct Thrd
*creatThrd(void *func, char **argv, uint8_t pri)
{
    #define THREAD_STACK_SIZE 0x200

    pid_t pid = pidalloc();
    if (pid == 0) return NULL;

    struct Thrd *thrd = malloc(sizeof(struct Thrd));
    if (thrd == NULL) { return NULL; }

    thrd->prev = thrd_heads[pri];
    thrd->next = thrd_heads[pri]->next;
    thrd_heads[pri]->next = thrd;

    if ((thrd->ptr = malloc(THREAD_STACK_SIZE)) == NULL) {
        free((void *)thrd);
        return NULL;
    }

    thrd->psp = thrd->ptr + THREAD_STACK_SIZE - 16 * 4;
    
    for (thrd->argc = 0; argv[thrd->argc] != NULL; thrd->argc++);
    thrd->argv = thrd->ptr;
    char *ptr = (char *)(thrd->argv + thrd->argc);
    for (int i = 0; i < thrd->argc; i++) {
        thrd->argv[i] = ptr;
        strcpy(ptr, argv[i]);
        ptr += strlen(argv[i]) + 1;
    }

    thrd->pid = pid;
    thrd->stat = RUN;
    thrd->dywd = NULL;
    thrd->is_homicide = false;

    struct Frm *frm = thrd->psp;
    frm->r0 = thrd->argc;
    frm->r1 = (uint32_t)thrd->argv;
    frm->lr = NULL;
    frm->pc = func;
    frm->psr = 0x01000000;

    return thrd;
}

void delThrd(struct Thrd *thrd)
{
    thrd->next->prev = thrd->prev;
    thrd->prev->next = thrd->next;
    free(thrd->ptr);
    free((void *)thrd);
}

struct Thrd *findThrd(pid_t pid)
{
    for (uint8_t pri = 0; pri < PRI_N; pri++) {
        for (
            struct Thrd *thrd = thrd_heads[pri]->next;
            thrd != thrd_heads[pri]; thrd = thrd->next
        ) if (thrd->pid == pid) return thrd;
    }
    return NULL;
}

struct Thrd *swtThrd(struct Thrd *thrd, uint8_t pri)
{
    thrd = thrd->next;
    if (thrd->pid == 0) thrd = thrd->next;
    return thrd;
}

void printStat(struct Thrd *thrd)
{
    switch (thrd->stat) {
    case RUN:
        printf(" RUN");
        break;
    case PEND:
        printf(" PEND");
        break;
    case ZOMBIE:
        printf(" ZOMBIE");
        break;
    case DIE:
        printf(" DIE");
        break;
    }
}
