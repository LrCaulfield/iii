#pragma once

#define SIG_N 9

enum SysNum {
    SYS_YIELD,
    SYS_NEW,
    SYS_EXIT,
    SYS_WAIT,
    SYS_KILL,
    SYS_DYWD,
} ;

enum Sig {
    SIG_NO = 0,
    SIG_KILL,
};

void *distr(void *psp);
