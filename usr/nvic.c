#include "knl/stm32f1xx.h"

void nvic(void)
{
    // Group: 4bit, Sub: 0bit
    SCB->AIRCR |= (0b011 << SCB_AIRCR_PRIGROUP_Pos);
    SCB->SHP[7] |= (1 << 4); // SVCall priority
    SCB->SHP[10] |= (15 << 4); // PendSv priority
    SCB->SHP[11] |= (15 << 4); // SysTick priority

    NVIC->IP[37] = 0; // USART1_IRQ priority
    NVIC->ISER[1] = (1 << 5); // USART1_IRQ enable
}
