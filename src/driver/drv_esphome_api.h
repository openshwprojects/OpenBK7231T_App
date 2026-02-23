#ifndef __DRV_BT_PROXY_API_H__
#define __DRV_BT_PROXY_API_H__

#include <stdint.h>
#include <stddef.h>

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
#define ESPHOME_MSG_SubscribeBluetoothConnectionsFreeRequest 80
#define ESPHOME_MSG_BluetoothConnectionsFreeResponse 81
#define ESPHOME_MSG_BluetoothLERawAdvertisementsResponse 93
#define ESPHOME_MSG_BluetoothScannerStateResponse 126

// API Handlers (Called by TCP Server Thread)
void BTProxy_Process_Packet(int client_sock, uint32_t type, uint8_t *payload, uint32_t size);
void BTProxy_Send_HelloResponse(int client_sock);
void BTProxy_Send_ConnectResponse(int client_sock);
void BTProxy_Send_PingResponse(int client_sock);
void BTProxy_Send_DeviceInfoResponse(int client_sock);
void BTProxy_Send_ListEntitiesDoneResponse(int client_sock);

void BTProxy_Hook_ScanResult(int client_sock, const uint8_t *mac, int rssi, uint8_t addr_type, const uint8_t *data, int data_len);

#endif // __DRV_BT_PROXY_API_H__
