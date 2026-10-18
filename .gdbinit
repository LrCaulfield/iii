file main.elf
target remote localhost:3333
monitor reset
load

monitor reset halt
maintenance flush register-cache

define printFrm
    p *(struct Frm *)(findThrdPid($arg0)->psp)
end
    
python
import gdb

class ListThrd(gdb.Command):

    def __init__(self):
        super(ListThrd, self).__init__("listThrd", gdb.COMMAND_USER)

    def invoke(self, arg, from_tty):
        try:
            # 1. 拿到 struct Thrd * 类型
            thrd_ptr_type = gdb.lookup_type("struct Thrd").pointer()

            # 2. 获取 thrd_heads[0]（假设它是全局的 struct Thrd *）
            head = gdb.parse_and_eval("thrd_heads[0]").cast(thrd_ptr_type)
            if int(head) == 0:
                print("thrd_heads[0] is NULL")
                return

            head_addr = int(head)          # 头节点地址，作为循环终止条件
            cur = head["next"].cast(thrd_ptr_type)

            idx = 0
            while int(cur) != 0 and int(cur) != head_addr:
                pid  = int(cur["pid"])
                stat = int(cur["stat"])    # 如果是全局 thrd_stat，见下面说明
                print(f"[{pid}] {stat}")

                cur = cur["next"].cast(thrd_ptr_type)
                idx += 1

                if idx > 100000:           # 防御性保护，防止链表成环
                    print("... too many threads, break to avoid infinite loop")
                    break

        except gdb.error as e:
            print(f"Error: {e}")

ListThrd()
end

b HardFault_Handler
