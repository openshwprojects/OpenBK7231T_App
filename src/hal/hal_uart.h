#include "../new_common.h"
#include "../driver/drv_uart.h"

#if PLATFORM_BEKEN
#define UART_2_UARTS_CONCURRENT
#endif

#ifdef UART_2_UARTS_CONCURRENT
void HAL_UART_SendByteEx(int auartindex, byte b);

int HAL_UART_InitEx(int auartindex, int baud, int parity, bool hwflowc);
#else
void HAL_UART_SendByte(byte b);

int HAL_UART_Init(int baud, int parity, bool hwflowc);
#endif