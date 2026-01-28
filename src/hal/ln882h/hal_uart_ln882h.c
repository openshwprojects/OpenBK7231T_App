#ifdef PLATFORM_LN882H

#include "../../new_pins.h"
#include "../../new_cfg.h"
#include "../hal_uart.h"
#include "hal/hal_uart.h"
#include "serial.h"
#include "serial_hw.h"
#include "hal/hal_gpio.h"
#include "hal/hal_misc.h"

static bool isInitialized;

void HAL_UART_SendByte(byte b)
{
	while(!hal_uart_flag_get(UART0_BASE, USART_FLAG_TXE));
	hal_uart_send_data(UART0_BASE, b);
}

int HAL_UART_Init(int baud, int parity, bool hwflowc, int txOverride, int rxOverride)
{
	if(isInitialized)
	{
		hal_misc_reset_uart0();
		NVIC_ClearPendingIRQ(UART0_IRQn);
		NVIC_DisableIRQ(UART0_IRQn);
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

	hal_gpio_pin_afio_select(GPIOA_BASE, GPIO_PIN_2, UART0_TX);
	hal_gpio_pin_afio_select(GPIOA_BASE, GPIO_PIN_3, UART0_RX);
	hal_gpio_pin_afio_en(GPIOA_BASE, GPIO_PIN_2, HAL_ENABLE);
	hal_gpio_pin_afio_en(GPIOA_BASE, GPIO_PIN_3, HAL_ENABLE);

	hal_uart_init(UART0_BASE, &uart_init_struct);
	hal_uart_rx_mode_en(UART0_BASE, HAL_ENABLE);
	hal_uart_tx_mode_en(UART0_BASE, HAL_ENABLE);
	hal_uart_en(UART0_BASE, HAL_ENABLE);

	NVIC_SetPriority(UART0_IRQn, 4);
	NVIC_EnableIRQ(UART0_IRQn);
	hal_uart_it_en(UART0_BASE, USART_IT_RXNE);

	isInitialized = true;
	return 1;
}

void UART0_IRQHandler(void)
{
	if(isInitialized)
	{
		if(hal_uart_flag_get(UART0_BASE, USART_FLAG_RXNE) == 1 && hal_uart_it_en_status_get(UART0_BASE, USART_IT_RXNE))
		{
			UART_AppendByteToReceiveRingBuffer(hal_uart_recv_data(UART0_BASE));
		}
	}
	else
	{
		serial_uart0_isr_callback();
	}
}

#endif
