#if PLATFORM_XRADIO

#include "driver/chip/hal_uart.h"
#include "../hal_uart.h"
#include "../../logging/logging.h"
#include "hal_pinmap_xradio.h"

static UART_ID uart = UART1_ID;
bool isInit = false;

__nonxip_text
static void rx_callback(void* arg)
{
	UART_T* uart_inst = HAL_UART_GetInstance(uart);
	while(HAL_UART_IsRxReady(uart_inst))
	{
		uint8_t data = HAL_UART_GetRxData(uart_inst);
		UART_AppendByteToReceiveRingBuffer(data);
	}
}

void HAL_UART_SendByte(byte b)
{
	HAL_UART_Transmit_IT(uart, &b, 1);
}

int OBK_HAL_UART_Init(int baud, int parity, bool hwflowc)
{
	if(isInit) return 1;
	HAL_Status status = HAL_ERROR;
	UART_InitParam param;
	param.baudRate = baud;
	param.dataBits = UART_DATA_BITS_8;
	param.stopBits = UART_STOP_BITS_1;
	param.parity = parity;
	param.isAutoHwFlowCtrl = hwflowc;

	if(CFG_HasFlag(OBK_FLAG_USE_SECONDARY_UART))
	{
		// UART0 is log/console only (XR3), assuming it won't ever be used.
#if PLATFORM_XR809
		// No UART2 on XR809
		uart = UART1_ID;
#else
		uart = UART2_ID;
#endif
	}
#undef HAL_UART_Init
	status = HAL_UART_Init(uart, &param);
	if(status != HAL_OK)
	{
		ADDLOG_ERROR(LOG_FEATURE_PINS, "HAL_UART_Init error");
		return 0;
	}
	isInit = true;
	UART_T* uart_inst = HAL_UART_GetInstance(uart);
	HAL_UART_EnableRxCallback(uart, rx_callback, uart_inst);
	return 1;
}

void HAL_UART_Deinit()
{
	HAL_UART_DeInit(uart);
	isInit = false;
}

#endif // PLATFORM_XR809

