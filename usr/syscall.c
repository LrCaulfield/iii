#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "knl/stm32f1xx.h"
#include "knl/syshdlr.h"
#include "usart.h"

extern char _end;

static uint32_t
syscall(uint32_t sys_num, uint32_t arg0, uint32_t arg1, uint32_t arg2)
{
    register uint32_t r7 __asm__("r7") = sys_num;
    register uint32_t r0 __asm__("r0") = arg0;
    register uint32_t r1 __asm__("r1") = arg1;
    register uint32_t r2 __asm__("r2") = arg2;

    __asm__ volatile (
        "    svc #0     \n"
        : "+r"(r0)
        : "r"(r7), "r"(r1), "r"(r2)
        : "memory"
    );

    return r0;
}

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

void iiiYield(void)
{
    syscall(SYS_YIELD, 0, 0, 0);
}

pid_t iiiNew(void *func, char **argv)
{
    return syscall(SYS_NEW, (uint32_t)func, (uint32_t)argv, 0);
}

pid_t iiiWait(pid_t pid, int *_Nullable wstat)
{
    return syscall(SYS_WAIT, pid, (uint32_t)wstat, 0);
}

void iiiExit(int stat)
{
    syscall(SYS_EXIT, stat, 0, 0);
}

int iiiKill(pid_t pid, enum Sig sig)
{
    return syscall(SYS_KILL, pid, sig, 0);
}

void iiiDywd(void (*func)(void))
{
    syscall(SYS_DYWD, (uint32_t)func, 0, 0);
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

void _exit(int stat) { iiiExit(stat); }

ssize_t _read(int fd, char *buf, int len)
{
    return iiiRead(fd, buf, len);
}

ssize_t _write(int fd, char *ptr, int len)
{
    return iiiWrite(fd, ptr, len);
}
