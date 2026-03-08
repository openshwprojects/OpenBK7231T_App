#ifndef __HAL_BT_PROXY_H__
#define __HAL_BT_PROXY_H__

#include "../obk_config.h"

#if ENABLE_BT_PROXY

#include <stdint.h>
#include <stdbool.h>

extern bool g_bt_proxy_forwarding_active;

#define MAC2STRINV(a) (a)[5], (a)[4], (a)[3], (a)[2], (a)[1], (a)[0]

// Max simultaneous connections (typical constraints: ESP32 ~3-5, ESP32-C3 ~4)
#define BT_PROXY_MAX_CONNECTIONS 3

// Defines the types of operations pushed to the BT Command Queue
//typedef enum {
//    BT_CMD_CONNECT,
//    BT_CMD_DISCONNECT,
//    BT_CMD_READ,
//    BT_CMD_WRITE,
//    BT_CMD_SUBSCRIBE,
//    BT_CMD_UNSUBSCRIBE
//} bt_proxy_cmd_type_t;

// A generic command structure enqueued by the HA bridge (e.g. ESPHome Protobuf)
// and executed by the platform-specific Bluetooth task (e.g. ESP-IDF GAP/GATTC)
//typedef struct {
//    bt_proxy_cmd_type_t type;
//    uint8_t mac[6];           // Target device address
//    uint8_t address_type;     // 0=Public, 1=Random
//    uint16_t handle;          // GATT Handle to read/write/subscribe
//    uint8_t *data;            // Payload for writes (dynamically matched, must be freed by consumer)
//    uint16_t data_len;        // Length of payload
//    void *user_data;          // Optional pointer for completion callbacks (e.g. TCP client reference)
//} bt_proxy_cmd_t;

// State tracking for an active BT connection slot
//typedef enum {
//    BT_CONN_STATE_DISCONNECTED,
//    BT_CONN_STATE_CONNECTING,
//    BT_CONN_STATE_CONNECTED_IDLE,
//    BT_CONN_STATE_READING,
//    BT_CONN_STATE_WRITING
//} bt_conn_state_t;

//typedef struct {
//    bt_conn_state_t state;
//    uint8_t remote_mac[6];
//    uint16_t platform_conn_id;  // E.g., ESP-IDF gattc_if or conn_id
//    uint32_t last_activity_ms;  // Used for watchdog/timeout disconnecting stale peers
//} bt_proxy_conn_slot_t;

bool HAL_BTProxy_IsInit(void);
void HAL_BTProxy_Init(void);
void HAL_BTProxy_Deinit(void);
void HAL_BTProxy_StartScan();
void HAL_BTProxy_StopScan();
void HAL_BTProxy_GetMAC(uint8_t* mac);
void HAL_BTProxy_SetScanMode(bool isActive);
// true - active, false - passive
bool HAL_BTProxy_GetScanMode(void);
// in ms
void HAL_BTProxy_SetWindowInterval(uint16_t window, uint16_t interval);

void HAL_BTProxy_OnEverySecond(void);
// Send a command to the proxy core queue
// Returns 0 on success, or an error code if the queue is full
//int HAL_BTProxy_EnqueueCommand(bt_proxy_cmd_t *cmd);

int HAL_BTProxy_GetScanStats(int* init_done, int* scan_active, int* total_packets, int* dropped_packets);
void HAL_BTProxy_Lock(void);
void HAL_BTProxy_Unlock(void);
void HAL_BTProxy_RegisterPlatformCommands(void);
void HAL_BTProxy_RegisterCommands(void);

#endif // ENABLE_BT_PROXY
#endif // __HAL_BT_PROXY_H__
