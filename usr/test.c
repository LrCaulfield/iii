#include <stdio.h>
#include "syscall.h"

void func()
{
    printf("Hi, I'm func\n");
    iiiExit(123);
}

void test()
{
    printf("Hello, III\n");
    pid_t pid = iiiNew(func, (char *[]){ "func", NULL } );
    int wstat;
    iiiWait(pid, &wstat);
    printf("wstat == %d\n", wstat);
    for (;;);
}
