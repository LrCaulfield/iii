# iii (Iii Is Iii) —— STM32F103C6T6 上的玩具调度器

> 自己写起耍的:)

## 简介

`iii` 是一个跑在 **STM32F103C6T6**（Cortex-M3，32 KB Flash / 10 KB SRAM）上的极简玩具调度器，实现了：

- 基于 **PendSV** 的上下文切换（SysTick 每 10 ms 挂起一次 PendSV）
- 基于 **SVC** 的系统调用分发
- 2 级优先级、最多 9 个线程的抢占式调度
- 线程操作：`iiiNew / iiiExit / iiiWait / iiiKill`
- 信号处理：`iiiDywd` 为自己立遗嘱（注册信号处理函数），`iiiKill` 杀死（发送信号）给指定线程
- 用户态 libc（`printf` / `malloc` / `strlen` / `strcmp` / `atoi` …）+ 串口 shell
- GDB Python 脚本 `ps.py`，调试时查看所有线程

## 目录结构

```
.
├── knl/                    内核
│   ├── executor.S          信号处理 trampoline
│   ├── list.c / list.h     双向链表操作
│   ├── sched.c / sched.h   调度器 + 系统初始化
│   ├── syshdlr.c / .h      SVC 分发与各系统调用处理
│   ├── thrd.c / thrd.h     线程控制块
│   ├── stm32f1xx_it.c      各异常处理（SysTick 挂起 PendSV）
│   └── system_stm32f1xx.c  芯片初始化
├── usr/                    用户空间 & 驱动
│   ├── clk.c   systick.c   时钟与时间片
│   ├── io.c    usart.c     GPIO / USART1
│   ├── nvic.c  exti.c      中断配置
│   ├── syscall.c           svc 封装 系统调用桩
│   ├── shell.c led.c test.c
│   └── delay.c
├── startup.s               复位向量、异常入口、向量表
├── STM32F103X6_FLASH.ld    链接脚本
├── debug.c                 openocd + gdb 启动器
├── telnet.c                openocd + telnet（OpenOCD 控制台）
├── tty.c                   串口终端
├── ps.py                   GDB 命令：打印线程列表
└── compile_commands.json
```

## 编译 & 烧录 & 调试

```bash
# 编译
make

# 编译并烧录（OpenOCD + ST-Link）
make flash
```

依赖：`arm-none-eabi-gcc (Fedora 15.2.0-4.fc44) 15.2.0`、`Open On-Chip Debugger 0.12.0+dev-snapshot (2026-07-11-00:00)`、`GNU Make 4.4.1`。

### 调试 & 串口

| 工具 | 作用 | 用法 |
|---|---|---|
| `debug.c` | 启动 openocd 并进入 gdb | 先编译成可执行文件再运行 |
| `telnet.c` | 启动 openocd + telnet（端口 4444） | 同上 |
| `tty.c` | 串口终端（`/dev/ttyUSB0 @ 9600`） | 同上 |
| `ps.py` | GDB 命令 `ps`，列出所有线程 | 在 gdb 里 `source ps.py` 然后 `ps` 或 `ps -v` |

`debug` 和 `telnet` 会把 openocd 的日志写到 `./log/openocd.log`（目录得先建好）。`tty` 可用 `Ctrl-C` 退出。

## 启动流程

```
Reset_Handler
  ├─ copy .data, clear .bss
  ├─ SystemInit   (clk, systick, gpio, usart, nvic...)
  ├─ iiiInit      (初始化链表头，创建 root 线程)
  ├─ PSP := MSP
  ├─ CONTROL := 0b10   (进入 PSP 模式)
  ├─ SysTick CTRL |= TICKINT
  └─ b .              (等第一次 PendSV)
```

第一次 SysTick 到来 → 挂 PendSV → `PendSV_Handler` 跳 `trapper` → `sched(psp)`。

`sched()` 第一次被调用时走 `booted == false` 分支：设置 `CONTROL = 0b11`，返回 `thrd_run->psp`。`trapper` 从 `psp` 弹 `r4-r11` 和硬件帧，直接进入第一个线程。

之后每次 PendSV/SVC 都走同一入口，只是传进去的 `psp` 不同。

## 上下文切换

STM32 进异常时硬件自动压 `r0-r3, r12, lr, pc, psr`（8 word = 32 B）。`trapper` 里再手动压 `r4-r11`（8 word），一共 16 word：

```
低地址                                  高地址
+---------+---------+---------+---------+
| r4..r11 | r0..r3  | r12,lr  | pc,psr  |  <- 用户栈
+---------+---------+---------+---------+
^
thrd->psp
```

把这个 `psp` 交给调度器，就等于把线程全部寄存器交给调度器。C 侧对应 `struct Frm`（`knl/thrd.h`）。

## 线程对象

```c
struct Thrd {
    struct Thrd *prev, *next;     // 侵入式链表
    pid_t pid;                    // 1..MAX_THRD_N-1
    void *psp;                    // 指向 Frm 起始
    void *ptr;                    // malloc 来的栈基址
    int argc; char **argv;        // 启动参数（拷在栈顶）
    int *wstat;                   // wait 接收退出码
    void (*dywd[10])(void);       // 每个信号的「遗言」
    struct Thrd *pare, *chil, *sibl;  // 进程树
    pid_t wpid;                   // 正在等谁
    enum Stat stat;               // RUN/BLK/ZMB/DIE
    enum Sig sig;                 // 待处理信号
};
```

`thrdCreat()` 用 `malloc(stk_sz)` 分配线程栈，`psp` 指向 `栈顶 - 16*4`，构造一个假帧：

```
frm->r0  = argc
frm->r1  = argv
frm->lr  = NULL
frm->pc  = func
frm->psr = 0x01000000
```

第一次调度到这个线程时，弹出 `r4-r11` + 硬件帧，等同于直接进入 `func(argc, argv)`，与「恢复一个被抢占的线程」是完全一样的操作。**通过假帧传参** 是这一设计的关键技巧。

线程栈大小由调用方决定，例如 `shell` 的栈有 `0x400` 字节。

## 调度器（`knl/sched.c`）

```c
void *sched(void *psp);
```

- 首次调用：`booted = true; __set_CONTROL(0b11); return thrd_run->psp;`
- 之后：
  1. 保存当前线程 `psp`
  2. `listExch(thrd_run->prev)` 让当前线程在链表中后退一格，避免立刻被重新选中
  3. 按优先级 `0 .. PRI_N-1` 遍历链表，取第一个 `sig != 0` 或 `stat == RUN` 的线程
  4. `DIE` 线程被立刻 `thrdDel`
  5. 找不到就死循环

线程状态变化：

```
RUN  →(wait)  → BLK
BLK  →(子进程 exit / kill 唤醒) → RUN
RUN  →(exit 但父进程还没 wait)  → ZMB
ZMB  →(父进程 wait)            → DIE
RUN  →(kill)                   → DIE（但 sig != 0，会先执行遗言）
```

## 系统调用（`knl/syshdlr.c` + `usr/syscall.c`）

用户态 `syscall()` 把 **系统调用号放 `r7`**，**参数放 `r0-r3`**，然后 `svc #0`：

```c
static uint32_t
syscall(uint32_t sys_num,
        uint32_t arg0, uint32_t arg1,
        uint32_t arg2, uint32_t arg3);
```

`SVC_Handler` 跳到 `trapper` → `distr(psp)`，`distr` 按 `frm->r7` 分发：

| # | 名字 | 原型 |
|---|---|---|
| 0 | `SYS_YIELD` | `void iiiYield(void)` |
| 1 | `SYS_NEW` | `pid_t iiiNew(void *func, char **argv, uint8_t pri, size_t stk_sz)` |
| 2 | `SYS_EXIT` | `void iiiExit(int stat)` |
| 3 | `SYS_WAIT` | `pid_t iiiWait(pid_t pid, int *wstat)` |
| 4 | `SYS_KILL` | `int iiiKill(pid_t pid, enum Sig sig)` |
| 5 | `SYS_DYWD` | `void iiiDywd(enum Sig sig, void (*func)(void))` |

内置信号目前只有 `SIG_KILL = 1`，你当然可以用任何一个小于或等于 `9` 的数作信号编号。

### 关于 `svc`

严格来说，这些系统调用**根本没必要**进中断 —— 全是用户态的线程操作，直接调 C 函数就行。用 `svc` 只是为了假把意思玩一下传参。

## `executor`：一个馊主意

`knl/executor.S` 里有一段汇编，作用是**在不破坏上下文的前提下临时跳出去执行信号处理函数**。

`sysKill`（更确切说是 `sigSend`）的步骤：

1. 把线程 `psp` 下移 8 字节，整帧 `memcpy` 下去（相当于「腾出一格」）
2. 把新帧的 `pc` 改成 `executor`，原 `pc | 1` 写到**帧末尾之后**那个槽位
3. 异常返回`pc = executor`，于是执行：

```
低地址                                                          高地址
+---------+---------+---------+---------+---------+---------+---------+
| r4..r11 | r0..r3  | r12,lr  | pc,psr  |   res.  | orig pc |   ...   |  <- 用户栈
+---------+---------+---------+---------+---------+---------+---------+
^                                   ^                   ^
thrd->psp                       psr 槽               pop {pc} 取这
（下移 8 字节）                                      （跳回原处）
```

```asm
executor:
    push {r0-r5, r7, r12, lr}
    mrs  r4, apsr                     @ 保存原线程上下文（除了 pc ，pc 存在原线程原栈下面，1会 pop 出来）
    ldr  r5, =thrd_run
    ldr  r5, [r5, #0]
    ldrb r1, [r5, #Thrd_sig]          @ 取信号号
    add  r0, r5, #Thrd_dywd
    lsl  r1, #2
    ldr  r0, [r0, r1]
    orr  r0, #1
    blx  r0                            @ 调用遗言函数

    mov  r0, #0
    strb r0, [r5, #Thrd_sig]           @ 清 sig

    mov  r7, #0
    svc  #0                            @ SYS_YIELD

    msr  apsr, r4
    pop  {r0-r5, r7, r12, lr}
    add  sp, #4                        @ 恢复原线程上下文（除了 pc）
    pop  {pc}                          @ 跳回原 pc
```

于是：

- `SIG_KILL`：线程在进 `executor` 之前就被置 `DIE`，但 `sig != 0`，所以调度器仍会选中它、执行遗言、yield 然后被回收。
- 其他信号：线程从 `pop {pc}` 回到原处继续运行。

这样 `sched` 只判断当前线程要不要跑，**不关心**当前线程是在正常跑，还是在处理信号。

## Shell 命令

`shell` 从 USART1（PA9/PA10）读命令行，`_read` / `_write` 映射到了 `iiiRead` / `iiiWrite`。

命令会开1个新线程，后面的词会通过 `argv` 传给新线程：

| 命令 | 说明 |
|---|---|
| `ps` | 列出所有线程（pid、状态、argv） |
| `kill <pid> <sig>` | 向 `pid` 发送 `sig` |
| `led on` / `led off` | 点亮 / 熄灭板载 LED（PC13） |
| `flash <ms> &` | 板载 LED 闪烁，延时 ms 毫秒（**必须加 `&`**） |
| `flashEx <ms> &` | PA0 外部 LED 闪烁，延时 ms 毫秒（**必须加 `&`**） |

`flash` / `flashEx` 是死循环，不加 `&` 的话 shell 会一直卡在 `iiiWait` 上。

## 默认根线程

`sched.c` 里的 `iiiInit()` 目前默认启动 `shell`，你当然可以改成 `test` 或你自己的程序：

```c
char *argv[2] = { "shell", NULL, };
thrd_run = thrdCreat(shell, argv, 0, 0x400);
```

更改很简单：

```c
char *argv[2] = { "yourProg", NULL, };
thrd_run = thrdCreat(yourProg, argv, 0, 0x400);
```

## 常量速查

| 名字 | 值 | 说明 |
|---|---|---|
| `PRI_N` | 2 | 优先级级数 |
| `MAX_THRD_N` | 10 | pid 上限（pid = 1..9） |
| `SIG_N` | 9 | `dywd` 数组长度（实际信号 0..9） |
| `USART_BUF_SIZE` | 0x100 | 串口环形缓冲 |
| `_Min_Heap_Size` | 0x200 | 链接脚本里的堆 |
| `_Min_Stack_Size` | 0x400 | 链接脚本里的主栈 |

## 已知问题 / TODO

- **无内存保护**：Cortex-M3 没有 MMU，用户线程能写任何 SRAM 。
- 每个线程栈大小由调用方决定，容易爆栈，测试时已经遭很多次了，以后可以做栈溢出检测和thrdRealloc之类的。
