#if PLATFORM_XRADIO

#include "driver/chip/hal_uart.h"
#include "../hal_uart.h"
#include "../../logging/logging.h"
#include "hal_pinmap_xradio.h"
#if PLATFORM_XR806 || PLATFORM_XR809
#include "serial.h"
#endif

extern void serial_stop(void);
extern int serial_deinit(UART_ID uart_id);
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

int OBK_HAL_UART_Init(int baud, int parity, bool hwflowc, int txOverride, int rxOverride)
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
#if PLATFORM_XR809
		// No UART2 on XR809
		serial_stop();
		serial_deinit(UART0_ID);
		uart = UART0_ID;
#elif PLATFORM_XR806
		// WXU uses UART0, and we have console enabled on it.
		serial_stop();
		serial_deinit(UART0_ID);

		// reenable console on UART2 (default log port on WXU)
		serial_init(UART2_ID, 115200, UART_DATA_BITS_8, UART_PARITY_NONE, UART_STOP_BITS_1, 1);
		serial_start();

		uart = UART0_ID;
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

