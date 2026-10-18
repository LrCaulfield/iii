.syntax unified
.global executor
.equ Thrd_dywd, 40

.section .text
.type executor, %function
executor:
    push {r0-r4, r7, r12, lr}
    mrs r4, apsr
    ldr r0, =thrd_run
    ldr r1, =pri_run
    ldr r1, [r1, #0]
    lsl r1, 2
    ldr r0, [r0, r1]
    ldr r0, [r0, #Thrd_dywd]
    blx r0
    mov r7, #0
    svc #0
    msr apsr_nzcvq, r4
    pop {r0-r4, r7, r12, lr}
    add sp, #4
    pop {pc}
.size executor, . - executor
