#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "knl/stm32f1xx.h"
#include "knl/syshdlr.h"
#include "syscall.h"
#include "led.h"
#include "delay.h"

static void shutdown()
{
    GPIOC->BSRR = GPIO_BSRR_BS13;
}

static void sigHdlr()
{
    printf("recieve sig 3\n");
}

static void shutdownEx()
{
    GPIOA->BSRR = GPIO_BSRR_BS0;
}

void flash(int argc, char **argv)
{
    if (argc != 2) {
        char msg[] = "flash: usage: flash {toggle delay}\n";
        iiiWrite(1, msg, strlen(msg));
        iiiExit(1);
    }
    iiiDywd(SIG_KILL, shutdown);
    iiiDywd(3, sigHdlr);
    for (;;) {
        GPIOC->BSRR = GPIO_BSRR_BR13;
        delay(atoi(argv[1]));
        GPIOC->BSRR = GPIO_BSRR_BS13;
        delay(atoi(argv[1]));
    }
}

void flashEx(int argc, char **argv)
{
    if (argc != 2) {
        char msg[] = "flashEx: usage: flashEx {toggle delay}\n";
        iiiWrite(1, msg, strlen(msg));
        iiiExit(1);
    }
    iiiDywd(SIG_KILL, shutdownEx);
    for (;;) {
        GPIOA->BSRR = GPIO_BSRR_BR0;
        delay(atoi(argv[1]));
        GPIOA->BSRR = GPIO_BSRR_BS0;
        delay(atoi(argv[1]));
    }
}

void led(int argc, char **argv)
{
    char msg[] = "led: usage: led {on | off}\n";
    if (argc != 2) {
        perror(msg);
        iiiExit(1);
    }
    if (!strcmp(argv[1], "on")) {
        GPIOC->BSRR = GPIO_BSRR_BR13;
        iiiExit(0);
    }
    if(!strcmp(argv[1], "off")) {
        GPIOC->BSRR = GPIO_BSRR_BS13;
        iiiExit(0);
    }
    perror(msg);
    delay(200);
    iiiExit(1);
}
