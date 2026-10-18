#include "knl/stm32f1xx.h"
#include "usart.h"

char usart_buf[USART_BUF_SIZE];
char *usart_wp = usart_buf;
char *usart_rp = usart_buf;

void usart(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN;

    USART1->BRR = 7500;
    USART1->CR1 = USART_CR1_UE;

    USART1->CR1 |= USART_CR1_TE;
    USART1->CR1 |= USART_CR1_RE;

    USART1->CR1 |= USART_CR1_RXNEIE;
}

void USART1_IRQHandler(void)
{
    NVIC->ICPR[1] = (1 << 5); // USART1_IRQ clear-pending
    *usart_wp = USART1->DR;
    ptrInc(&usart_wp);
}

void ptrInc(char **p)
{
    if (*p - usart_buf < USART_BUF_SIZE) {
        (*p)++;
    } else {
        *p = usart_buf;
    }
}
