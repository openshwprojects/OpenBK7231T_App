#include "../hal_uart.h"

void __attribute__((weak)) HAL_UART_SendByte(byte b)
{

}

int __attribute__((weak)) HAL_UART_Init(int baud, int parity)
{
	return 0;
}