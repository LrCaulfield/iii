如你所见，这篇README和它所描述的程序一样烂

通过SysTick_Handler定期挂起PendSV中断，从而切换线程此时传入的yeild为0，不进入系统调用处理；


##异常调用和异常返回
STM32F103C6T6进入中断函数前会自动压入寄存器r0-r3,r12,pc,psr,lr，我们只需在中断函数中再压入r4-r11就可以了；
+-----------------+      +-----------------+      +-------+
|      stack      |      |      stack      |      | stack |
|regs|                   |regs|                   |regs|                
^                        ^                        ^
thrd1->psp             thrd2->psp             thrd3->psp
然后把psp传给调度器就能访问对应线程的寄存器组，从而读取参数、传返回值、恢复线程；


##线程对象
+---------------------------------------+
| argv | strings |        stack         |
^                             ^
thrd->ptr                     frame
thrd.c中的函数create会malloc一片空间，thrd->psp会从这片空间末尾-(18*32)开始，
因为第1次运行线程的操作和恢复线程其实一模一样，即需要弹出18word然后运行用户程序，
所以要预留18word假栈，而这上面的数值是会弹进寄存器的，所以可以用这个假栈传参数给新线程，
如：
uint32_t *frame = thrd->psp;
frame[8] = 114514; // 这1word到时会弹进r0，从而使新线程的第1个参数是114514（EABI）

    <<----------- 8 byte
+---------------------------------------+
|          regs           | res  |  pc  |
|executor|
^
thrd->psp
kill的实现很有意思，它用knl/executor.S中的函数executor包装dywd，
sysKill会把线程的寄存器栈下移8字节，在后面填上原线程被打断处，即原thrd_kill->pc，
然后把thrd_kill->pc改成executor以执行dywd，
executor最后的add sp, #4; pop {pc}会跳回原线程，以后kill就可以改成发送信号；
这样sysKill只管使线程返回executor，schel完全不管当前线程是在正常运行还是处理信号


##系统调用
svc指令会触发SVCcall中断，这个中断处理函数会调用schel并传入yeild = 1，
从而进入distr（系统处理分发函数）； syshdlr.c
这个指令集的系统调用号是立即数，如：svc #1，系统处理函数可通过pc找到这个立即数；
传参类似函数；

usr/syscall.c封装了svc指令及系统调用号，
在调用这些函数时，编译器会按ABI填寄存器并svc，从而将参数保留在用户栈中，
系统处理函数会用psp从栈中访问参数；

iiiYield   挂起PendSV中断以切出当前线程
iiiNew     创建新线程
iiiWaitpid 等指定pid的线程退出
iiiExit    退出
iiiKill    杀死指定pid的线程并执行遗言函数
iiiDywd    立遗嘱


##使用
编译用arm-none-eabi-gcc，下载、调试用openocd；
启动完成后，schel会创建第1个线程shell， 你当然也可以改成自己的程序，
cmd用USART1（PA9，PA10）输入输出，这是_read、_write决定的，
有5个命令可用：
    kill    杀死指定pid的线程
    ps      显示当前所有线程
    flash   闪板载LED（命令后一定要加“&”，
        因为这个内置程序会无限循环）
    flashEx 闪PA0接的LED（同样要加“&”）
    led     控制板载LED亮灭
命令本身其实就是创建一个线程；
修改程序后在项目根目录用make编译，make flash编译并下载；
tool/debug启动GDB调试；
tool/tty是一个小串口工具，会一直读/dev/ttyUSB0并将内容写到标准输出，同时读取标准输入并将内容写到/dev/ttyUSB0

##闲聊
主要的就是knl/schel.c knl/syshdlr.c knl/thrd.c，复位后会从Reset_Handler开始，做一些初始化后就会开PendSV中断，进schel，
第1次进schel会创建shell并运行。系统调用用svc指令触发SVC中断，SVC_Handler会调用distr调用对应处理函数，如需要处理完后切换线程的调用会挂PendSV中断，出来后立即进schel调度。
这里的系统调用只能做线程操作，根本没必要进中断，这样做只是假把意思玩1下传参
我觉得最好玩的就是上下文切换和通过用户栈传参，还有就是executor执行dywd这个馊主意:)，内存映射应该也好玩，这个机器没有内存保护，没玩成
executor这个方法基本上就是：sysKill把原寄存器栈下移了8字节，并在后面添上原来的pc，
把现在的pc改成executor，这样异常返回就会回到executor，最后的add ps, #4; pop {pc}就会返回原线程，并保证了上下文；mov r7, #0; svc #0是做yield，
因为对于真正被杀死的线程，在进executor之前就被sysKill置DIE了，只是因为有is_homicide才能最后1舞，再进schel就会彻底delThrd，对于后面要加的信号功能，不真正死掉的线程自然能从psp {pc}返回原线程
