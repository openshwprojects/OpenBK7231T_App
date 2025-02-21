#ifdef PLATFORM_RTL8720D

#include "../../../new_pins.h"
#include "../../../new_cfg.h"
#include "../../hal_uart.h"
#include "serial_api.h"
#include "rtl8721d_uart.h"
#define UART0_TX	PA_18
#define UART0_RX	PA_19
#define UART0_RTS	PA_16
#define UART0_CTS	PA_17
#define UART1_TX	PA_12
#define UART1_RX	PA_13
#define UART1_RTS	PA_23
#define UART1_CTS	PA_24
// log port
#define UART2_TX	PA_7
#define UART2_RX	PA_8

serial_t sobj;
static bool isInitialized;

static void uart_cb(uint32_t id, SerialIrq event)
{
	if(event != RxIrq)
		return;

	serial_t* obj = (void*)id;
	uint8_t c = (uint8_t)serial_getc(obj);
	UART_AppendByteToReceiveRingBuffer(c);
}

void HAL_UART_SendByte(byte b)
{
	while(!serial_writable(&sobj));
	serial_putc(&sobj, b);
}

int HAL_UART_Init(int baud, int parity, bool hwflowc)
{
	if(isInitialized)
	{
		serial_free(&sobj);
	}
	PinName tx = UART0_TX, rx = UART0_RX, rts = UART0_RTS, cts = UART0_CTS;
	if(CFG_HasFlag(OBK_FLAG_USE_SECONDARY_UART))
	{
		tx = UART1_TX;
		rx = UART1_RX;
		rts = UART1_RTS;
		cts = UART1_CTS;
	}
	serial_init(&sobj, tx, rx);
	serial_baud(&sobj, baud);
	serial_format(&sobj, 8, parity, 1);
	serial_irq_handler(&sobj, uart_cb, (uint32_t)&sobj);
	serial_irq_set(&sobj, RxIrq, 1);
	if(hwflowc)
	{
		serial_set_flow_control(&sobj, FlowControlRTSCTS, rts, cts);
	}
	isInitialized = true;
	return 1;
}

#endif