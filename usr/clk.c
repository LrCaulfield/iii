#include "knl/stm32f1xx.h"

void SystemCoreClockUpdate(void);

void clk(void)
{
    FLASH->ACR = FLASH_ACR_LATENCY_2 | FLASH_ACR_PRFTBE;

                                    // PLLSTPRE = HSE
    RCC->CFGR = RCC_CFGR_PLLSRC
              | RCC_CFGR_PLLMULL9
                                   // HCLK = SYSCLK
              | RCC_CFGR_PPRE1_2
                                   // PCLK2 = HCLK
              | RCC_CFGR_ADCPRE_1;

    RCC->CR |= RCC_CR_HSEON;
    while (!(RCC->CR & RCC_CR_HSERDY_Msk)); // wait HSE ready
    RCC->CR |= RCC_CR_CSSON
            | RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY_Msk)); // wait PLL ready

    RCC->CFGR |= RCC_CFGR_SW_1;

    while ((RCC->CFGR & RCC_CFGR_SWS_Msk) != RCC_CFGR_SWS_1); // wait PLL to be used as SYSCLK
}
