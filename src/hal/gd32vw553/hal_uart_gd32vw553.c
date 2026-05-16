#ifdef PLATFORM_GD32VW553

#include "../../new_pins.h"
#include "../../new_cfg.h"
#include "../hal_uart.h"
#include "gd32vw55x.h"
#include "gd32vw55x_it.h"
#include "uart.h"

static uint32_t uart = UART1;
static bool isInitialized;

static void uart_cb(uint32_t uart_port)
{
	if(SET == usart_interrupt_flag_get(uart, USART_INT_FLAG_RBNE))
	{
		if(RESET != usart_flag_get(uart, USART_FLAG_ORERR))
		{
			usart_flag_clear(uart, USART_FLAG_ORERR);
		}
		uint8_t c = usart_data_receive(uart);
		UART_AppendByteToReceiveRingBuffer(c);
	}
}

void HAL_UART_SendByte(byte b)
{
	while(RESET == usart_flag_get(uart, USART_FLAG_TBE));
	usart_data_transmit(uart, (uint8_t)b);
}

int HAL_UART_Init(int baud, int parity, bool hwflowc, int txOverride, int rxOverride)
{
	if(isInitialized)
	{
		uart_irq_callback_unregister(uart);
	}
	if(CFG_HasFlag(OBK_FLAG_USE_SECONDARY_UART))
	{
		uart = UART1;
	}
	else
	{
		uart = USART0;
	}
	uart_config(uart, baud, hwflowc, false, false);
	uint32_t parityb;
	switch(parity)
	{
		case 1:  parityb = USART_PM_ODD; break;
		case 2:  parityb = USART_PM_EVEN; break;
		default: parityb = USART_PM_NONE; break;
	}
	usart_disable(uart);
	usart_parity_config(uart, parityb);
	usart_enable(uart);
	uart_irq_callback_register(uart, uart_cb);
	isInitialized = true;
	return 1;
}

#endif
