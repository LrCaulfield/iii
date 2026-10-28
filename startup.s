.syntax unified
.cpu cortex-m3
.thumb
.fpu softvfp
.equ  BootRAM, 0xF108F85F

.global g_pfnVectors


.section .text
.type Reset_Handler, %function
Reset_Handler:
    ldr r0, =_sidata
    ldr r1, =_sdata
    ldr r2, =_edata

carry_data_loop:
    cmp r1, r2
    bge carry_data_done
    ldr r3, [r0, #0]
    str r3, [r1, #0]
    add r0, #4
    add r1, #4
    b carry_data_loop
carry_data_done:

    ldr r0, =_sbss
    ldr r1, =_ebss
    mov r2, #0
clear_bss_loop:
    cmp r0, r1
    bge clear_bss_done
    str r2, [r0, #0]
    add r0, #4
    b clear_bss_loop
clear_bss_done:
    
    bl SystemInit
    bl iiiInit

    mrs r0, msp
    msr psp, r0
    mov r0, #0b10
    msr control, r0

    /* set SysTick_CTRL_TICKINT */
    ldr r0, =0xe000e010 // SysTick_CTRL
    ldr r1, [r0, #0]
    mov r2, #0b10
    orr r1, r1, r2
    str r1, [r0, #0]

infinit_loop:
    b infinit_loop
.size Reset_Handler, . - Reset_Handler

.type PendSV_Handler, %function
PendSV_Handler:
    ldr r1, =schel
    b trapper
.size PendSV_Handler, . - PendSV_Handler

.type SVC_Handler, %function
SVC_Handler:
    ldr r1, = distr
    b trapper
.size SVC_Handler, . - SVC_Handler

trapper:
    push {lr}
    mrs r0, psp
    stmdb r0!, {r4-r11}
    blx r1
    ldmia r0!, {r4-r11}
    msr psp, r0
    pop {pc}
.size trapper, . - trapper


.type Default_Handler, %function
Default_Handler:
    b Default_Handler
.size Default_Handler, . - Default_Handler

.section .isr_vector, "a", %progbits
.type isr_vector, %object
g_pfnVectors:
    .word _estack
    .word Reset_Handler
    .word NMI_Handler
    .word HardFault_Handler
    .word MemManage_Handler
    .word BusFault_Handler
    .word UsageFault_Handler
    .word 0
    .word 0
    .word 0
    .word 0
    .word SVC_Handler
    .word DebugMon_Handler
    .word 0
    .word PendSV_Handler
    .word SysTick_Handler
    .word WWDG_IRQHandler
    .word PVD_IRQHandler
    .word TAMPER_IRQHandler
    .word RTC_IRQHandler
    .word FLASH_IRQHandler
    .word RCC_IRQHandler
    .word EXTI0_IRQHandler
    .word EXTI1_IRQHandler
    .word EXTI2_IRQHandler
    .word EXTI3_IRQHandler
    .word EXTI4_IRQHandler
    .word DMA1_Channel1_IRQHandler
    .word DMA1_Channel2_IRQHandler
    .word DMA1_Channel3_IRQHandler
    .word DMA1_Channel4_IRQHandler
    .word DMA1_Channel5_IRQHandler
    .word DMA1_Channel6_IRQHandler
    .word DMA1_Channel7_IRQHandler
    .word ADC1_2_IRQHandler
    .word USB_HP_CAN1_TX_IRQHandler
    .word USB_LP_CAN1_RX0_IRQHandler
    .word CAN1_RX1_IRQHandler
    .word CAN1_SCE_IRQHandler
    .word EXTI9_5_IRQHandler
    .word TIM1_BRK_IRQHandler
    .word TIM1_UP_IRQHandler
    .word TIM1_TRG_COM_IRQHandler
    .word TIM1_CC_IRQHandler
    .word TIM2_IRQHandler
    .word TIM3_IRQHandler
    .word 0
    .word I2C1_EV_IRQHandler
    .word I2C1_ER_IRQHandler
    .word 0
    .word 0
    .word SPI1_IRQHandler
    .word 0
    .word USART1_IRQHandler
    .word USART2_IRQHandler
    .word 0
    .word EXTI15_10_IRQHandler
    .word RTC_Alarm_IRQHandler
    .word USBWakeUp_IRQHandler
    .word 0
    .word 0
    .word 0
    .word 0
    .word 0
    .word 0
    .word 0
    .word BootRAM        /* @0x108. This is for boot in RAM mode for
                          STM32F10x Low Density devices.*/

    .weak NMI_Handler
    .thumb_set NMI_Handler,Default_Handler

    .weak HardFault_Handler
    .thumb_set HardFault_Handler,Default_Handler

    .weak MemManage_Handler
    .thumb_set MemManage_Handler,Default_Handler

    .weak BusFault_Handler
    .thumb_set BusFault_Handler,Default_Handler

    .weak UsageFault_Handler
    .thumb_set UsageFault_Handler,Default_Handler

    .weak DebugMon_Handler
    .thumb_set DebugMon_Handler,Default_Handler

    .weak SysTick_Handler
    .thumb_set SysTick_Handler,Default_Handler

    .weak WWDG_IRQHandler
    .thumb_set WWDG_IRQHandler,Default_Handler

    .weak PVD_IRQHandler
    .thumb_set PVD_IRQHandler,Default_Handler

    .weak TAMPER_IRQHandler
    .thumb_set TAMPER_IRQHandler,Default_Handler

    .weak RTC_IRQHandler
    .thumb_set RTC_IRQHandler,Default_Handler

    .weak FLASH_IRQHandler
    .thumb_set FLASH_IRQHandler,Default_Handler

    .weak RCC_IRQHandler
    .thumb_set RCC_IRQHandler,Default_Handler

    .weak EXTI0_IRQHandler
    .thumb_set EXTI0_IRQHandler,Default_Handler

    .weak EXTI1_IRQHandler
    .thumb_set EXTI1_IRQHandler,Default_Handler

    .weak EXTI2_IRQHandler
    .thumb_set EXTI2_IRQHandler,Default_Handler

    .weak EXTI3_IRQHandler
    .thumb_set EXTI3_IRQHandler,Default_Handler

    .weak EXTI4_IRQHandler
    .thumb_set EXTI4_IRQHandler,Default_Handler

    .weak DMA1_Channel1_IRQHandler
    .thumb_set DMA1_Channel1_IRQHandler,Default_Handler

    .weak DMA1_Channel2_IRQHandler
    .thumb_set DMA1_Channel2_IRQHandler,Default_Handler

    .weak DMA1_Channel3_IRQHandler
    .thumb_set DMA1_Channel3_IRQHandler,Default_Handler

    .weak DMA1_Channel4_IRQHandler
    .thumb_set DMA1_Channel4_IRQHandler,Default_Handler

    .weak DMA1_Channel5_IRQHandler
    .thumb_set DMA1_Channel5_IRQHandler,Default_Handler

    .weak DMA1_Channel6_IRQHandler
    .thumb_set DMA1_Channel6_IRQHandler,Default_Handler

    .weak DMA1_Channel7_IRQHandler
    .thumb_set DMA1_Channel7_IRQHandler,Default_Handler

    .weak ADC1_2_IRQHandler
    .thumb_set ADC1_2_IRQHandler,Default_Handler

    .weak USB_HP_CAN1_TX_IRQHandler
    .thumb_set USB_HP_CAN1_TX_IRQHandler,Default_Handler

    .weak USB_LP_CAN1_RX0_IRQHandler
    .thumb_set USB_LP_CAN1_RX0_IRQHandler,Default_Handler

    .weak CAN1_RX1_IRQHandler
    .thumb_set CAN1_RX1_IRQHandler,Default_Handler

    .weak CAN1_SCE_IRQHandler
    .thumb_set CAN1_SCE_IRQHandler,Default_Handler

    .weak EXTI9_5_IRQHandler
    .thumb_set EXTI9_5_IRQHandler,Default_Handler

    .weak TIM1_BRK_IRQHandler
    .thumb_set TIM1_BRK_IRQHandler,Default_Handler

    .weak TIM1_UP_IRQHandler
    .thumb_set TIM1_UP_IRQHandler,Default_Handler

    .weak TIM1_TRG_COM_IRQHandler
    .thumb_set TIM1_TRG_COM_IRQHandler,Default_Handler

    .weak TIM1_CC_IRQHandler
    .thumb_set TIM1_CC_IRQHandler,Default_Handler

    .weak TIM2_IRQHandler
    .thumb_set TIM2_IRQHandler,Default_Handler

    .weak TIM3_IRQHandler
    .thumb_set TIM3_IRQHandler,Default_Handler

    .weak I2C1_EV_IRQHandler
    .thumb_set I2C1_EV_IRQHandler,Default_Handler

    .weak I2C1_ER_IRQHandler
    .thumb_set I2C1_ER_IRQHandler,Default_Handler

    .weak SPI1_IRQHandler
    .thumb_set SPI1_IRQHandler,Default_Handler

    .weak USART1_IRQHandler
    .thumb_set USART1_IRQHandler,Default_Handler

    .weak USART2_IRQHandler
    .thumb_set USART2_IRQHandler,Default_Handler

    .weak EXTI15_10_IRQHandler
    .thumb_set EXTI15_10_IRQHandler,Default_Handler

    .weak RTC_Alarm_IRQHandler
    .thumb_set RTC_Alarm_IRQHandler,Default_Handler

    .weak USBWakeUp_IRQHandler
    .thumb_set USBWakeUp_IRQHandler,Default_Handler

.size g_pfnVectors, . - g_pfnVectors
