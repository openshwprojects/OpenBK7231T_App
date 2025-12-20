#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
#include "../quicktick.h"
#include "../cmnds/cmd_public.h"
#include "../logging/logging.h"
#include "errno.h"
#include <lwip/sockets.h>
#include "drv_uart.h"

#if ENABLE_DRIVER_UART_TCP

#define DEFAULT_BUF_SIZE		512
#define DEFAULT_UART_TCP_PORT	8888
#define INVALID_SOCK			-1
#ifndef UTCP_DEBUG
#define UTCP_DEBUG				0
#endif
#if UTCP_DEBUG
#define STACK_SIZE				8192
#else
#define STACK_SIZE				3584
#endif
extern int Main_HasWiFiConnected();
static uint16_t buf_size = DEFAULT_BUF_SIZE;
static int g_conn_channel = -1;
static int g_baudRate = 115200;
static int listen_sock = INVALID_SOCK;
static int client_sock = INVALID_SOCK;
static xTaskHandle g_start_thread = NULL;
static xTaskHandle g_trx_thread = NULL;
static xTaskHandle g_rx_thread = NULL;
static xTaskHandle g_tx_thread = NULL;
static bool rx_closed, tx_closed;
static byte* g_utcpBuf = 0;

void Start_UART_TCP(void* arg);
void UART_TCP_Deinit();

static void UTCP_TX_Thd(void* param)
{
	int client_fd = *(int*)param;

	while(1)
	{
		int ret = 0;
		int delay = 0;
		memset(g_utcpBuf, 0, buf_size);
		int len = UART_GetDataSize();

		if(client_fd == INVALID_SOCK) goto exit;
		while(len < buf_size && delay < 10)
		{
			rtos_delay_milliseconds(1);
			len = UART_GetDataSize();
			delay++;
		}

		if(len > 0)
		{
			for(int i = 0; i < len; i++)
			{
				g_utcpBuf[i] = UART_GetByte(i);
			}
			UART_ConsumeBytes(len);
#if UTCP_DEBUG
			char data[len * 2];
			char* p = data;
			for(int i = 0; i < len; i++)
			{
				sprintf(p, "%02X", g_utcpBuf[i]);
				p += 2;
			}
			ADDLOG_EXTRADEBUG(LOG_FEATURE_DRV, "%d bytes UART RX->TCP TX: %s", len, data);
#endif
			ret = send(client_fd, g_utcpBuf, len, 0);
		}
		else
		{
			if(rx_closed)
			{
				goto exit;
			}
			continue;
		}

		if(ret <= 0)
			goto exit;

		rtos_delay_milliseconds(5);
	}

exit:
	ADDLOG_DEBUG(LOG_FEATURE_DRV, "UTCP_TX_Thd closed");
	tx_closed = true;
	rtos_suspend_thread(NULL);
}

static void UTCP_RX_Thd(void* param)
{
	int client_fd = *(int*)param;
	unsigned char buffer[1024];

	while(1)
	{
		int ret = 0;

		if(client_fd == INVALID_SOCK) goto exit;
		ret = recv(client_fd, buffer, sizeof(buffer), 0);
		if(ret > 0)
		{
#if UTCP_DEBUG
			char data[ret * 2];
			char* p = data;
			for(int i = 0; i < ret; i++)
			{
				sprintf(p, "%02X", buffer[i]);
				p += 2;
			}
			ADDLOG_EXTRADEBUG(LOG_FEATURE_DRV, "%d bytes TCP RX->UART TX: %s", ret, data);
#endif
			for(int i = 0; i < ret; i++)
			{
				UART_SendByte(buffer[i]);
			}
		}
		else if(tx_closed)
		{
			goto exit;
		}

		// ret == -1 and socket error == EAGAIN when no data received for nonblocking
		if((ret == -1) && (errno == EAGAIN))
			continue;
		else if(ret <= 0)
		{
			ADDLOG_DEBUG(LOG_FEATURE_DRV, "ret: %i, errno: %i", ret, errno);
			goto exit;
		}

		rtos_delay_milliseconds(5);
	}

exit:
	ADDLOG_DEBUG(LOG_FEATURE_DRV, "UTCP_RX_Thd closed");
	rx_closed = true;
	rtos_suspend_thread(NULL);
}

void UART_TCP_TRX_Thread()
{
	OSStatus err = kNoErr;
	int reuse = 1;
	struct sockaddr_in server_addr =
	{
		.sin_family = AF_INET,
		.sin_addr =
		{
			.s_addr = INADDR_ANY,
		},
		.sin_port = htons(DEFAULT_UART_TCP_PORT),
	};

	if(listen_sock != INVALID_SOCK) close(listen_sock);
	if(client_sock != INVALID_SOCK) close(client_sock);

	listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(listen_sock < 0)
	{
		ADDLOG_ERROR(LOG_FEATURE_DRV, "Unable to create socket");
		goto error;
	}
	int flags = fcntl(listen_sock, F_GETFL, 0);
	if(fcntl(listen_sock, F_SETFL, flags | O_NONBLOCK) == -1)
	{
		ADDLOG_ERROR(LOG_FEATURE_DRV, "Unable to set socket non blocking");
		goto error;
	}

	setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse));

	err = bind(listen_sock, (struct sockaddr*)&server_addr, sizeof(server_addr));
	if(err != 0)
	{
		ADDLOG_ERROR(LOG_FEATURE_DRV, "Socket unable to bind");
		goto error;
	}
	err = listen(listen_sock, 2);
	if(err != 0)
	{
		ADDLOG_ERROR(LOG_FEATURE_HTTP, "Error occurred during listen");
		goto error;
	}

	while(1)
	{
		struct sockaddr_storage source_addr;
		socklen_t addr_len = sizeof(source_addr);
		client_sock = accept(listen_sock, (struct sockaddr*)&source_addr, &addr_len);
		if(client_sock != INVALID_SOCK)
		{
			if(g_conn_channel >= 0) CHANNEL_Set(g_conn_channel, 1, CHANNEL_SET_FLAG_SKIP_MQTT | CHANNEL_SET_FLAG_SILENT);
			rx_closed = true;
			tx_closed = true;

			if(g_tx_thread != NULL)
			{
				rtos_delete_thread(&g_tx_thread);
			}
			err = rtos_create_thread(&g_tx_thread, BEKEN_APPLICATION_PRIORITY - 1,
				"UTCP_TX_Thd",
				(beken_thread_function_t)UTCP_TX_Thd,
				STACK_SIZE,
				(beken_thread_arg_t)&client_sock);
			if(err != kNoErr)
			{
				ADDLOG_ERROR(LOG_FEATURE_DRV, "create \"UTCP_TX_Thd\" thread failed with %i!", err);
			}
			else tx_closed = false;

			//rtos_delay_milliseconds(10);

			if(g_rx_thread != NULL)
			{
				rtos_delete_thread(&g_rx_thread);
			}
			err = rtos_create_thread(&g_rx_thread, BEKEN_APPLICATION_PRIORITY - 1,
				"UTCP_RX_Thd",
				(beken_thread_function_t)UTCP_RX_Thd,
				STACK_SIZE,
				(beken_thread_arg_t)&client_sock);
			if(err != kNoErr)
			{
				ADDLOG_ERROR(LOG_FEATURE_DRV, "create \"UTCP_RX_Thd\" thread failed with %i!", err);
			}
			else rx_closed = false;

			while(1)
			{
				if(tx_closed && rx_closed)
				{
					close(client_sock);
					ADDLOG_DEBUG(LOG_FEATURE_DRV, "UART TCP connection closed", err);
					if(g_conn_channel >= 0) CHANNEL_Set(g_conn_channel, 0, CHANNEL_SET_FLAG_SKIP_MQTT | CHANNEL_SET_FLAG_SILENT);
					break;
				}
				else
				{
					if(!Main_HasWiFiConnected()) goto error;
					rtos_delay_milliseconds(10);
				}
			}
		}
		rtos_delay_milliseconds(10);
	}

error:
	ADDLOG_ERROR(LOG_FEATURE_DRV, "UART TCP Error");

	if(g_start_thread != NULL)
	{
		rtos_delete_thread(&g_start_thread);
	}
	err = rtos_create_thread(&g_start_thread, BEKEN_APPLICATION_PRIORITY,
		"UART TCP Restart",
		(beken_thread_function_t)Start_UART_TCP,
		0x800,
		(beken_thread_arg_t)0);
	if(err != kNoErr)
	{
		ADDLOG_ERROR(LOG_FEATURE_DRV, "create \"UART TCP Restart\" thread failed with %i!\r\n", err);
	}
}

void Start_UART_TCP(void* arg)
{
	UART_TCP_Deinit();

	g_utcpBuf = (byte*)os_malloc(buf_size);

	OSStatus err = rtos_create_thread(&g_trx_thread, BEKEN_APPLICATION_PRIORITY,
		"UART_TCP_TRX",
		(beken_thread_function_t)UART_TCP_TRX_Thread,
		0x800,
		(beken_thread_arg_t)0);
	if(err != kNoErr)
	{
		ADDLOG_ERROR(LOG_FEATURE_DRV, "create \"UART_TCP_TRX\" thread failed with %i!\r\n", err);
	}
	rtos_suspend_thread(NULL);
}

// startDriver UartTCP [baudrate] [buffer size] [connection channel] [hw flow control]
// connection is for led, -1 if not used.
// Sample:
// startDriver UartTCP 115200 8192
// Then connect to 8888

// backlog stopDriver UartTCP; startDriver UartTCP 115200 8192
void UART_TCP_Init()
{
	g_baudRate = Tokenizer_GetArgIntegerDefault(1, g_baudRate);
	uint32_t reqbufsize = Tokenizer_GetArgIntegerDefault(2, DEFAULT_BUF_SIZE);
	buf_size = reqbufsize > 16384 ? 16384 : reqbufsize;
	g_conn_channel = Tokenizer_GetArgIntegerDefault(3, -1);
	int flowcontrol = Tokenizer_GetArgIntegerDefault(4, 0);

	UART_InitUART(g_baudRate, 0, flowcontrol > 0 ? true : false);
	UART_InitReceiveRingBuffer(buf_size * 2);

	if(g_start_thread != NULL)
	{
		rtos_delete_thread(&g_start_thread);
	}
	OSStatus err = rtos_create_thread(&g_start_thread, BEKEN_APPLICATION_PRIORITY,
		"UART_TCP",
		(beken_thread_function_t)Start_UART_TCP,
		0x800,
		(beken_thread_arg_t)0);
	if(err != kNoErr)
	{
		ADDLOG_ERROR(LOG_FEATURE_DRV, "create \"UART_TCP\" thread failed with %i!", err);
	}
}

void UART_TCP_Deinit()
{
	if(g_trx_thread != NULL)
	{
		rtos_delete_thread(&g_trx_thread);
		g_trx_thread = NULL;
	}
	if(g_rx_thread != NULL)
	{
		rx_closed = true;
		rtos_delete_thread(&g_rx_thread);
		g_rx_thread = NULL;
	}
	if(g_tx_thread != NULL)
	{
		tx_closed = true;
		rtos_delete_thread(&g_tx_thread);
		g_tx_thread = NULL;
	}
	if(g_utcpBuf) free(g_utcpBuf);

	if(listen_sock != INVALID_SOCK) close(listen_sock);
	if(client_sock != INVALID_SOCK) close(client_sock);
}

#endif