#include "knl/stm32f1xx.h"
#include "syscall.h"
#include "shell.h"

void init(void)
{
    SystemInit();
    iiiNew(shell, NULL);
}
