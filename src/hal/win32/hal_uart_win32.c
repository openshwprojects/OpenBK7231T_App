#ifdef WINDOWS

#include "../hal_uart.h"

void HAL_UART_SendByte(byte b)
{
	void SIM_AppendUARTByte(byte b);
	// STUB - for testing
	SIM_AppendUARTByte(b);
#if 1
	printf("%02X", b);
#endif
	//addLogAdv(LOG_INFO, LOG_FEATURE_TUYAMCU,"%02X", b);
}

int HAL_UART_Init(int baud, int parity, bool hwflowc, int txOverride, int rxOverride)
{
	return 1;
}
void HAL_UART_Flush(void)
{
}
void HAL_SetBaud(unsigned int baud)
{
}
#endif