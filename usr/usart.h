#pragma once

#define USART_BUF_SIZE 0x100

extern char usart_buf[USART_BUF_SIZE];
extern char *usart_wp;
extern char *usart_rp;

void usart(void);
void ptrInc(char **p);
