#include "../new_common.h"
#include "../logging/logging.h"
#include "../cmnds/cmd_public.h"
#include "drv_esphome_api.h"
#include "../hal/hal_bt_proxy.h"
#include "../new_pins.h"
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
static volatile int client_sock = INVALID_SOCK;

// lwIP here is built without LWIP_NETCONN_FULLDUPLEX: a socket must only be
// used by ONE thread at a time. Sending from the BT task while the server
// thread sits in select()/recv() on the same socket corrupts lwIP/heap state
// (seen as a hard fault in the BT stack on the next HCI send). So only the
// server thread touches client_sock; every other task queues finished frames
// here and the server thread writes them out between select() polls.
typedef struct
{
	uint16_t len;
	uint8_t data[];
} tx_frame_t;
#define TX_QUEUE_LEN 48
#define TX_POLL_MS 20
static QueueHandle_t s_tx_queue = NULL;
static volatile int s_tx_dropped = 0;

// Optional status LEDs (channel indices, -1 = disabled), set by BTProxyLeds
static int s_led_red_ch = -1;
static int s_led_blue_ch = -1;
static int s_led_red_val = -1;
static int s_led_blue_val = -1;

static void ESPHome_API_EncodeVarint(uint8_t** buf, uint32_t value)
{
	while(value >= 0x80)
	{
		**buf = (value & 0x7F) | 0x80;
		(*buf)++;
		value >>= 7;
	}
	**buf = value & 0x7F;
	(*buf)++;
}

// Writes all bytes of a buffer to the client socket without ever blocking
// indefinitely: every send uses MSG_DONTWAIT and is retried for a bounded time.
// (On RTL87X0C lwIP, SO_SNDTIMEO/errno can't be trusted, and a send() that
// blocks forever stalls the BT task.) A failed send cannot be told apart from
// "would block", so both are retried until the deadline.
// Droppable frames give up immediately if nothing could be written.
// Returns number of bytes written (== len on success).
#define SEND_RETRY_DELAY_MS 5
#define SEND_MAX_RETRIES 300 // ~1.5 s
static int ESPHome_API_SendAll(int sock, const uint8_t* data, int len, bool droppable)
{
	int sent = 0;
	int retries = 0;
	while(sent < len)
	{
		int ret = send(sock, data + sent, len - sent, MSG_DONTWAIT);
		if(ret > 0)
		{
			sent += ret;
			retries = 0;
			continue;
		}
		if(droppable && sent == 0) break;
		if(++retries > SEND_MAX_RETRIES) break;
		rtos_delay_milliseconds(SEND_RETRY_DELAY_MS);
	}
	return sent;
}

bool ESPHome_API_IsClientConnected(void)
{
	return client_sock != INVALID_SOCK;
}

// Server thread only. Writes one complete frame; a frame cut off half-way would
// desync the stream, so in that case the client is dropped.
static bool ESPHome_API_WriteFrame(int sock, const uint8_t* buf, int len, bool droppable)
{
	int n = ESPHome_API_SendAll(sock, buf, len, droppable);
	if(n == len) return true;
	if(n > 0)
	{
		ADDLOG_ERROR(LOG_FEATURE_DRV, "ESPHomeAPI: send failed mid-frame (%d/%d), dropping client", n, len);
		shutdown(sock, SHUT_RDWR);
	}
	return false;
}

// Server thread only: write out (or, with sock == INVALID_SOCK, discard) queued frames.
static void ESPHome_API_DrainTx(int sock)
{
	tx_frame_t* f;
	if(s_tx_queue == NULL) return;
	while(xQueueReceive(s_tx_queue, &f, 0) == pdTRUE)
	{
		if(sock != INVALID_SOCK)
		{
			ESPHome_API_WriteFrame(sock, f->data, f->len, false);
		}
		os_free(f);
	}
}

static tx_frame_t* ESPHome_API_BuildFrame(uint32_t msg_type, const uint8_t* payload, uint32_t payload_len)
{
	uint8_t header[16];
	uint8_t* ptr = header;
	*ptr++ = 0x00; // Preamble
	ESPHome_API_EncodeVarint(&ptr, payload_len);
	ESPHome_API_EncodeVarint(&ptr, msg_type);
	int header_len = ptr - header;
	int total = header_len + payload_len;

	tx_frame_t* f = (tx_frame_t*)os_malloc(sizeof(tx_frame_t) + total);
	if(!f) return NULL;
	f->len = (uint16_t)total;
	memcpy(f->data, header, header_len);
	if(payload_len) memcpy(f->data + header_len, payload, payload_len);
	return f;
}

// Server thread only (replies to requests): flush queued frames first to keep
// ordering, then write this one.
bool ESPHome_API_SendFrameNow(int sock, uint32_t msg_type, const uint8_t* payload, uint32_t payload_len)
{
	if(sock == INVALID_SOCK) return false;
	tx_frame_t* f = ESPHome_API_BuildFrame(msg_type, payload, payload_len);
	if(!f) return false;
	ESPHome_API_DrainTx(sock);
	bool ok = ESPHome_API_WriteFrame(sock, f->data, f->len, false);
	os_free(f);
	return ok;
}

// Any thread: queue a frame for the server thread to send.
bool ESPHome_API_SendFrame(uint32_t msg_type, const uint8_t* payload, uint32_t payload_len, bool droppable)
{
	if(client_sock == INVALID_SOCK || s_tx_queue == NULL) return false;
	tx_frame_t* f = ESPHome_API_BuildFrame(msg_type, payload, payload_len);
	if(!f)
	{
		s_tx_dropped++;
		return false;
	}
	// Advertisements are dropped if the queue is full; GATT results wait briefly.
	TickType_t wait = droppable ? 0 : pdMS_TO_TICKS(500);
	if(xQueueSend(s_tx_queue, &f, wait) != pdTRUE)
	{
		os_free(f);
		if(!droppable) ADDLOG_ERROR(LOG_FEATURE_DRV, "ESPHomeAPI: TX queue full, dropped frame type %d", msg_type);
		s_tx_dropped++;
		return false;
	}
	return true;
}

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

	// Non-blocking listen socket: it is also polled with select() while a client
	// is active, and lwIP can report it readable with nothing to accept; a
	// blocking accept() there would stall the active client. Accepted sockets
	// are switched back to blocking below.
	fcntl(s_listen_sock, F_SETFL, O_NONBLOCK);

	ADDLOG_INFO(LOG_FEATURE_DRV, "ESPHomeAPI: TCP Server listening on port %d", ESPHOME_API_PORT);

	// A connection accepted while another client was active; it replaces that
	// client (only one client is served at a time). A new connection from HA
	// means the old one is gone, even if its FIN/RST never reached us.
	int pending_sock = INVALID_SOCK;

	while(1)
	{
		struct sockaddr_storage source_addr;
		socklen_t addr_len = sizeof(source_addr);
		int new_sock;
		if(pending_sock != INVALID_SOCK)
		{
			new_sock = pending_sock;
			pending_sock = INVALID_SOCK;
		}
		else
		{
			new_sock = accept(s_listen_sock, (struct sockaddr*)&source_addr, &addr_len);
		}

		if(new_sock >= 0)
		{
			fcntl(new_sock, F_SETFL, 0); // blocking (see listen socket note)
			ESPHome_API_DrainTx(INVALID_SOCK); // stale frames from a previous client
			client_sock = new_sock;
			// Blocking socket + select(): on some SDKs (RTL87X0C) a non-blocking
			// recv() with no data returns -1 with errno == 0, which is
			// indistinguishable from a real error.
			struct timeval snd_tv = { 2, 0 };
			setsockopt(client_sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&snd_tv, sizeof(snd_tv));
			int nodelay = 1;
			setsockopt(client_sock, IPPROTO_TCP, TCP_NODELAY, (const char*)&nodelay, sizeof(nodelay));
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
				fd_set rfds;
				FD_ZERO(&rfds);
				FD_SET(client_sock, &rfds);
				FD_SET(s_listen_sock, &rfds);
				int maxfd = client_sock > s_listen_sock ? client_sock : s_listen_sock;
				// Short poll so frames queued by other tasks go out promptly.
				struct timeval tv = { 0, TX_POLL_MS * 1000 };
				int sel = select(maxfd + 1, &rfds, NULL, NULL, &tv);
				ESPHome_API_DrainTx(client_sock);
				// No idle timeout: aioesphomeapi only pings when it receives nothing,
				// and we stream adverts, so a healthy client can stay silent for long.
				if(sel == 0) continue;
				if(sel < 0)
				{
					ADDLOG_INFO(LOG_FEATURE_DRV, "ESPHomeAPI: select failed (%d)", sel);
					break;
				}
				if(FD_ISSET(s_listen_sock, &rfds))
				{
					addr_len = sizeof(source_addr);
					pending_sock = accept(s_listen_sock, (struct sockaddr*)&source_addr, &addr_len);
					if(pending_sock < 0)
					{
						pending_sock = INVALID_SOCK; // spurious readiness, nothing to accept
					}
					else
					{
						ADDLOG_INFO(LOG_FEATURE_DRV, "ESPHomeAPI: new client (sock %d) replaces sock %d", pending_sock, client_sock);
						goto disconnect;
					}
				}
				if(!FD_ISSET(client_sock, &rfds)) continue;
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
				else
				{
					ADDLOG_INFO(LOG_FEATURE_DRV, "ESPHomeAPI: recv returned %d", ret);
					break; // Error or closed
				}
			}
		disconnect:
			ADDLOG_INFO(LOG_FEATURE_DRV, "ESPHomeAPI: Client disconnected or error");
			g_bt_proxy_forwarding_active = false;
			{
				int sock = client_sock;
				client_sock = INVALID_SOCK; // stop other tasks from queueing more
				ESPHome_API_DrainTx(INVALID_SOCK);
				close(sock);
			}
#if ENABLE_BT_PROXY_GATT
			// Nobody is left to drive the links; free them for the next client.
			HAL_BTProxy_GattDisconnectAll();
#endif
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
	if(s_tx_queue == NULL)
	{
		s_tx_queue = xQueueCreate(TX_QUEUE_LEN, sizeof(tx_frame_t*));
	}
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

void ESPHome_API_SetStatusLeds(int red_ch, int blue_ch)
{
	s_led_red_ch = red_ch;
	s_led_blue_ch = blue_ch;
	s_led_red_val = -1;
	s_led_blue_val = -1;
}

static void ESPHome_API_UpdateLed(int ch, int* last, int val)
{
	if(ch < 0) return;
	if(*last == val) return;
	*last = val;
	CHANNEL_Set(ch, val, 0);
}

void DRV_ESPHome_API_OnEverySecond()
{
#if ENABLE_BT_PROXY
	HAL_BTProxy_OnEverySecond();
#endif
	// Blue: Home Assistant connected and subscribed to the proxy.
	ESPHome_API_UpdateLed(s_led_blue_ch, &s_led_blue_val,
		(client_sock != INVALID_SOCK && g_bt_proxy_forwarding_active) ? 1 : 0);
	// Red: off = no BLE link, blinking = connecting/discovering, on = connected.
	int red = 0;
#if ENABLE_BT_PROXY_GATT
	int link = HAL_BTProxy_GattGetLinkState();
	if(link == 2) red = 1;
	else if(link == 1) red = (s_led_red_val == 1) ? 0 : 1;
#endif
	ESPHome_API_UpdateLed(s_led_red_ch, &s_led_red_val, red);
}

#endif // ENABLE_DRIVER_ESPHOME_API
