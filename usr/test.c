#include <stdio.h>
#include "knl/stm32f1xx.h"
#include "knl/syshdlr.h"
#include "knl/thrd.h"
#include "syscall.h"
#include "delay.h"

static char msg[2] = "A";

static void msgChg()
{
    if (msg[0] == 'A') msg[0] = 'B';
    else msg[0] = 'A';
}

static void func()
{
    iiiDywd(3, msgChg);
    for (;;) {
        printf(msg);
        delay(150);
    }
}

void test()
{
    setvbuf(stdout, NULL, _IONBF, 0);
    pid_t pid = iiiNew(func, (char *[]){ "func", NULL }, 0, 0x400);
    for (;;) {
        delay(600);
        iiiKill(pid, 3);
    }
}
