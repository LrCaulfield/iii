#include <stdlib.h>
#include <string.h>
#include "knl/thrd.h"
#include "stm32f1xx.h"
#include "thrd.h"
#include "syshdlr.h"

static void sysYield(struct Frm *frm);
static void sysNew(struct Frm *frm);
static void sysExit(struct Frm *frm);
static void sysWait(struct Frm *frm);
static void sysKill(struct Frm *frm);
static void sysDywd(struct Frm *frm);
static bool checkWait(struct Thrd *thrd_zombie, int wstat);
void executor(void);

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

// pid_t iiiNew(void *func, char **argv);
static void sysNew(struct Frm *frm)
{
    void *func  = (void *)frm->r0;
    char **argv = (char **)frm->r1;
    struct Thrd *thrd = creatThrd(func, argv, 0);
    if (thrd == NULL) { frm->r0 = 0; return; }
    frm->r0 =  thrd->pid;
}

// pid_t iiiWait(pid_t pid, int *wstat);
static void sysWait(struct Frm *frm)
{
    SCB->ICSR = SCB_ICSR_PENDSVSET_Msk; // set pend PendSV to swt

    pid_t pid = frm->r0;
    struct Thrd *thrd_zombie = findThrd(pid);
    if (thrd_zombie == NULL) { frm->r0 = -1; return; }
    if (thrd_zombie->stat == ZOMBIE) {
        thrd_zombie->stat = DIE;
        return;
    }
    thrd_run[pri_run]->stat = PEND;
    thrd_run[pri_run]->rsn.type = WAIT;
    thrd_run[pri_run]->rsn.val.wait.pid = frm->r0;
    thrd_run[pri_run]->rsn.val.wait.wstat = (int *)frm->r1;
}

// void iiiExit(int stat);
static void sysExit(struct Frm *frm)
{
    SCB->ICSR = SCB_ICSR_PENDSVSET_Msk; // set pend PendSV to swt
    int wstat = frm->r0;
    if (checkWait(thrd_run[pri_run], wstat)) return;
    thrd_run[pri_run]->stat = ZOMBIE;
}

// int iiiKill(pid_t pid, enum Sig sig);
static void sysKill(struct Frm *frm)
{
    pid_t pid = frm->r0;
    enum Sig sig = frm->r1;
    struct Thrd *thrd_kill = findThrd(pid);
    
    if (thrd_kill->is_homicide || thrd_kill == NULL) {
        frm->r0 = -1; return;
    }
    thrd_kill->is_homicide = true;

    if (sig == SIG_KILL) {
        if (!checkWait(thrd_kill, sig)) thrd_kill->stat = ZOMBIE;
    }
    if (thrd_kill->dywd != NULL) {
        thrd_kill->psp -= 8;
        memcpy(thrd_kill->psp, thrd_kill->psp + 8, sizeof(struct Frm));
        struct Frm *frm_kill = thrd_kill->psp;
        *(void **)(thrd_kill->psp + sizeof(struct Frm) + 4) = frm_kill->pc;
        frm_kill->pc = executor;
    }

    frm->r0 = 0;
}

// void iiiDywd(void (*fund)(void));
static void sysDywd(struct Frm *frm)
{
    thrd_run[pri_run]->dywd = (void (*)(void))frm->r0;
}

static bool checkWait(struct Thrd *thrd_zombie, int wstat)
{
    for (int i = 0; i < PRI_N; i++) {
        for (
            struct Thrd *thrd_wait = thrd_heads[i]->next;
            thrd_wait != thrd_heads[i];
            thrd_wait = thrd_wait->next
        ) {
            if (thrd_wait->stat == PEND && thrd_wait->rsn.type == WAIT
            && thrd_wait->rsn.val.wait.pid == thrd_zombie->pid
            ) {
                if (thrd_wait->rsn.val.wait.wstat)
                    *(thrd_wait->rsn.val.wait.wstat) = wstat;
                thrd_wait->stat = RUN;
                thrd_zombie->stat = DIE;
                return true;
            }
        }
    }
    return false;
}
