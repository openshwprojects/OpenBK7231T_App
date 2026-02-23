#include "../new_common.h"
#include "../logging/logging.h"
#include "drv_bt_proxy.h"
#include "drv_bt_proxy_api.h"
#include "../hal/hal_bt_proxy.h"
#include <lwip/sockets.h>

#if ENABLE_DRIVER_BT_PROXY

// Global state tracking whether the client requested BLE advertisement forwarding
int g_bt_proxy_forwarding_active = 0;

#define BTProxy_PORT 6053
#define INVALID_SOCK -1

#if PLATFORM_ESPIDF
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#endif

extern int Main_HasWiFiConnected();

static int s_listen_sock = INVALID_SOCK;
static xTaskHandle s_BTProxy_thread = NULL;

static void BTProxy_TCP_Server_Thread(void* param) {
    int reuse = 1;
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(BTProxy_PORT);

    if (s_listen_sock != INVALID_SOCK) {
        close(s_listen_sock);
    }

    s_listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s_listen_sock < 0) {
        ADDLOG_ERROR(LOG_FEATURE_DRV, "BTProxy: Unable to create socket");
        rtos_suspend_thread(NULL);
        return;
    }

    setsockopt(s_listen_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse));

    if (bind(s_listen_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) != 0) {
        ADDLOG_ERROR(LOG_FEATURE_DRV, "BTProxy: Socket unable to bind to port %d", BTProxy_PORT);
        close(s_listen_sock);
        s_listen_sock = INVALID_SOCK;
        rtos_suspend_thread(NULL);
        return;
    }

    if (listen(s_listen_sock, 2) != 0) {
        ADDLOG_ERROR(LOG_FEATURE_DRV, "BTProxy: Error occurred during listen");
        close(s_listen_sock);
        s_listen_sock = INVALID_SOCK;
        rtos_suspend_thread(NULL);
        return;
    }

    ADDLOG_INFO(LOG_FEATURE_DRV, "BTProxy: TCP Server listening on port %d", BTProxy_PORT);

    while (1) {
        struct sockaddr_storage source_addr;
        socklen_t addr_len = sizeof(source_addr);
        int client_sock = accept(s_listen_sock, (struct sockaddr*)&source_addr, &addr_len);
        
        if (client_sock != INVALID_SOCK) {
            ADDLOG_INFO(LOG_FEATURE_DRV, "BTProxy: Client connected (sock %d)", client_sock);
            
            // ESPHome Native API Framing Decoder
            unsigned char buffer[1024];
            int read_len = 0;
            
            while (Main_HasWiFiConnected()) {
                int ret = recv(client_sock, buffer + read_len, sizeof(buffer) - read_len, 0);
                if (ret > 0) {
                    read_len += ret;
                    
                    // Parse framing
                    while (read_len > 0) {
                        if (buffer[0] != 0x00) {
                            ADDLOG_ERROR(LOG_FEATURE_DRV, "BTProxy: Invalid preamble (0x%02X)", buffer[0]);
                            goto disconnect;
                        }
                        
                        // Need enough bytes for 1 preamble + at least 2 varints
                        if (read_len < 3) break; 
                        
                        uint8_t *p = buffer + 1;
                        uint32_t msg_size = 0, msg_type = 0;
                        int shift = 0;
                        
                        // varint size
                        while (p < buffer + read_len) {
                            msg_size |= (*p & 0x7F) << shift;
                            shift += 7;
                            if (!(*p++ & 0x80)) break;
                        }
                        // varint type
                        shift = 0;
                        while (p < buffer + read_len) {
                            msg_type |= (*p & 0x7F) << shift;
                            shift += 7;
                            if (!(*p++ & 0x80)) break;
                        }
                        
                        int header_len = p - buffer;
                        
                        // Check if the full message has arrived based on msg_size
                        if (read_len >= header_len + (int)msg_size) {
                            // Full frame received, process it
                            BTProxy_Process_Packet(client_sock, msg_type, p, msg_size);
                            
                            // Consume bytes and shift buffer left
                            int consumed = header_len + msg_size;
                            read_len -= consumed;
                            if (read_len > 0) {
                                memmove(buffer, buffer + consumed, read_len);
                            }
                        } else {
                            if (header_len + msg_size > sizeof(buffer)) {
                                ADDLOG_ERROR(LOG_FEATURE_DRV, "BTProxy: Message exceeds buffer! (%d)", msg_size);
                                goto disconnect;
                            }
                            break; // Wait for more data to complete the frame
                        }
                    }
                } else if (ret == -1 && errno == EAGAIN) {
                    rtos_delay_milliseconds(10);
                    continue;
                } else {
                    break; // Error or closed
                }
                
                // Flush pending BLE scans from HAL to TCP socket if requested
                if (g_bt_proxy_forwarding_active) {
                    uint8_t mac_buf[6];
                    int rssi_buf;
                    uint8_t addr_type_buf;
                    uint8_t data_buf[62];
                    int data_len_buf;
                    
                    // Pop up to 10 packets per loop iteration to drain the ringbuffer quickly
                    for(int i = 0; i < 10; i++) {
                        if (HAL_BTProxy_PopScanResult(mac_buf, &rssi_buf, &addr_type_buf, data_buf, &data_len_buf)) {
                            BTProxy_Hook_ScanResult(client_sock, mac_buf, rssi_buf, addr_type_buf, data_buf, data_len_buf);
                        } else {
                            break; // Ring buffer empty
                        }
                    }
                }
            }
        disconnect:
            ADDLOG_INFO(LOG_FEATURE_DRV, "BTProxy: Client disconnected or error");
            g_bt_proxy_forwarding_active = 0;
            close(client_sock);
        }
        rtos_delay_milliseconds(50);
    }

    rtos_suspend_thread(NULL);
}

void DRV_BT_Proxy_Init() {
    OSStatus err;
    if (s_BTProxy_thread != NULL) {
        rtos_delete_thread(&s_BTProxy_thread);
        s_BTProxy_thread = NULL;
    }

    err = rtos_create_thread(&s_BTProxy_thread, BEKEN_APPLICATION_PRIORITY - 1,
                             "BTProxy_Srv",
                             (beken_thread_function_t)BTProxy_TCP_Server_Thread,
                             4096,
                             (beken_thread_arg_t)0);
                             
    if (err != kNoErr) {
        ADDLOG_ERROR(LOG_FEATURE_DRV, "BTProxy: create server thread failed with %d", err);
    } else {
        ADDLOG_INFO(LOG_FEATURE_DRV, "BTProxy: server thread started.");
    }
}

#endif // ENABLE_DRIVER_BT_PROXY
