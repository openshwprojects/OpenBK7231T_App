#ifdef PLATFORM_RTL87X0C

#include "../hal_uart.h"
#include "serial_api.h"
#include "hal_uart.h"
#define UART_TX	PA_14
#define UART_RX	PA_13

serial_t sobj;

static void uart_cb(uint32_t id, uint32_t event)
{
	if(event != RxIrq)
		return;
	serial_t* sobj = (void*)id;

	uint8_t c;
	while(hal_uart_rgetc(&sobj->uart_adp, (char*)&c))
	{
		UART_AppendByteToReceiveRingBuffer(c);
	}
}

void HAL_UART_SendByte(byte b)
{
	while(!hal_uart_writeable(&sobj.uart_adp));
	hal_uart_putc(&sobj.uart_adp, b);
}

int HAL_UART_Init(int baud, int parity)
{
	serial_init(&sobj, UART_TX, UART_RX);
	serial_baud(&sobj, baud);
	serial_format(&sobj, 8, parity, 1);
	serial_irq_handler(&sobj, uart_cb, (uint32_t)&sobj);
	serial_irq_set(&sobj, RxIrq, 1);
	return 1;
}

#endif