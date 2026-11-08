#include "../../new_common.h"
#include "../hal_uart.h"
#include "uart.h"

void HAL_UART_SendByte(byte b)
{
	uart_putc((char)b);
}

int HAL_UART_Init(int baud, int parity, bool hwflowc, int txOverride, int rxOverride)
{
	(void)baud;
	(void)parity;
	(void)hwflowc;
	(void)txOverride;
	(void)rxOverride;
	uart_init();
	return 1;
}
