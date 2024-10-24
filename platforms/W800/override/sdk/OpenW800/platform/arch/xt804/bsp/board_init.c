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
 * @file     board_init.c
 * @brief    CSI Source File for board init
 * @version  V1.0
 * @date     02. June 2017
 ******************************************************************************/
#include <stdio.h>
#include <stdint.h>
#include <csi_config.h>
#include <csi_core.h>
#include "wm_regs.h"

#define UART_TXEN_BIT			(0x40)
#define UART_RXEN_BIT			(0x80)
#define UART_PARITYEN_BIT		(0x08)
#define UART_PARITYODD_BIT		(0x10)
#define UART_BITSTOP_VAL		(0x03)                  /// 1 stop-bit; no crc; 8 data-bits

extern void set_printf_port(unsigned char port);
static void uart0Init (int bandrate)
{
	unsigned int bd;

	NVIC_DisableIRQ(UART0_IRQn);
	NVIC_ClearPendingIRQ(UART0_IRQn);

	bd = (APB_CLK/(16*bandrate) - 1)|(((APB_CLK%(bandrate*16))*16/(bandrate*16))<<16);
	tls_reg_write32(HR_UART0_BAUD_RATE_CTRL, bd);

	tls_reg_write32(HR_UART0_LINE_CTRL, UART_BITSTOP_VAL | UART_TXEN_BIT | UART_RXEN_BIT);
	tls_reg_write32(HR_UART0_FLOW_CTRL, 0x00);   			/* Disable afc */
	tls_reg_write32(HR_UART0_DMA_CTRL, 0x00);             		/* Disable DMA */
	tls_reg_write32(HR_UART0_FIFO_CTRL, 0x00);             		/* one byte TX/RX */
//	tls_reg_write32(HR_UART0_INT_MASK, 0x00);             		/* Disable INT */

}
#if 0
static void uart1_io_init(void)
{
    uint32_t temp;

    /* PB6.7 AF Close */
	temp = tls_reg_read32(HR_GPIOB_AFSEL);
	temp &= ~0xC0;
	tls_reg_write32(HR_GPIOB_AFSEL, temp);

    /* PB6.7 AF Open opt1 */
    temp = tls_reg_read32(HR_GPIOB_AFSEL);
    temp |= 0xC0;
    tls_reg_write32(HR_GPIOB_AFSEL, temp);

    temp = tls_reg_read32(HR_GPIOB_AFS0);
    temp &= ~0xC0;
    tls_reg_write32(HR_GPIOB_AFS0, temp);

    temp = tls_reg_read32(HR_GPIOB_AFS1);
    temp &= ~0xC0;
    tls_reg_write32(HR_GPIOB_AFS1, temp);

}
static void uart1Init (int bandrate)
{
	unsigned int bd;

	NVIC_DisableIRQ(UART1_IRQn);
	NVIC_ClearPendingIRQ(UART1_IRQn);

	bd = (APB_CLK/(16*bandrate) - 1)|(((APB_CLK%(bandrate*16))*16/(bandrate*16))<<16);
	tls_reg_write32(HR_UART1_BAUD_RATE_CTRL, bd);

	tls_reg_write32(HR_UART1_LINE_CTRL, UART_BITSTOP_VAL | UART_TXEN_BIT | UART_RXEN_BIT);
	tls_reg_write32(HR_UART1_FLOW_CTRL, 0x00);   			/* Disable afc */
	tls_reg_write32(HR_UART1_DMA_CTRL, 0x00);             		/* Disable DMA */
	tls_reg_write32(HR_UART1_FIFO_CTRL, 0x00);             		/* one byte TX/RX */
	tls_reg_write32(HR_UART1_INT_MASK, 0x00);             		/* Disable INT */

}
#endif
void board_init(void)
{

#if USE_UART0_PRINT
    /* use uart0 as log output io */
    uart0Init(115200);
	set_printf_port(0);
#else
    uart1_io_init();
    /* use uart1 as log output io */
	uart1Init(115200);
	set_printf_port(1);	
#endif
}
