class ps(gdb.Command):
    """ps [-v|--verbose] —— 打印线程列表。"""

    STAT_NAMES = {0: "RUN", 1: "BLK", 2: "ZMB", 3: "DIE"}

    def __init__(self):
        super().__init__("ps", gdb.COMMAND_USER)

    def invoke(self, arg, from_tty):
        verbose = self._parse_args(arg)
        try:
            self._dump(verbose)
        except gdb.error as e:
            gdb.write(f"ps: {e}\n")

    @staticmethod
    def _parse_args(arg):
        verbose = False
        for tok in arg.split():
            if tok in ("-v", "--verbose"):
                verbose = True
            elif tok in ("-h", "--help"):
                gdb.write("usage: ps [-v|--verbose]\n")
                raise SystemExit          # 见下方说明
            else:
                gdb.write(f"ps: unknown option '{tok}'\n")
                raise SystemExit
        return verbose

    def _dump(self, verbose):
        node_t  = gdb.lookup_type("struct Node")
        thrd_t  = gdb.lookup_type("struct Thrd")
        thrd_p  = thrd_t.pointer()

        pri_n     = int(gdb.parse_and_eval("PRI_N"))
        heads     = int(gdb.parse_and_eval("&thrd_heads"))
        cur_addr  = int(gdb.parse_and_eval("thrd_run"))

        gdb.write(f"=== Threads  thrd_run=0x{cur_addr:08x}  "
                  f"thrd_heads=0x{heads:08x} ===\n")

        for pri in range(pri_n):
            head = heads + pri * node_t.sizeof
            node = gdb.Value(head).cast(node_t.pointer())["next"]

            if int(node) == head:
                gdb.write(f"pri {pri}: (empty)\n")
                continue

            gdb.write(f"pri {pri}:\n")
            guard = 0
            while int(node) != head:
                thrd = gdb.Value(int(node)).cast(thrd_p)
                self._print_one(thrd, cur_addr, verbose)
                node = thrd["next"]
                guard += 1
                if guard > 64:
                    gdb.write("  !!! list corrupted (loop?)\n")
                    break

    def _print_one(self, thrd, cur_addr, verbose):
        mark  = "*" if int(thrd) == cur_addr else " "
        pid   = int(thrd["pid"])
        stat  = int(thrd["stat"])
        sname = self.STAT_NAMES.get(stat, f"?{stat}")

        if not verbose:
            # 精简：只 pid + 状态 + psp
            psp = int(thrd["psp"])
            gdb.write(f"{mark} pid={pid:<3d} {sname:<3s} psp=0x{psp:08x}\n")
            return

        # 详细：全部字段
        psp  = int(thrd["psp"])
        argc = int(thrd["argc"])
        wpid = int(thrd["wpid"])
        sig  = int(thrd["sig"])
        gdb.write(
            f"{mark} Thrd@0x{int(thrd):08x} pid={pid:<3d} "
            f"{sname:<3s} psp=0x{psp:08x} "
            f"sig={sig} wpid={wpid:<3d} argc={argc}\n"
        )
        # 若 verbose，还可打印父子关系
        pare = int(thrd["pare"])
        chil = int(thrd["chil"])
        gdb.write(f"    pare=0x{pare:08x} chil=0x{chil:08x}\n")


ps()
