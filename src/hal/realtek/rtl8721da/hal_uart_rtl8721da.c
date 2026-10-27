#if PLATFORM_RTL8721DA || PLATFORM_RTL8720E

#include "../../../new_pins.h"
#include "../../../new_cfg.h"
#include "../../hal_uart.h"
#include "serial_api.h"
#include "os_wrapper.h"
#if PLATFORM_RTL8721DA
// default example pins
//#define UART0_TX	PA_26
//#define UART0_RX	PA_27
//#define UART0_RTS	PA_28
//#define UART0_CTS	PA_29
//// UART0 on WR11-U and WR11-2S
//#define UART1_TX	PB_20
//#define UART1_RX	PB_21
#define UART0_TX	PB_20
#define UART0_RX	PB_21
#define UART0_RTS	PB_18
#define UART0_CTS	PB_19
#elif PLATFORM_RTL8720E
// default example pins
#define UART0_TX	PA_28
#define UART0_RX	PA_29
#define UART0_RTS	PA_30
#define UART0_CTS	PA_31
#define UART1_TX UART0_TX
#define UART1_RX UART0_RX
#endif

serial_t sobj;
static bool isInitialized;
int g_urx_pin = UART0_RX;
int g_utx_pin = UART0_TX;
int g_rts_pin = UART0_RTS;
int g_cts_pin = UART0_CTS;

static void uart_cb(uint32_t id, SerialIrq event)
{
	if(event != RxIrq)
		return;

	serial_t* obj = (void*)id;
	while(serial_readable(obj))
	{
		uint8_t c = (uint8_t)serial_getc(obj);
		UART_AppendByteToReceiveRingBuffer(c);
	}
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
	PinName tx = UART0_TX, rx = UART0_RX, rts = UART0_RTS, cts = UART0_CTS;
	serial_init(&sobj, g_utx_pin, g_urx_pin);
	serial_baud(&sobj, baud);
	serial_format(&sobj, 8, parity, 1);
	serial_irq_handler(&sobj, uart_cb, (uint32_t)&sobj);
	serial_irq_set(&sobj, RxIrq, 1);
	if(hwflowc && g_rts_pin != -1 && g_cts_pin != -1)
	{
		serial_set_flow_control(&sobj, FlowControlRTSCTS, g_rts_pin, g_cts_pin);
	}
	isInitialized = true;
	return 1;
}

#endif
