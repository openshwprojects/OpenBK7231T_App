/*
 * Copyright (C) 2017 C-SKY Microsystems Co., Ltd. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
 /******************************************************************************
 * @file     startup.S
 * @brief    startup file for smartl. Should use with
 *           GCC for CSKY Embedded Processors
 * @version  V1.0
 * @date     02. June 2017
 ******************************************************************************/

#include <csi_config.h>

.section .vectors
    .align  10
    .globl  __Vectors
    .type   __Vectors, @object
__Vectors:
    .long   Reset_Handler
    .long   Default_Handler
    .long   Default_Handler
    .long   Default_Handler
    .long   Default_Handler
    .long   Default_Handler
    .long   Default_Handler
    .long   Default_Handler
    .long   Default_Handler
    .long   Default_Handler
    .long   Default_Handler
    .long   Default_Handler
    .long   Default_Handler
    .long   Default_Handler
    .long   Default_Handler
    .long   Default_Handler
    .long   Default_Handler
    .long   Default_Handler
    .long   Default_Handler
    .long   Default_Handler
    .long   Default_Handler
    .long   Default_Handler
    .long   tspend_handler
    .long   Default_Handler
    .long   Default_Handler
    .long   Default_Handler
    .long   Default_Handler
    .long   Default_Handler
    .long   Default_Handler
    .long   Default_Handler
    .long   Default_Handler
    .long   Default_Handler

    /* External interrupts */
    .long   SDIO_IRQHandler            /*  0:  SDIO         */
    .long   MAC_IRQHandler             /*  1:  MAC          */
    .long   RF_Cfg_IRQHandler          /*  2:  RF Cfg       */
    .long   SEC_IRQHandler             /*  3:  SEC          */
    .long   DMA_Channel0_IRQHandler         /*  4:  DMA_Channel0       */
    .long   DMA_Channel1_IRQHandler         /*  5:  DMA_Channel1       */
    .long   DMA_Channel2_IRQHandler         /*  6:  DMA_Channel2       */
    .long   DMA_Channel3_IRQHandler         /*  7:  DMA_Channel3       */
    .long   DMA_Channel4_7_IRQHandler       /*  8:  DMA_Channel4_7     */
    .long   DMA_BRUST_IRQHandler            /*  9:  DMA_BRUST          */
    .long   I2C_IRQHandler            /*  10: IIC         */
    .long   ADC_IRQHandler            /*  11: SD ADC      */
    .long   SPI_LS_IRQHandler         /*  12: LS SPI      */
    .long   SPI_HS_IRQHandler         /*  13: HS SPI      */
    .long   GPIOA_IRQHandler          /*  14: GPIOA       */
    .long   GPIOB_IRQHandler          /*  15: GPIOB       */
    .long   USART_IRQHandler          /*  16: UART0       */
    .long   USART1_IRQHandler         /*  17: UART1       */
    .long   USART2_IRQHandler         /*  18: UART2&7816  */
    .long   USART3_5_IRQHandler       /*  19: USART3_5    */
    .long   BLE_IRQHandler            /*  20: BLE         */
    .long   BT_IRQHandler             /*  21: BT          */
    .long   PWM_IRQHandler            /*  22: PWM         */
    .long   I2S_IRQHandler            /*  23: I2S         */
    .long   SDIO_HOST_IRQHandler      /*  24: SDIO HOST   */
    .long   CORET_IRQHandler        /*  25:  CoreTIM      */
    .long   RSA_IRQHandler          /*  26:  RSA          */
    .long   GPSEC_IRQHandler        /*  27:  GPSEC        */
    .long   FLASH_IRQHandler        /*  28:  Flash        */
    .long   PMU_IRQHandler          /*  29:  PMU          */
    .long   TIM0_5_IRQHandler       /*  30:  Timer0_5     */
    .long   WDG_IRQHandler          /*  31:  Watch dog    */

    .size   __Vectors, . - __Vectors

    .text
    .align  2
_start:
    .text
    .align  2
    .globl  Reset_Handler
    .type   Reset_Handler, %function
Reset_Handler:
#ifdef CONFIG_KERNEL_NONE
    lrw     r0, 0xe0000200
#else
    lrw     r0, 0x80000200
    mtcr    r0, psr
#endif
    mtcr    r0, psr

    lrw     r0, g_top_irqstack
    mov     sp, r0

/*
 *	move __Vectors to irq_vectors
 */
    lrw     r1, __Vectors
    lrw     r2, __vdata_start__
    lrw     r3, __vdata_end__

    subu    r3, r2
    cmpnei  r3, 0
    bf      .L_loopv0_done

.L_loopv0:
    ldw     r0, (r1, 0)
    stw     r0, (r2, 0)
    addi    r1, 4
    addi    r2, 4
    subi    r3, 4
    cmpnei  r3, 0
    bt      .L_loopv0

.L_loopv0_done:

/*
 *  The ranges of copy from/to are specified by following symbols
 *    __etext: LMA of start of the section to copy from. Usually end of text
 *    __data_start__: VMA of start of the section to copy to
 *    __data_end__: VMA of end of the section to copy to
 *
 *  All addresses must be aligned to 4 bytes boundary.
 */
    lrw     r1, __erodata
    lrw     r2, __data_start__
    lrw     r3, __data_end__

    subu    r3, r2
    cmpnei  r3, 0
    bf      .L_loop0_done

.L_loop0:
    ldw     r0, (r1, 0)
    stw     r0, (r2, 0)
    addi    r1, 4
    addi    r2, 4
    subi    r3, 4
    cmpnei  r3, 0
    bt      .L_loop0

.L_loop0_done:

/*
 *  The BSS section is specified by following symbols
 *    __bss_start__: start of the BSS section.
 *    __bss_end__: end of the BSS section.
 *
 *  Both addresses must be aligned to 4 bytes boundary.
 */
 #if 1
    lrw     r1, __bss_start__
    lrw     r2, __bss_end__

    movi    r0, 0

    subu    r2, r1
    cmpnei  r2, 0
    bf      .L_loop1_done

.L_loop1:
    stw     r0, (r1, 0)
    addi    r1, 4
    subi    r2, 4
    cmpnei  r2, 0
    bt      .L_loop1
.L_loop1_done:
#endif

#ifndef __NO_SYSTEM_INIT
    jbsr    SystemInit
#endif

#ifndef __NO_BOARD_INIT
    jbsr    board_init
#endif

    jbsr    main
    .size   Reset_Handler, . - Reset_Handler

__exit:
    br      __exit

.section .bss

    .align  2
    .globl  g_intstackalloc
    .global g_intstackbase
    .global g_top_irqstack
g_intstackalloc:
g_intstackbase:
    .space CONFIG_ARCH_INTERRUPTSTACK
g_top_irqstack:

.section .vdata
	.align  10
    .globl  irq_vectors
    .type   irq_vectors, @object
irq_vectors:
	.space CONFIG_IRQ_VECTOR_SIZE
	.size   irq_vectors, . - irq_vectors

    .globl  irq_vectors_end
irq_vectors_end:
