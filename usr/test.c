#include <stdio.h>
#include "knl/syshdlr.h"
#include "syscall.h"
#include "delay.h"

static void dywd()
{
    printf("AAA! I'm homicided!\n");
}

static void func()
{
    iiiDywd(2, dywd);
    char *msg[3] = { "Hello ", "meine ", "Fraunde!\n" };
    for (int i = 0; i < 3; i++) {
        printf(msg[i]);
        delay(250);
    }
    printf("I'm still alive...\n");
    for (;;);
}

void test()
{
    setvbuf(stdout, NULL, _IONBF, 0);
    printf("Hello, iii\n");
    pid_t pid = iiiNew(func, (char *[]){ "func", NULL }, 0);
    delay(600);
    iiiKill(pid, 2);
    for (;;);
}
