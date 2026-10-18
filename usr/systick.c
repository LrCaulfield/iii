#include "knl/stm32f1xx.h"

volatile uint32_t time_ms;

void systick(void)
{
    SysTick->LOAD = 9000;
    SysTick->VAL = 0;

    SysTick->CTRL = SysTick_CTRL_ENABLE_Msk;
}

