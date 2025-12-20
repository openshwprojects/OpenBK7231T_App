#if defined(PLATFORM_W800) || defined(PLATFORM_W600) 

#include "../hal_generic.h"
#include "../../new_pins.h"
#include "../../new_cfg.h"
#include "../hal_uart.h"
#include "../../logging/logging.h"

#include "wm_include.h"
#include "wm_uart.h"
#include "wm_gpio_afsel.h" 	// for wm_uart1_rx_config and wm_uart1_tx_config

//#define READ_BUF_SIZE 256
uint16_t used_uart = TLS_UART_1;

static s16 obk_uart_rx(u16 len)
{

	//u8 *buf=malloc(READ_BUF_SIZE);
	//int i;
	//if (! buf ){ 
	//	addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER, "W800 obk_uart_rx - malloc failed!\r\n");
	//	return WM_FAILED;
	//}
	while(len)
	{
		//int ml = tls_uart_read(TLS_UART_1, buf, READ_BUF_SIZE - 1);
		//buf[READ_BUF_SIZE - 1] = 0;
		//for(i = 0; i < len && i < ml; i++)
		//{
		//	UART_AppendByteToReceiveRingBuffer(buf[i]);
		//}
		//len -= i;
		byte b;
		int ml = tls_uart_read(used_uart, &b, 1);
		UART_AppendByteToReceiveRingBuffer(b);
		len -= ml;
	}
	//free(buf);
	return WM_SUCCESS;
}

void HAL_UART_SendByte(byte b)
{
	tls_uart_write(used_uart, (char*)&b, 1);
}

int HAL_UART_Init(int baud, int parity, bool hwflowc, int txOverride, int rxOverride)
{
	struct tls_uart_options uart_opts;
	uart_opts.baudrate = baud;
	uart_opts.charlength = TLS_UART_CHSIZE_8BIT;
	uart_opts.flow_ctrl = hwflowc == true ? TLS_UART_FLOW_CTRL_HARDWARE : TLS_UART_FLOW_CTRL_NONE;
	uart_opts.paritytype = parity;
	uart_opts.stopbits = TLS_UART_ONE_STOPBITS;

#if defined(PLATFORM_W600)
#define tls_uart_rx_callback_register(a,b,c) tls_uart_rx_callback_register(a,b)
	if(!CFG_HasFlag(OBK_FLAG_USE_SECONDARY_UART))
	{
		used_uart = TLS_UART_1;
		wm_uart1_cts_config(WM_IO_PB_09);
		wm_uart1_rts_config(WM_IO_PB_10);
		wm_uart1_rx_config(WM_IO_PB_11);
		wm_uart1_tx_config(WM_IO_PB_12);
	}
	else
	{
		used_uart = TLS_UART_0;
		wm_uart0_rx_config(WM_IO_PA_05);
		wm_uart0_tx_config(WM_IO_PA_04);
	}
#else
	if(!CFG_HasFlag(OBK_FLAG_USE_SECONDARY_UART))
	{
		used_uart = TLS_UART_1;
		wm_uart1_cts_config(WM_IO_PB_19);
		wm_uart1_rts_config(WM_IO_PB_20);
		wm_uart1_rx_config(WM_IO_PB_07);
		wm_uart1_tx_config(WM_IO_PB_06);
	}
	else
	{
		used_uart = TLS_UART_0;
		wm_uart0_rx_config(WM_IO_PB_20);
		wm_uart0_tx_config(WM_IO_PB_19);
	}
#endif

	if(WM_SUCCESS != tls_uart_port_init(used_uart, &uart_opts, 1))
	{
		addLogAdv(LOG_ERROR, LOG_FEATURE_DRV, "UART init failed!");
		return;
	}
	tls_uart_rx_callback_register((u16)used_uart, (s16(*)(u16, void*))obk_uart_rx, NULL);
}

#endif
