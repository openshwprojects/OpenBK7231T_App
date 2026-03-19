#include "../new_common.h"
#include "../logging/logging.h"
#include "../cmnds/cmd_public.h"
#include "drv_esphome_api.h"
#include "../hal/hal_bt_proxy.h"
#include <lwip/sockets.h>

#if ENABLE_DRIVER_ESPHOME_API

// Global state tracking whether the client requested BLE advertisement forwarding
bool g_bt_proxy_forwarding_active = false;

#define ESPHOME_API_PORT 6053
#define INVALID_SOCK -1

#if PLATFORM_ESPIDF
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#endif

extern int Main_HasWiFiConnected();

static int s_listen_sock = INVALID_SOCK;
static xTaskHandle s_esphome_api_thread = NULL;
static int client_sock = INVALID_SOCK;

static void ESPHome_API_TCP_Server_Thread(void* param)
{
	int reuse = 1;
	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(ESPHOME_API_PORT);

	if(s_listen_sock != INVALID_SOCK)
	{
		close(s_listen_sock);
	}

	s_listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(s_listen_sock < 0)
	{
		ADDLOG_ERROR(LOG_FEATURE_DRV, "BTProxy: Unable to create socket");
		rtos_suspend_thread(NULL);
		return;
	}

	setsockopt(s_listen_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse));

	if(bind(s_listen_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) != 0)
	{
		ADDLOG_ERROR(LOG_FEATURE_DRV, "ESPHomeAPI: Socket unable to bind to port %d", ESPHOME_API_PORT);
		close(s_listen_sock);
		s_listen_sock = INVALID_SOCK;
		rtos_suspend_thread(NULL);
		return;
	}

	if(listen(s_listen_sock, 2) != 0)
	{
		ADDLOG_ERROR(LOG_FEATURE_DRV, "ESPHomeAPI: Error occurred during listen");
		close(s_listen_sock);
		s_listen_sock = INVALID_SOCK;
		rtos_suspend_thread(NULL);
		return;
	}

	ADDLOG_INFO(LOG_FEATURE_DRV, "ESPHomeAPI: TCP Server listening on port %d", ESPHOME_API_PORT);

	while(1)
	{
		struct sockaddr_storage source_addr;
		socklen_t addr_len = sizeof(source_addr);
		client_sock = accept(s_listen_sock, (struct sockaddr*)&source_addr, &addr_len);

		if(client_sock != INVALID_SOCK)
		{
			int flags = fcntl(client_sock, F_GETFL, 0);
			fcntl(client_sock, F_SETFL, flags | O_NONBLOCK);
			ADDLOG_INFO(LOG_FEATURE_DRV, "ESPHomeAPI: Client connected (sock %d)", client_sock);

			// ESPHome Native API Framing Decoder
			unsigned char buffer[1024];
			int read_len = 0;

			while(Main_HasWiFiConnected())
			{
				if(read_len == sizeof(buffer))
				{
					goto disconnect;
				}
				int ret = recv(client_sock, buffer + read_len, sizeof(buffer) - read_len, 0);
				if(ret > 0)
				{
					read_len += ret;

					// Parse framing
					while(read_len > 0)
					{
						if(buffer[0] != 0x00)
						{
							ADDLOG_ERROR(LOG_FEATURE_DRV, "ESPHomeAPI: Invalid preamble (0x%02X)", buffer[0]);
							goto disconnect;
						}

						// Need enough bytes for 1 preamble + at least 2 varints
						if(read_len < 3) break;

						uint8_t* p = buffer + 1;
						uint32_t msg_size = 0, msg_type = 0;
						int shift = 0;

						// varint size
						while(p < buffer + read_len)
						{
							msg_size |= (*p & 0x7F) << shift;
							shift += 7;
							if(!(*p++ & 0x80)) break;
						}
						// varint type
						shift = 0;
						while(p < buffer + read_len)
						{
							msg_type |= (*p & 0x7F) << shift;
							shift += 7;
							if(!(*p++ & 0x80)) break;
						}

						int header_len = p - buffer;

						// Check if the full message has arrived based on msg_size
						if(read_len >= header_len + (int)msg_size)
						{
							// Full frame received, process it
							ESPHome_API_Process_Packet(client_sock, msg_type, p, msg_size);

							// Consume bytes and shift buffer left
							int consumed = header_len + msg_size;
							read_len -= consumed;
							if(read_len > 0)
							{
								memmove(buffer, buffer + consumed, read_len);
							}
						}
						else
						{
							if(header_len + msg_size > sizeof(buffer))
							{
								ADDLOG_ERROR(LOG_FEATURE_DRV, "ESPHomeAPI: Message exceeds buffer! (%d)", msg_size);
								goto disconnect;
							}
							break; // Wait for more data to complete the frame
						}
					}
				}
				else if(ret == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
				{
					rtos_delay_milliseconds(10);
					continue;
				}
				else
				{
					break; // Error or closed
				}
			}
		disconnect:
			ADDLOG_INFO(LOG_FEATURE_DRV, "ESPHomeAPI: Client disconnected or error");
			g_bt_proxy_forwarding_active = false;
			close(client_sock);
			client_sock = INVALID_SOCK;
		}
		rtos_delay_milliseconds(50);
	}

	rtos_suspend_thread(NULL);
}

bool ESPHome_API_PassScanResult(const uint8_t* mac, int rssi, uint8_t addr_type, const uint8_t* data, int data_len)
{
	return ESPHome_API_Hook_ScanResult(client_sock, mac, rssi, addr_type, data, data_len);
}

void DRV_ESPHome_API_Init()
{
	OSStatus err = rtos_create_thread(&s_esphome_api_thread, BEKEN_APPLICATION_PRIORITY - 1,
		"ESPHomeAPI_Srv",
		(beken_thread_function_t)ESPHome_API_TCP_Server_Thread,
		4096,
		(beken_thread_arg_t)0);

	if(err != kNoErr)
	{
		ADDLOG_ERROR(LOG_FEATURE_DRV, "ESPHomeAPI: create server thread failed with %d", err);
	}
	else
	{
		ADDLOG_INFO(LOG_FEATURE_DRV, "ESPHomeAPI: server thread started.");
	}
}

void DRV_ESPHome_API_Deinit()
{
	if(s_esphome_api_thread != NULL)
	{
		rtos_delete_thread(&s_esphome_api_thread);
		s_esphome_api_thread = NULL;
	}
}

void DRV_ESPHome_API_OnEverySecond()
{
#if ENABLE_BT_PROXY
	HAL_BTProxy_OnEverySecond();
#endif
}

#endif // ENABLE_DRIVER_ESPHOME_API
