#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "knl/stm32f1xx.h"
#include "knl/syshdlr.h"
#include "syscall.h"
#include "usart.h"

extern char _end;

ssize_t iiiRead(int fd, char *buf, size_t len)
{
    (void)fd;
    int left = len - 1;

    while(left) {
        if (*usart_rp) {
            *buf = *usart_rp;
            left--;
            ptrInc(&usart_rp);
            if (*buf++ == '\n') {
                break;
            }
        }
    }
    *buf = '\0';
    return len - left;
}

ssize_t iiiWrite(int fd, char *ptr, size_t len)
{
    for (int i = 0; i < len; i++) {
        while (!(USART1->SR & USART_SR_TXE));
        USART1->DR = ptr[i];
    }
    while (!(USART1->SR & USART_SR_TC));
    return len;
}

/* ! */
void *mallocSv(size_t size)
{
    uint32_t basepri = __get_BASEPRI();
    __set_BASEPRI(1);
    void *p = malloc(size);
    __set_BASEPRI(basepri);
    return p;
}

int _fstat(int file, struct stat *st)
{
	st->st_mode = S_IFCHR;
	return 0;
}

int _isatty(int file)
{
	return 1;
}

int _lseek(int file, int ptr, int dir)
{
	return 0;
}

int _close(int file)
{
	return -1;
}

caddr_t _sbrk(int incr)
{
    #define HEAP_SIZE 0x1000
    
    static char *heap_end = &_end;
    char *prev_heap_end = heap_end;
    
    if (heap_end + incr > &_end + HEAP_SIZE) {
        return (caddr_t)-1;
    }
    
    heap_end += incr;
    return (caddr_t)prev_heap_end;
}

void _exit(int stat) __attribute__((alias("iiiExit")));

ssize_t _read(int fd, char *buf, size_t len)
    __attribute__((alias("iiiRead")));

ssize_t _write(int fd, char *ptr, size_t len)
    __attribute__((alias("iiiWrite")));
