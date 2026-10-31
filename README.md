# iii (Iii Is Iii) — A Toy Scheduler on STM32F103C6T6

> just write for self-playing:)

## Introduction

`iii` is a minimal toy scheduler running on **STM32F103C6T6** (Cortex-M3, 32 KB Flash / 10 KB SRAM). It implements:

- Context switching based on **PendSV** (SysTick pends PendSV every 10 ms)
- System call dispatch based on **SVC**
- Preemptive scheduling with 2 priority levels and up to 9 threads
- thread operations: `iiiNew / iiiExit / iiiWait / iiiKill`
- Signal handling: `iiiDywd` lets you write your own "last words" (register a signal handler); `iiiKill` kills (sends a signal to) the specified thread
- User-space libc (`printf` / `malloc` / `strlen` / `strcmp` / `atoi` …) + serial shell
- GDB Python script `ps.py` to inspect all threads during debugging

## Directory Structure

```
.
├── knl/                    Kernel
│   ├── executor.S          Signal-handling trampoline
│   ├── list.c / list.h     Doubly linked list operations
│   ├── sched.c / sched.h   Scheduler + system initialization
│   ├── syshdlr.c / .h      SVC dispatch and system call handlers
│   ├── thrd.c / thrd.h     Thread control block
│   ├── stm32f1xx_it.c      Exception handlers (SysTick pends PendSV)
│   └── system_stm32f1xx.c  Chip initialization
├── usr/                    User space & drivers
│   ├── clk.c   systick.c   Clock and time slice
│   ├── io.c    usart.c     GPIO / USART1
│   ├── nvic.c  exti.c      Interrupt configuration
│   ├── syscall.c           SVC wrapper / system call stubs
│   ├── shell.c led.c test.c
│   └── delay.c
├── startup.s               Reset vector, exception entries, vector table
├── STM32F103X6_FLASH.ld    Linker script
├── debug.c                 openocd + gdb launcher
├── telnet.c                openocd + telnet (OpenOCD console)
├── tty.c                   Serial terminal
├── ps.py                   GDB command: print thread list
└── compile_commands.json
```

## Build & Flash & Debug

```bash
# Build
make

# Build and flash (OpenOCD + ST-Link)
make flash
```

Dependencies: `arm-none-eabi-gcc (Fedora 15.2.0-4.fc44) 15.2.0`, `Open On-Chip Debugger 0.12.0+dev-snapshot (2026-07-11-00:00)`, `GNU Make 4.4.1`.

### Debugging & Serial

| Tool | Purpose | Usage |
|---|---|---|
| `debug.c` | Start openocd and enter gdb | Compile to an executable and run |
| `telnet.c` | Start openocd + telnet (port 4444) | Same as above |
| `tty.c` | Serial terminal (`/dev/ttyUSB0 @ 9600`) | Same as above |
| `ps.py` | GDB command `ps`, list all threads | Inside gdb: `source ps.py`, then `ps` or `ps -v` |

`debug` and `telnet` write openocd logs to `./log/openocd.log` (create the directory first). `tty` can be exited with `Ctrl-C`.

## Boot Flow

```
Reset_Handler
  ├─ copy .data, clear .bss
  ├─ SystemInit   (clk, systick, gpio, usart, nvic...)
  ├─ iiiInit      (initialize list heads, create root thread)
  ├─ PSP := MSP
  ├─ CONTROL := 0b10   (switch to PSP mode)
  ├─ SysTick CTRL |= TICKINT
  └─ b .              (wait for the first PendSV)
```

First SysTick → pend PendSV → `PendSV_Handler` branches to `trapper` → `sched(psp)`.

The first time `sched()` is called, it takes the `booted == false` branch: sets `CONTROL = 0b11` and returns `thrd_run->psp`. `trapper` pops `r4-r11` and the hardware frame from `psp`, entering the first thread directly.

Every subsequent PendSV/SVC goes through the same entry, only with a different `psp`.

## Context Switching

On exception entry, STM32 automatically pushes `r0-r3, r12, lr, pc, psr` (8 words = 32 B). `trapper` manually pushes `r4-r11` (8 words), totaling 16 words:

```
Low address                             High address
+---------+---------+---------+---------+
| r4..r11 | r0..r3  | r12,lr  | pc,psr  |  <- user stack
+---------+---------+---------+---------+
^
thrd->psp
```

Handing this `psp` to the scheduler is equivalent to handing over all registers of the thread. The C-side counterpart is `struct Frm` (`knl/thrd.h`).

## Thread Object

```c
struct Thrd {
    struct Thrd *prev, *next;     // intrusive linked list
    pid_t pid;                    // 1..MAX_THRD_N-1
    void *psp;                    // points to start of Frm
    void *ptr;                    // stack base from malloc
    int argc; char **argv;        // startup args (copied at top of stack)
    int *wstat;                   // wait receives exit status here
    void (*dywd[10])(void);       // "last words" for each signal
    struct Thrd *pare, *chil, *sibl;  // process tree
    pid_t wpid;                   // who we are waiting for
    enum Stat stat;               // RUN/BLK/ZMB/DIE
    enum Sig sig;                 // pending signal
};
```

`thrdCreat()` allocates the thread stack with `malloc(stk_sz)`, sets `psp` to `stack_top - 16*4`, and builds a fake frame:

```
frm->r0  = argc
frm->r1  = argv
frm->lr  = NULL
frm->pc  = func
frm->psr = 0x01000000
```

The first time this thread is scheduled, popping `r4-r11` + the hardware frame is equivalent to entering `func(argc, argv)` directly, exactly the same operation as restoring a preempted thread. **Passing arguments through the fake frame** is the key trick of this design.

The thread stack size is decided by the caller, e.g. `shell` has a `0x400`-byte stack.

## Scheduler (`knl/sched.c`)

```c
void *sched(void *psp);
```

- First call: `booted = true; __set_CONTROL(0b11); return thrd_run->psp;`
- Afterwards:
  1. Save the current thread's `psp`
  2. `listExch(thrd_run->prev)` moves the current thread back one slot in the list, avoiding immediate reselection
  3. Walk the list from priority `0 .. PRI_N-1`, pick the first thread with `sig != 0` or `stat == RUN`
  4. `DIE` threads are immediately `thrdDel`'d
  5. If none found, spin forever

Thread state transitions:

```
RUN  →(wait)  → BLK
BLK  →(child exit / kill wakes up) → RUN
RUN  →(exit but parent hasn't waited yet)  → ZMB
ZMB  →(parent wait)            → DIE
RUN  →(kill)                   → DIE (but sig != 0, so last words run first)
```

## System Calls (`knl/syshdlr.c` + `usr/syscall.c`)

User-space `syscall()` puts the **system call number in `r7`** and **arguments in `r0-r3`**, then executes `svc #0`:

```c
static uint32_t
syscall(uint32_t sys_num,
        uint32_t arg0, uint32_t arg1,
        uint32_t arg2, uint32_t arg3);
```

`SVC_Handler` branches to `trapper` → `distr(psp)`, and `distr` dispatches on `frm->r7`:

| # | Name | Prototype |
|---|---|---|
| 0 | `SYS_YIELD` | `void iiiYield(void)` |
| 1 | `SYS_NEW` | `pid_t iiiNew(void *func, char **argv, uint8_t pri, size_t stk_sz)` |
| 2 | `SYS_EXIT` | `void iiiExit(int stat)` |
| 3 | `SYS_WAIT` | `pid_t iiiWait(pid_t pid, int *wstat)` |
| 4 | `SYS_KILL` | `int iiiKill(pid_t pid, enum Sig sig)` |
| 5 | `SYS_DYWD` | `void iiiDywd(enum Sig sig, void (*func)(void))` |

The only built-in signal is `SIG_KILL = 1`. You can of course use any number ≤ `9` as a signal number.

### About `svc`

Strictly speaking, these system calls **do not need** to enter an exception at all — they are all user-space thread operations; you could just call C functions directly. Using `svc` is purely for the fun of passing arguments through the exception mechanism.

## `executor`: A Dirty Trick

`knl/executor.S` contains a piece of assembly whose job is to **temporarily jump out to run a signal handler without destroying the context**.

The steps of `sysKill` (more precisely `sigSend`):

1. Move the thread's `psp` down by 8 bytes and `memcpy` the whole frame down (i.e. "make room")
2. Set the new frame's `pc` to `executor`, and write the original `pc | 1` to the slot **right after the frame**
3. On exception return `pc = executor`, so it runs:

```
Low address                                                           High address
+---------+---------+---------+---------+---------+---------+---------+
| r4..r11 | r0..r3  | r12,lr  | pc,psr  |   res.  | orig pc |   ...   |   <- user stack
+---------+---------+---------+---------+---------+---------+---------+
^                                   ^                   ^
thrd->psp                       psr slot             pop {pc} form here
(move down 8 bytes)                               (jmp back to orig thrd)
```

```asm
executor:
    push {r0-r5, r7, r12, lr}
    mrs  r4, apsr                     @ save original context (except pc; pc is stored below the original stack and will be popped later)
    ldr  r5, =thrd_run
    ldr  r5, [r5, #0]
    ldrb r1, [r5, #Thrd_sig]          @ get signal number
    add  r0, r5, #Thrd_dywd
    lsl  r1, #2
    ldr  r0, [r0, r1]
    orr  r0, #1
    blx  r0                            @ call the last-words function

    mov  r0, #0
    strb r0, [r5, #Thrd_sig]           @ clear sig

    mov  r7, #0
    svc  #0                            @ SYS_YIELD

    msr  apsr, r4
    pop  {r0-r5, r7, r12, lr}
    add  sp, #4                        @ restore original context (except pc)
    pop  {pc}                          @ jump back to original pc
```

Thus:

- `SIG_KILL`: the thread is marked `DIE` before entering `executor`, but since `sig != 0`, the scheduler still selects it, runs the last words, yields, and then it gets reaped.
- Other signals: the thread simply returns to where it was via `pop {pc}` and continues running.

This way `sched` only decides whether the current thread should run; it **does not care** whether the thread is running normally or handling a signal.

## Shell Commands

`shell` reads command lines from USART1 (PA9/PA10); `_read` / `_write` are mapped to `iiiRead` / `iiiWrite`.

A command spawns a new thread, and the remaining words are passed to the new thread via `argv`:

| Command | Description |
|---|---|
| `ps` | List all threads (pid, state, argv) |
| `kill <pid> <sig>` | Send `sig` to `pid` |
| `led on` / `led off` | Turn on / off the on-board LED (PC13) |
| `flash <ms> &` | Blink on-board LED with delay ms ( **`&` is mandatory** ) |
| `flashEx <ms> &` | Blink external LED on PA0 with delay ms ( **`&` is mandatory** ) |

`flash` / `flashEx` are infinite loops; without `&`, the shell will block forever in `iiiWait`.

## Default Root Thread

In `sched.c`, `iiiInit()` currently starts `shell` by default. You can of course change it to `test` or your own program:

```c
char *argv[2] = { "shell", NULL, };
thrd_run = thrdCreat(shell, argv, 0, 0x400);
```

Changing it is easy:

```c
char *argv[2] = { "yourProg", NULL, };
thrd_run = thrdCreat(yourProg, argv, 0, 0x400);
```

## Constants Cheat Sheet

| Name | Value | Description |
|---|---|---|
| `PRI_N` | 2 | Number of priority levels |
| `MAX_THRD_N` | 10 | pid limit (pid = 1..9) |
| `SIG_N` | 9 | `dywd` array length (actual signals 0..9) |
| `USART_BUF_SIZE` | 0x100 | Serial ring buffer |
| `_Min_Heap_Size` | 0x200 | Heap in linker script |
| `_Min_Stack_Size` | 0x400 | Main stack in linker script |

## Known Issues / TODO

- **No memory protection**: Cortex-M3 has no MMU; user threads can write any SRAM.
- Each thread's stack size is decided by the caller, so stack overflow is easy — I've hit it many times during testing. Stack overflow detection and something like `thrdRealloc` would be nice in the future.
