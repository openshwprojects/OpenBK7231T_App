#ifdef PLATFORM_LN882H

#include "../../new_pins.h"
#include "../../new_cfg.h"
#include "../hal_uart.h"
#include "hal/hal_uart.h"
#include "serial.h"
#include "serial_hw.h"
#include "hal/hal_misc.h"
#include "hal_pinmap_ln882h.h"

static bool isInitialized;
int g_urx_pin = 3;
int g_utx_pin = 2;

void HAL_UART_SendByte(byte b)
{
	while(!hal_uart_flag_get(UART2_BASE, USART_FLAG_TXE));
	hal_uart_send_data(UART2_BASE, b);
}

int HAL_UART_Init(int baud, int parity, bool hwflowc, int txOverride, int rxOverride)
{
	if(isInitialized)
	{
		hal_misc_reset_uart2();
		NVIC_ClearPendingIRQ(UART2_IRQn);
		NVIC_DisableIRQ(UART2_IRQn);
	}

	uart_init_t_def uart_init_struct;
	uart_init_struct.baudrate = baud;
	uart_init_struct.word_len = UART_WORD_LEN_8;
	uart_init_struct.stop_bits = UART_STOP_BITS_1;
	uart_init_struct.over_sampl = UART_OVER_SAMPL_8;
	switch(parity)
	{
		case 1:  uart_init_struct.parity = UART_PARITY_ODD; break;
		case 2:  uart_init_struct.parity = UART_PARITY_EVEN; break;
		default: uart_init_struct.parity = UART_PARITY_NONE; break;
	}

	uint32_t rxbase = g_pins[g_urx_pin].base;
	uint32_t txbase = g_pins[g_utx_pin].base;
	gpio_pin_t rxpin = g_pins[g_urx_pin].pin;
	gpio_pin_t txpin = g_pins[g_utx_pin].pin;

	hal_gpio_pin_afio_select(txbase, txpin, UART2_TX);
	hal_gpio_pin_afio_select(rxbase, rxpin, UART2_RX);
	hal_gpio_pin_afio_en(txbase, txpin, HAL_ENABLE);
	hal_gpio_pin_afio_en(rxbase, rxpin, HAL_ENABLE);

	hal_uart_init(UART2_BASE, &uart_init_struct);
	hal_uart_rx_mode_en(UART2_BASE, HAL_ENABLE);
	hal_uart_tx_mode_en(UART2_BASE, HAL_ENABLE);
	hal_uart_en(UART2_BASE, HAL_ENABLE);

	NVIC_SetPriority(UART2_IRQn, 4);
	NVIC_EnableIRQ(UART2_IRQn);
	hal_uart_it_en(UART2_BASE, USART_IT_RXNE);

	isInitialized = true;
	return 1;
}

void UART2_IRQHandler(void)
{
	if(isInitialized)
	{
		if(hal_uart_flag_get(UART2_BASE, USART_FLAG_RXNE) == 1 && hal_uart_it_en_status_get(UART2_BASE, USART_IT_RXNE))
		{
			UART_AppendByteToReceiveRingBuffer(hal_uart_recv_data(UART2_BASE));
		}
	}
}

#endif
