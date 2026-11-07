#ifndef __DRV_BT_PROXY_API_H__
#define __DRV_BT_PROXY_API_H__

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// ESPHome Message Types
#define ESPHOME_MSG_HelloRequest 1
#define ESPHOME_MSG_HelloResponse 2
#define ESPHOME_MSG_ConnectRequest 3
#define ESPHOME_MSG_ConnectResponse 4
#define ESPHOME_MSG_DisconnectRequest 5
#define ESPHOME_MSG_DisconnectResponse 6
#define ESPHOME_MSG_PingRequest 7
#define ESPHOME_MSG_PingResponse 8
#define ESPHOME_MSG_DeviceInfoRequest 9
#define ESPHOME_MSG_DeviceInfoResponse 10
#define ESPHOME_MSG_ListEntitiesRequest 11
#define ESPHOME_MSG_ListEntitiesDoneResponse 19

#define ESPHOME_MSG_SubscribeStatesRequest 20
#define ESPHOME_MSG_SubscribeLogsRequest 28
#define ESPHOME_MSG_SubscribeHomeassistantServicesRequest 34
#define ESPHOME_MSG_SubscribeHomeAssistantStatesRequest 38

#define ESPHOME_MSG_SubscribeBluetoothLEAdvertisementsRequest 66
#define ESPHOME_MSG_BluetoothDeviceRequest 68
#define ESPHOME_MSG_BluetoothDeviceConnectionResponse 69
#define ESPHOME_MSG_BluetoothGATTGetServicesRequest 70
#define ESPHOME_MSG_BluetoothGATTGetServicesResponse 71
#define ESPHOME_MSG_BluetoothGATTGetServicesDoneResponse 72
#define ESPHOME_MSG_BluetoothGATTReadRequest 73
#define ESPHOME_MSG_BluetoothGATTReadResponse 74
#define ESPHOME_MSG_BluetoothGATTWriteRequest 75
#define ESPHOME_MSG_BluetoothGATTReadDescriptorRequest 76
#define ESPHOME_MSG_BluetoothGATTWriteDescriptorRequest 77
#define ESPHOME_MSG_BluetoothGATTNotifyRequest 78
#define ESPHOME_MSG_BluetoothGATTNotifyDataResponse 79
#define ESPHOME_MSG_SubscribeBluetoothConnectionsFreeRequest 80
#define ESPHOME_MSG_BluetoothConnectionsFreeResponse 81
#define ESPHOME_MSG_BluetoothGATTErrorResponse 82
#define ESPHOME_MSG_BluetoothGATTWriteResponse 83
#define ESPHOME_MSG_BluetoothGATTNotifyResponse 84
#define ESPHOME_MSG_BluetoothDevicePairingResponse 85
#define ESPHOME_MSG_BluetoothDeviceUnpairingResponse 86
#define ESPHOME_MSG_UnsubscribeBluetoothLEAdvertisementsRequest 87
#define ESPHOME_MSG_BluetoothDeviceClearCacheResponse 88
#define ESPHOME_MSG_BluetoothLERawAdvertisementsResponse 93
#define ESPHOME_MSG_BluetoothScannerStateResponse 126

// BluetoothDeviceRequestType
#define ESPHOME_BT_REQ_CONNECT 0
#define ESPHOME_BT_REQ_DISCONNECT 1
#define ESPHOME_BT_REQ_PAIR 2
#define ESPHOME_BT_REQ_UNPAIR 3
#define ESPHOME_BT_REQ_CONNECT_V3_WITH_CACHE 4
#define ESPHOME_BT_REQ_CONNECT_V3_WITHOUT_CACHE 5
#define ESPHOME_BT_REQ_CLEAR_CACHE 6

// Thread-safe frame send to the connected client (serialised by a mutex).
// droppable: if the socket is congested before anything was written, drop instead of waiting.
bool ESPHome_API_SendFrame(uint32_t msg_type, const uint8_t* payload, uint32_t payload_len, bool droppable);
// Server thread only (replying to a request on that thread's socket).
bool ESPHome_API_SendFrameNow(int sock, uint32_t msg_type, const uint8_t* payload, uint32_t payload_len);
bool ESPHome_API_IsClientConnected(void);
void ESPHome_API_SetStatusLeds(int red_ch, int blue_ch);

// Callbacks from the BT HAL (active connections). Called from the BT task.
void ESPHome_API_BT_OnConnection(uint64_t address, bool connected, uint16_t mtu, int error);
void ESPHome_API_BT_OnService(uint64_t address, const void* service /* bt_gatt_service_t */);
void ESPHome_API_BT_OnServicesDone(uint64_t address);
void ESPHome_API_BT_OnRead(uint64_t address, uint16_t handle, const uint8_t* data, uint16_t len);
void ESPHome_API_BT_OnWrite(uint64_t address, uint16_t handle);
void ESPHome_API_BT_OnNotifyAck(uint64_t address, uint16_t handle);
void ESPHome_API_BT_OnNotifyData(uint64_t address, uint16_t handle, const uint8_t* data, uint16_t len);
void ESPHome_API_BT_OnError(uint64_t address, uint16_t handle, int error);
void ESPHome_API_BT_OnConnectionsFreeChanged(void);

// API Handlers (Called by TCP Server Thread)
void ESPHome_API_Process_Packet(int client_sock, uint32_t type, uint8_t *payload, uint32_t size);
void ESPHome_API_Send_HelloResponse(int client_sock);
void ESPHome_API_Send_ConnectResponse(int client_sock);
void ESPHome_API_Send_PingResponse(int client_sock);
void ESPHome_API_Send_DeviceInfoResponse(int client_sock);
void ESPHome_API_Send_ListEntitiesDoneResponse(int client_sock);

bool ESPHome_API_Hook_ScanResult(int client_sock, const uint8_t *mac, int rssi, uint8_t addr_type, const uint8_t *data, int data_len);
bool ESPHome_API_PassScanResult(const uint8_t* mac, int rssi, uint8_t addr_type, const uint8_t* data, int data_len);

#endif // __DRV_BT_PROXY_API_H__
