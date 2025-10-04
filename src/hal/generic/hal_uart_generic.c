#include "../hal_uart.h"

void __attribute__((weak)) HAL_UART_SendByte(byte b)
{

}
int __attribute__((weak)) HAL_UART_Init(int baud, int parity, bool hwflowc, int txOverride, int rxOverride)
{
	return 0;
}
void __attribute__((weak)) HAL_UART_Flush()
{
}
void __attribute__((weak)) HAL_SetBaud(unsigned int baud)
{
}


