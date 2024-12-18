#include "../new_common.h"
#include "../driver/drv_uart.h"

void HAL_UART_SendByte(byte b);

int HAL_UART_Init(int baud, int parity);
