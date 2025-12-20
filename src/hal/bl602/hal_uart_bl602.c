#ifdef PLATFORM_BL602

#include "../../new_pins.h"
#include "../../new_cfg.h"
#include "../../cmnds/cmd_public.h"
#include "../../cmnds/cmd_local.h"
#include "../../logging/logging.h"

#include "../hal_uart.h"
#include <vfs.h>
#include <device/vfs_uart.h>
#include <bl_uart.h>
#include <bl602_uart.h>
#include <aos/yloop.h>

//int g_fd;
uint8_t g_id = 1;
int fd_console = -1;

//void UART_RunQuickTick() {
//}
//void MY_UART1_IRQHandler(void)
//{
//	int length;
//	byte buffer[16];
//	//length = aos_read(g_fd, buffer, 1);
//	//if (length > 0) {
//	//	UART_AppendByteToReceiveRingBuffer(buffer[0]);
//	//}
//	int res = bl_uart_data_recv(g_id);
//	if (res >= 0) {
//		addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER, "UART received: %i\n", res);
//		UART_AppendByteToReceiveRingBuffer(res);
//	}
//}

static void console_cb_read(int fd, void* param)
{
	char buffer[64];  /* adapt to usb cdc since usb fifo is 64 bytes */
	int ret;
	int i;

	ret = aos_read(fd, buffer, sizeof(buffer));
	if(ret > 0)
	{
		if(ret < sizeof(buffer))
		{
			fd_console = fd;
			buffer[ret] = 0;
			addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER, "BL602 received: %s\n", buffer);
			for(i = 0; i < ret; i++)
			{
				UART_AppendByteToReceiveRingBuffer(buffer[i]);
			}
		}
		else
		{
			printf("-------------BUG from aos_read for ret\r\n");
		}
	}
}

void HAL_UART_SendByte(byte b)
{
	aos_write(fd_console, &b, 1);
	//bl_uart_data_send(g_id, b);
}

int HAL_UART_Init(int baud, int parity, bool hwflowc, int txOverride, int rxOverride)
{
	if(fd_console < 0)
	{
		//uint8_t tx_pin = 16;
		//uint8_t rx_pin = 7;
		//bl_uart_init(g_id, tx_pin, rx_pin, 0, 0, baud);
		//g_fd = aos_open(name, 0);
		//bl_uart_int_rx_enable(1);
		//bl_irq_register(UART1_IRQn, MY_UART1_IRQHandler);
		//bl_irq_enable(UART1_IRQn);
		//vfs_uart_init_simple_mode(0, 7, 16, baud, "/dev/ttyS0");

		if(CFG_HasFlag(OBK_FLAG_USE_SECONDARY_UART))
		{
			fd_console = aos_open("/dev/ttyS1", 0);
		}
		else
		{
			fd_console = aos_open("/dev/ttyS0", 0);
		}
		if(fd_console >= 0)
		{
			aos_ioctl(fd_console, IOCTL_UART_IOC_BAUD_MODE, baud);
			aos_poll_read_fd(fd_console, console_cb_read, (void*)0x12345678);
			addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER, "UART init done\r\n");
		}
		else
		{
			addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER, "UART init failed\r\n");
		}
	}
	return 1;
}

#endif