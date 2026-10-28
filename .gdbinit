file main.elf
target remote localhost:3333
monitor reset
load

monitor reset halt
maintenance flush register-cache

b HardFault_Handler
commands
    printSCB
end
b executor.S:28

source tool/ps.py

define printSCB
    set $SCB_base = 0xe000ed00
    set $CFSR_offset  = 0x28
    set $HFSR_offset  = 0x2c
    set $MMFAR_offset = 0x34
    set $BFAR_offset  = 0x38
    echo SCB_CFSR:
    x/4bt $SCB_base + $CFSR_offset
    echo SCB_HFSR:
    x/4bt $SCB_base + $HFSR_offset
    echo SCB_MMFAR:
    x/4bt $SCB_base + $MMFAR_offset
    echo SCB_BFAR:
    x/4bt $SCB_base + $BFAR_offset
end

define frmPrint
    p *(struct Frm *)(findThrdPid($arg0)->psp)
end
