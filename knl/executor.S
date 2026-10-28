.syntax unified
.global executor
.equ Thrd_dywd, 32
.equ Thrd_sig, 89

.section .text
.type executor, %function
executor:
    push {r0-r5, r7, r12, lr}
    mrs r4, apsr
    ldr r5, =thrd_run
    ldr r5, [r5, #0]
    ldrb r1, [r5, #Thrd_sig]
    add r0, r5, #Thrd_dywd
    lsl r1, #2
    ldr r0, [r0, r1]
    orr r0, #1
    blx r0

    mov r0, #0
    strb r0, [r5, #Thrd_sig]

    mov r7, #0
    svc #0
    msr apsr, r4
    pop {r0-r5, r7, r12, lr}
    add sp, #4
    pop {pc}
.size executor, . - executor
