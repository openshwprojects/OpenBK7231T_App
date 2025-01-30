#ifdef PLATFORM_BL602

#include "../../new_pins.h"
#include "../../new_cfg.h"
#include "../../cmnds/cmd_public.h"
#include "../../cmnds/cmd_local.h"
#include "../../logging/logging.h"

#include "../hal_uart.h"
#include <vfs.h>
#include <bl_uart.h>
#include <bl_irq.h>
#include <event_device.h>
#include <cli.h>
#include <aos/kernel.h>
#include <aos/yloop.h>

#include <FreeRTOS.h>
#include <task.h>
#include <timers.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <vfs.h>
#include <aos/kernel.h>
#include <aos/yloop.h>
#include <event_device.h>
#include <cli.h>

#include <lwip/tcpip.h>
#include <lwip/sockets.h>
#include <lwip/netdb.h>
#include <lwip/tcp.h>
#include <lwip/err.h>
#include <http_client.h>
#include <netutils/netutils.h>

#include <bl602_glb.h>
#include <bl602_hbn.h>

#include <bl_uart.h>
#include <bl_chip.h>
#include <bl_wifi.h>
#include <hal_wifi.h>
#include <bl_sec.h>
#include <bl_cks.h>
#include <bl_irq.h>
#include <bl_dma.h>
#include <bl_timer.h>
#include <bl_gpio_cli.h>
#include <bl_wdt_cli.h>
#include <hal_uart.h>
#include <hal_sys.h>
#include <hal_gpio.h>
#include <hal_boot2.h>
#include <hal_board.h>
#include <looprt.h>
#include <loopset.h>
#include <sntp.h>
#include <bl_sys_time.h>
#include <bl_sys.h>
#include <bl_sys_ota.h>
#include <bl_romfs.h>
#include <fdt.h>
#include <device/vfs_uart.h>
#include <utils_log.h>
#include <bl602_uart.h>

#include <easyflash.h>
#include <bl60x_fw_api.h>
#include <wifi_mgmr_ext.h>
#include <utils_log.h>
#include <libfdt.h>
#include <blog.h>

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

int HAL_UART_Init(int baud, int parity)
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
			addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER, "Init CLI with event Driven\r\n");
			aos_cli_init(0);
			aos_poll_read_fd(fd_console, console_cb_read, (void*)0x12345678);
		}
		else
		{
			addLogAdv(LOG_INFO, LOG_FEATURE_ENERGYMETER, "failed CLI with event Driven\r\n");
		}
	}
	return 1;
}

#endif