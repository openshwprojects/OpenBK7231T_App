#ifdef PLATFORM_RDA5981

#include "../../new_pins.h"
#include "../../new_cfg.h"
#include "../hal_uart.h"
#include "serial_api.h"
#define UART0_RX	PA_0
#define UART0_TX	PA_1
#define UART1_RX	PB_1
#define UART1_TX	PB_2
#define UART1_1_RX	PD_2
#define UART1_1_TX	PD_3
#define UART1_1_RTS	PD_1
#define UART1_1_CTS	PD_0

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

int HAL_UART_Init(int baud, int parity, bool hwflowc, int txOverride, int rxOverride)
{
	if(isInitialized)
	{
		serial_free(&sobj);
	}
	PinName tx = UART0_TX, rx = UART0_RX;
	if(CFG_HasFlag(OBK_FLAG_USE_SECONDARY_UART))
	{
		tx = UART1_TX;
		rx = UART1_RX;
	}
	serial_init(&sobj, tx, rx);
	serial_baud(&sobj, baud);
	serial_format(&sobj, 8, parity, 1);
	serial_irq_handler(&sobj, uart_cb, (uint32_t)&sobj);
	serial_irq_set(&sobj, RxIrq, 1);
	if(hwflowc)
	{
		tx = UART1_1_TX;
		rx = UART1_1_RX;
		serial_set_flow_control(&sobj, FlowControlRTSCTS, UART1_1_RTS, UART1_1_CTS);
	}
	isInitialized = true;
	return 1;
}

#endif
