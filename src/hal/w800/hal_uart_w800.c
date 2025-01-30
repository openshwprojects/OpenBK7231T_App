#if defined(PLATFORM_W800) || defined(PLATFORM_W600) 

#include "../hal_generic.h"
#include "../hal_uart.h"
#include "../../logging/logging.h"

#include "wm_include.h"
#include "wm_uart.h"
#include "wm_gpio_afsel.h" 	// for wm_uart1_rx_config and wm_uart1_tx_config
#define UART_DEV_NAME "uart1"
#define READ_BUF_SIZE 256
static s16 obk_uart_rx(u16 len)
{

//addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER, "W800 obk_uart_rx - len=%i\r\n",len);
    u8 *buf=malloc(READ_BUF_SIZE);
    int i;
    if (! buf ){ 
    	addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER, "W800 obk_uart_rx - malloc failed!\r\n");
    	return WM_FAILED;
    }
    while(len){
	int ml=tls_uart_read(TLS_UART_1, buf,READ_BUF_SIZE-1 );
//addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER, "W800 obk_uart_rx - ml=%i\r\n",ml);
	buf[READ_BUF_SIZE-1]=0;
//addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER, "W800 obk_uart_rx - read=%s - len=%i - READ_BUF=%i\r\n",buf,len,READ_BUF_SIZE);
	for(i = 0; i < len && i < ml; i++)
	{
		UART_AppendByteToReceiveRingBuffer(buf[i]);
	}
	len -=i;
    }
    free(buf);
    return WM_SUCCESS;
}

void HAL_UART_SendByte(byte b)
{
	tls_uart_write(TLS_UART_1, (char*)&b, 1);
}

int HAL_UART_Init(int baud, int parity)
{
	struct tls_uart_options uart_opts;
	uart_opts.baudrate = baud;
	uart_opts.charlength = TLS_UART_CHSIZE_8BIT;
	uart_opts.flow_ctrl = TLS_UART_FLOW_CTRL_NONE;
	uart_opts.paritytype = parity;
	uart_opts.stopbits = TLS_UART_ONE_STOPBITS;
	wm_uart1_rx_config(WM_IO_PB_07);
	wm_uart1_tx_config(WM_IO_PB_06);

	if (WM_SUCCESS != tls_uart_port_init(TLS_UART_1, &uart_opts, 1))
            return;
	tls_uart_rx_callback_register((u16) TLS_UART_1, (s16(*)(u16, void*))obk_uart_rx, NULL);

}


#endif
