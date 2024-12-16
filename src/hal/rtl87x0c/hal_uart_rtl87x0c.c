#ifdef PLATFORM_RTL87X0C

#include "../../new_pins.h"
#include "../../new_cfg.h"
#include "../hal_uart.h"
#include "serial_api.h"
#include "hal_uart.h"
#define UART0_TX	PA_14
#define UART0_RX	PA_13
#define UART1_0_TX	PA_3
#define UART1_0_RX	PA_2
#define UART1_1_TX	PA_1
#define UART1_1_RX	PA_0

serial_t sobj;
static bool isInitialized;

static void uart_cb(uint32_t id, SerialIrq event)
{
	if(event != RxIrq)
		return;

	serial_t* obj = (void*)id;
	uint8_t c;
	while(hal_uart_rgetc(&obj->uart_adp, (char*)&c))
	{
		UART_AppendByteToReceiveRingBuffer(c);
	}
}

void HAL_UART_SendByte(byte b)
{
	while(!hal_uart_writeable(&sobj.uart_adp));
	hal_uart_putc(&sobj.uart_adp, b);
}

int HAL_UART_Init(int baud, int parity)
{
	if(isInitialized)
	{
		serial_free(&sobj);
	}
	PinName tx = UART0_TX, rx = UART0_RX;
	if(CFG_HasFlag(OBK_FLAG_USE_SECONDARY_UART))
	{
		tx = UART1_0_TX;
		rx = UART1_0_RX;
	}
	serial_init(&sobj, tx, rx);
	serial_baud(&sobj, baud);
	serial_format(&sobj, 8, parity, 1);
	serial_irq_handler(&sobj, uart_cb, (uint32_t)&sobj);
	serial_irq_set(&sobj, RxIrq, 1);
	isInitialized = true;
	return 1;
}

#endif