#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "cmsis_gcc.h"
#include "knl/thrd.h"
#include "knl/syshdlr.h"
#include "syscall.h"
#include "led.h"

#define TOKEN_N 10
#define CMD_N 10

static int tokenize(char *str);
static bool isEmp(char c);
static void cmdKill(int argc, char **argv);
void cmdPs(int argc, char **argv);
void status(int argc, char **argv);

struct Cmd {
    char *cmd;
    void (*func)(int, char **);
};

static char buf[0x100] = { 0 };
static char *tokens[TOKEN_N] = { 0 };
static struct Cmd cmds[CMD_N] = {
    { "kill", cmdKill },
    { "ps", cmdPs },
    { "flash", flash },
    { "flashEx", flashEx },
    { "led", led },
};

void shell(int argc, char **argv)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    char *prompt = "$ ";
    for (;;) {
        printf(prompt);

        iiiRead(0, buf, sizeof(buf));
        int argc = tokenize(buf);
        if (argc == 0) continue;

        uint8_t w = 1;
        if (!strcmp(tokens[argc - 1], "&")) {
            argc--;
            w = 0;
        }
        tokens[argc] = NULL;

        for (int i = 0; i < CMD_N; i++) {
            if (!strcmp(tokens[0], cmds[i].cmd)) {
                pid_t pid = iiiNew(cmds[i].func, tokens);
                if (w) iiiWait(pid, NULL);
                goto ctn;
            }
        }
        fprintf(stderr, "shell: cmd `%s' not found\n", tokens[0]);
ctn:
    }
}

static int tokenize(char *str)
{
    uint8_t mark = 1;
    int i = 0;
    for (; *str != '\n'; str++) {
        if (mark && !isEmp(*str)) {
            tokens[i++] = str;
            if (i >= TOKEN_N) return -1;
            mark = 0;
        } else if (isEmp(*str)) {
            *str = '\0';
            mark = 1;
        }
    }
    *str = '\0';
    return i;
}

static bool isEmp(char c)
{
    if (   c == '\t'
        || c == '\n'
        || c == '\r'
        || c == ' '
    ) {
        return true;
    } else {
        return false;
    }
}

static void cmdKill(int argc, char **argv)
{
    if (argc != 2) {
        perror("kill: usage: kill {pid}\n");
        iiiExit(1);
    }
    pid_t pid = atoi(argv[1]);
    if (iiiKill(pid, SIG_KILL) == -1) {
        fprintf(stderr, "pid %d not found\n", pid);
        iiiExit(-1);
    }
    iiiExit(0);
}

void cmdPs(int argc, char **argv)
{
    for (int i = 0; i < PRI_N; i++) { 
        uint32_t basepri = __get_BASEPRI();
        __set_BASEPRI(1);
        for (
            struct Thrd *thrd = thrd_heads[i]->next;
            thrd != thrd_heads[i];
            thrd = thrd->next
        ) {
            printf("%d", thrd->pid);
            printStat(thrd);
            for (int i = 0; i < thrd->argc; i++) {
                printf(" %s", thrd->argv[i]);
            }
            printf("\n");
        }
        __set_BASEPRI(basepri);
    }
    iiiExit(0);
}
