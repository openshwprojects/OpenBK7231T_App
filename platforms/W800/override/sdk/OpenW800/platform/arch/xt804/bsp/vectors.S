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
 * @file     vectors.S
 * @brief    define default vector handlers. Should use with
 *           GCC for CSKY Embedded Processors
 * @version  V1.0
 * @date     28. Nove 2017
 ******************************************************************************/

#include <csi_config.h>

.import trap_c

.section .bss
    .align  2
    .globl  g_trapstackalloc
    .global g_trapstackbase
    .global g_top_trapstack
g_trapstackalloc:
g_trapstackbase:
    .space 512
g_top_trapstack:

    .align 2
    .globl g_trap_sp
    .type  g_trap_sp, object
g_trap_sp:
    .long 0
    .size g_trap_sp, .-g_trap_sp

.text

/******************************************************************************
 * Functions:
 *     void trap(void);
 * default exception handler
 ******************************************************************************/
    .global trap
    .type   trap, %function
trap:
    psrset  ee

    subi    sp, 4
    stw     r13, (sp)
    addi    sp, 4

    lrw     r13, g_trap_sp
    stw     sp, (r13)

    lrw     sp, g_top_trapstack

    subi    sp, 72
    stm     r0-r12, (sp)

    lrw     r0, g_trap_sp
    ldw     r0, (r0)

    stw     r0, (sp, 56) /* save r14 */

    subi    r0, 4
    ldw     r13, (r0)
    stw     r13, (sp, 52)

    stw     r15, (sp, 60)
    mfcr    r0, epsr
    stw     r0, (sp, 64)
    mfcr    r0, epc
    stw     r0, (sp, 68)

    mov     r0, sp

    jbsr    trap_c

    .align  2
    .weak   Default_Handler
    .type   Default_Handler, %function
Default_Handler:
    br      trap
    .size   Default_Handler, . - Default_Handler

/*    Macro to define default handlers. Default handler
 *    will be weak symbol and just dead loops. They can be
 *    overwritten by other handlers */
    .macro  def_irq_handler handler_name
    .weak   \handler_name
    .globl  \handler_name
    .set    \handler_name, Default_Handler
    .endm

    def_irq_handler tspend_handler
    def_irq_handler SDIO_IRQHandler            /*  0:  SDIO         */
    def_irq_handler MAC_IRQHandler             /*  1:  MAC          */
    def_irq_handler RF_Cfg_IRQHandler          /*  2:  RF Cfg       */
    def_irq_handler SEC_IRQHandler             /*  3:  SEC          */
    def_irq_handler DMA_Channel0_IRQHandler         /*  4:  DMA_Channel0       */
    def_irq_handler DMA_Channel1_IRQHandler         /*  5:  DMA_Channel1       */
    def_irq_handler DMA_Channel2_IRQHandler         /*  6:  DMA_Channel2       */
    def_irq_handler DMA_Channel3_IRQHandler         /*  7:  DMA_Channel3       */
    def_irq_handler DMA_Channel4_7_IRQHandler       /*  8:  DMA_Channel4_7     */
    def_irq_handler DMA_BRUST_IRQHandler            /*  9:  DMA_BRUST          */
    def_irq_handler I2C_IRQHandler            /*  10: IIC         */
    def_irq_handler ADC_IRQHandler            /*  11: SD ADC      */
    def_irq_handler SPI_LS_IRQHandler         /*  12: LS SPI      */
    def_irq_handler SPI_HS_IRQHandler         /*  13: HS SPI      */
    def_irq_handler GPIOA_IRQHandler          /*  14: GPIOA       */
    def_irq_handler GPIOB_IRQHandler          /*  15: GPIOB       */
    def_irq_handler USART_IRQHandler          /*  16: UART0       */
    def_irq_handler USART1_IRQHandler         /*  17: UART1       */
    def_irq_handler USART2_IRQHandler         /*  18: UART2&7816  */
    def_irq_handler USART3_5_IRQHandler       /*  19: USART3_5    */
    def_irq_handler BLE_IRQHandler            /*  20: BLE         */
    def_irq_handler BT_IRQHandler             /*  21: BT          */
    def_irq_handler PWM_IRQHandler            /*  22: PWM         */
    def_irq_handler I2S_IRQHandler            /*  23: I2S         */
    def_irq_handler SDIO_HOST_IRQHandler      /*  24: SDIO HOST   */
    def_irq_handler CORET_IRQHandler        /*  25:  CoreTIM      */
    def_irq_handler RSA_IRQHandler          /*  26:  RSA          */
    def_irq_handler GPSEC_IRQHandler        /*  27:  GPSEC        */
    def_irq_handler FLASH_IRQHandler        /*  28:  Flash        */
    def_irq_handler PMU_IRQHandler          /*  29:  PMU          */
    def_irq_handler TIM0_5_IRQHandler       /*  30:  Timer0_5     */
    def_irq_handler WDG_IRQHandler          /*  31:  Watch dog    */
