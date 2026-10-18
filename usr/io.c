#include "knl/stm32f1xx.h"

#define MODE_INPUT          (uint32_t)(0b00)
#define MODE                (uint32_t)(0b01)
#define MODE_LFQ            (uint32_t)(0b10)
#define MODE_HFQ            (uint32_t)(0b11)

#define CNF_ANALOG          (uint32_t)(0b00 << 2)
#define CNF_FLOAT           (uint32_t)(0b01 << 2)
#define CNF_PULL            (uint32_t)(0b10 << 2)

#define CNF_GPP             (uint32_t)(0b00 << 2)
#define CNF_GOD             (uint32_t)(0b01 << 2)
#define CNF_APP             (uint32_t)(0b10 << 2)
#define CNF_AOD             (uint32_t)(0b11 << 2)

void gpio(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
    RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;

    /* USART */
    GPIOA->CRH |= (((MODE_LFQ | CNF_APP) << (9 - 8) * 4)
            | ((MODE_INPUT | CNF_FLOAT) << (10 - 8) * 4));
    GPIOA->BSRR = GPIO_BSRR_BS10;

    /* BDLED */
    GPIOC->CRH |= ((MODE_LFQ | CNF_GOD) << (13 - 8) * 4);
    GPIOC->BSRR = GPIO_BSRR_BS13;

    /* EXLED */
    GPIOA->CRL |= ((MODE_LFQ | CNF_GOD) << 0 * 4);
    GPIOA->BSRR = GPIO_BSRR_BS0;
}

void afio(void)
{
}
