#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "thrd.h"
#include "list.h"

struct Thrd *thrd_run;
struct Node thrd_heads[PRI_N];

static pid_t pidalloc(void)
{
    for (pid_t pid = 1; pid < MAX_THRD_N; pid++) {
        if (thrdSrch(pid)) continue;
        return pid;
    }
    return 0;
}

struct Thrd
*thrdCreat(void *func, char **argv, uint8_t pri, size_t stk_sz)
{
    pid_t pid = pidalloc();
    if (pid == 0) return nullptr;

    struct Thrd *thrd = nodeCreat(sizeof(struct Thrd));
    if (thrd == nullptr) { return nullptr; }

    thrd->ptr = malloc(stk_sz);
    if (!thrd->ptr) {
        free((void *)thrd);
        return nullptr;
    }

    thrd->psp = thrd->ptr + stk_sz - 16 * 4;
    
    for (thrd->argc = 0; argv[thrd->argc] != nullptr; thrd->argc++);
    thrd->argv = thrd->ptr;
    char *ptr = (char *)(thrd->argv + thrd->argc);
    for (int i = 0; i < thrd->argc; i++) {
        thrd->argv[i] = ptr;
        strcpy(ptr, argv[i]);
        ptr += strlen(argv[i]) + 1;
    }

    struct Frm *frm = thrd->psp;
    frm->r0 = thrd->argc;
    frm->r1 = (uint32_t)thrd->argv;
    frm->lr = nullptr;
    frm->pc = func;
    frm->psr = 0x01000000;

    listIns((struct Node *)thrd, &thrd_heads[pri]);

    thrd->pid = pid;
    thrd->stat = RUN;
    memset(thrd->dywd, 0, sizeof(thrd->dywd));
    thrd->sig = SIG_NO;

    return thrd;
}

void thrdDel(struct Thrd *thrd)
{
    listRm(thrd);
    free(thrd->ptr);
    free(thrd);
}

struct Thrd *thrdSrch(pid_t pid)
{
    for (uint8_t pri = 0; pri < PRI_N; pri++) {
        for (struct Thrd *thrd = (struct Thrd *)thrd_heads[pri].next;
             thrd != (struct Thrd *)&thrd_heads[pri];
             thrd = thrd->next)
            if (thrd->pid == pid) return thrd;
    }
    return nullptr;
}

struct Thrd *thrdSwt(struct Thrd *thrd)
{
    for (uint8_t pri = 0; pri < PRI_N; pri++)
        if (thrd->next == (struct Thrd *)&thrd_heads[pri])
            return (struct Thrd *)thrd_heads[pri].next;
    return thrd->next;
}

void printStat(struct Thrd *thrd)
{
    switch (thrd->stat) {
    case RUN:
        printf("RUN");
        break;
    case BLK:
        printf("BLK");
        break;
    case ZMB:
        printf("ZMB");
        break;
    case DIE:
        printf("DIE");
        break;
    }
}
