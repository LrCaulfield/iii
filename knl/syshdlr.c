#include <stdlib.h>
#include <string.h>
#include "knl/thrd.h"
#include "stm32f1xx.h"
#include "thrd.h"
#include "syshdlr.h"

void executor();
static void sysYield(struct Frm *frm);
static void sysNew(struct Frm *frm);
static void sysExit(struct Frm *frm);
static void sysWait(struct Frm *frm);
static void sysKill(struct Frm *frm);
static void sysDywd(struct Frm *frm);
static void checkWait(struct Thrd *thrd_zmb, int wstat);
static void sigSend(struct Thrd *thrd, enum Sig sig);
static void custodyTran(struct Thrd *thrd);

static void (*syshdlrs[])(struct Frm *frm) = {
    [SYS_YIELD] sysYield,
    [SYS_NEW]   sysNew,
    [SYS_EXIT]  sysExit,
    [SYS_WAIT]  sysWait,
    [SYS_KILL]  sysKill,
    [SYS_DYWD]  sysDywd,
};

void *distr(void *psp)
{
    struct Frm *frm = psp;
    enum SysNum num = frm->r7;
    syshdlrs[num](frm);
    return psp;
}

// void iiiYield(void);
void sysYield(struct Frm *frm)
{
    SCB->ICSR = SCB_ICSR_PENDSVSET_Msk; // set pend PendSV to swt
}

// pid_t
// iiiNew(void *func, char **argv, uint8_t pri size_t stk_sz);
static void sysNew(struct Frm *frm)
{
    void *func  = (void *)frm->r0;
    char **argv = (char **)frm->r1;
    uint8_t pri = frm->r2;
    size_t stk_sz = frm->r3;
    struct Thrd *thrd = thrdCreat(func, argv, pri, stk_sz);
    if (thrd == NULL) { frm->r0 = 0; return; }

    thrd->chil = NULL;
    thrd->pare = thrd_run;
    thrd->sibl = thrd_run->chil;
    thrd_run->chil = thrd;

    frm->r0 =  thrd->pid;
}

// pid_t iiiWait(pid_t pid, int *wstat);
static void sysWait(struct Frm *frm)
{
    pid_t pid = frm->r0;
    int *wstat = (int *)frm->r1;
    struct Thrd *thrd_zmb = thrdSrch(pid);
    if (thrd_zmb == NULL) {
        frm->r0 = -1;
        SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;
        return;
    }
    if (thrd_zmb->stat == ZMB) {
        thrd_zmb->stat = DIE;
        return;
    }
    thrd_run->stat = BLK;
    thrd_run->wpid = pid;
    thrd_run->wstat = wstat;
    SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;
}

// void iiiExit(int stat);
static void sysExit(struct Frm *frm)
{
    int wstat = frm->r0;
    checkWait(thrd_run, wstat);
    SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;
}

// int iiiKill(pid_t pid, enum Sig sig);
static void sysKill(struct Frm *frm)
{
    pid_t pid = frm->r0;
    enum Sig sig = frm->r1;
    if (sig > 9) { frm->r0 = -1; return; }
    sigSend(thrdSrch(pid), sig);

    frm->r0 = 0;
}

// void iiiDywd(enum Sig sig, void (*func)());
static void sysDywd(struct Frm *frm)
{
    enum Sig sig = frm->r0;
    void (*func)() = (void (*)())frm->r1;
    thrd_run->dywd[sig] = func;
}

static void checkWait(struct Thrd *thrd_zmb, int wstat)
{
    struct Thrd *thrd_pare = thrd_zmb->pare;
    if (thrd_pare->stat == BLK && thrd_pare->wpid == thrd_zmb->pid) {
        thrd_pare->stat = RUN;
        if (wstat) *thrd_pare->wstat = wstat;
        thrd_zmb->stat = DIE;
    } else {
        thrd_zmb->stat = ZMB;
    }
}

static void sigSend(struct Thrd *thrd, enum Sig sig)
{
    struct Frm *frm = thrd->psp;
    if (thrd == NULL || thrd->sig != SIG_NO) {
        frm->r0 = -1; return;
    }

    if (thrd->dywd[sig]) {
        thrd->sig = sig;
        thrd->psp -= 8;
        memcpy(thrd->psp, thrd->psp + 8, sizeof(struct Frm));
        struct Frm *frm = thrd->psp;
        *(void **)(thrd->psp + sizeof(struct Frm) + 4) =
            (void *)((uint32_t)(frm->pc) | 1);
        frm->pc = executor;
    }

    switch (sig) {
    case SIG_KILL:
        custodyTran(thrd);
        thrd->stat = DIE;
        break;
    default:
    }
}

static void custodyTran(struct Thrd *thrd)
{
    struct Thrd *thrd_pare = thrdSrch(1);
    thrd_pare->chil = thrd->chil;
    for (thrd = thrd->chil; thrd; thrd = thrd->sibl)
        thrd->pare = thrd_pare;
}
