#include "../new_common.h"
#include "../logging/logging.h"
#include "drv_esphome_api.h"
#include "../hal/hal_wifi.h"
#include <lwip/sockets.h>
#include "../new_cfg.h"
#include "../hal/hal_bt_proxy.h"

#if ENABLE_DRIVER_ESPHOME_API

#define INVALID_SOCK -1

extern bool g_bt_proxy_forwarding_active;

// Minimalist Varint encoder
static void encode_varint(uint8_t** buf, uint64_t value)
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

static int pb_decode_varint(uint8_t** buf, int* len, uint64_t* val)
{
	uint64_t result = 0;
	int shift = 0;
	while(*len > 0)
	{
		uint8_t byte = **buf;
		(*buf)++;
		(*len)--;
		result |= (uint64_t)(byte & 0x7F) << shift;
		shift += 7;
		if(!(byte & 0x80))
		{
			*val = result;
			return 1;
		}
	}
	return 0;
}

static void pb_encode_varint_field(uint8_t** buf, int field_idx, uint64_t value)
{
	encode_varint(buf, (field_idx << 3) | 0); // Wire Type 0
	encode_varint(buf, value);
}

static void pb_encode_string_field(uint8_t** buf, int field_idx, const char* str)
{
	if(!str) return;
	int len = strlen(str);
	encode_varint(buf, (field_idx << 3) | 2); // Wire Type 2
	encode_varint(buf, len);
	memcpy(*buf, str, len);
	*buf += len;
}

static void pb_encode_bytes_field(uint8_t** buf, int field_idx, const uint8_t* data, int len)
{
	if(!data || len == 0) return;
	encode_varint(buf, (field_idx << 3) | 2); // Wire Type 2
	encode_varint(buf, len);
	memcpy(*buf, data, len);
	*buf += len;
}

static void PB_SendFrame(int client_sock, uint32_t msg_type, uint8_t* payload, uint32_t payload_len)
{
	uint8_t header[16];
	uint8_t* ptr = header;
	*ptr++ = 0x00; // Preamble
	encode_varint(&ptr, payload_len);
	encode_varint(&ptr, msg_type);

	int header_len = ptr - header;

	// Send Header
	send(client_sock, header, header_len, 0);
	// Send Payload
	if(payload_len > 0)
	{
		send(client_sock, payload, payload_len, 0);
	}
}

void ESPHome_API_Send_HelloResponse(int client_sock)
{
	uint8_t payload[256];
	uint8_t* ptr = payload;
	pb_encode_varint_field(&ptr, 1, 1); // API Version Major
	pb_encode_varint_field(&ptr, 2, 9); // API Version Minor
	pb_encode_string_field(&ptr, 3, "OpenBeken"); // Server Info
	pb_encode_string_field(&ptr, 4, CFG_GetDeviceName());   // Name

	PB_SendFrame(client_sock, ESPHOME_MSG_HelloResponse, payload, ptr - payload);
}

void ESPHome_API_Send_ConnectResponse(int client_sock)
{
	uint8_t payload[16];
	uint8_t* ptr = payload;
	pb_encode_varint_field(&ptr, 1, 0); // invalid_password = false

	PB_SendFrame(client_sock, ESPHOME_MSG_ConnectResponse, payload, ptr - payload);
}

void ESPHome_API_Send_PingResponse(int client_sock)
{
	PB_SendFrame(client_sock, ESPHOME_MSG_PingResponse, NULL, 0);
}

void ESPHome_API_Send_DeviceInfoResponse(int client_sock)
{
	uint8_t payload[512];
	uint8_t* ptr = payload;
	char mac_str[32];

	// Retrieve device MAC address dynamically
	HAL_GetMACStr(mac_str);

	pb_encode_string_field(&ptr, 2, CFG_GetDeviceName()); // Name
	pb_encode_string_field(&ptr, 3, mac_str); // Mac address
	pb_encode_string_field(&ptr, 4, USER_SW_VER); // Firmware Version
	pb_encode_string_field(&ptr, 6, PLATFORM_MCU_NAME); // Model
	pb_encode_string_field(&ptr, 12, MANUFACTURER); // Manufacturer
	pb_encode_string_field(&ptr, 13, CFG_GetDeviceName()); // friendly_name

#if ENABLE_BT_PROXY
	// Feature flags (ESPHome 2024.1+):
	// 1 (Passive Scan), 2 (Active Connections), 32 (Raw Advertisements), 64 (State and Mode)
	// 1 | 2 | 32 | 64  = 99
	if(HAL_BTProxy_IsInit())
	{
#if PLATFORM_ESPIDF
		uint64_t flags = 2 | 4 | 32 | 64;
#else
		uint64_t flags = 32 | 64;
#endif
		flags |= HAL_BTProxy_GetScanMode();
		pb_encode_varint_field(&ptr, 15, flags); // bluetooth_proxy_feature_flags
		uint8_t btmac[6];
		HAL_BTProxy_GetMAC(btmac);
		sprintf(mac_str, MACSTR, MAC2STR(btmac));
		pb_encode_string_field(&ptr, 18, mac_str); // bluetooth_mac_address
	}
#endif

	PB_SendFrame(client_sock, ESPHOME_MSG_DeviceInfoResponse, payload, ptr - payload);
}

void ESPHome_API_Send_ListEntitiesDoneResponse(int client_sock)
{
	// Empty payload for ListEntitiesDoneResponse
	PB_SendFrame(client_sock, ESPHOME_MSG_ListEntitiesDoneResponse, NULL, 0);
}

void ESPHome_API_Send_DisconnectResponse(int client_sock)
{
	PB_SendFrame(client_sock, ESPHOME_MSG_DisconnectResponse, NULL, 0);
}

void ESPHome_API_Send_ScannerStateResponse(int client_sock)
{
	uint8_t payload[32];
	uint8_t* ptr = payload;

	// state (1): 2 = RUNNING
	// mode (2): 1 = ACTIVE
	// configured_mode (3): 1 = ACTIVE
	pb_encode_varint_field(&ptr, 1, 2);
	pb_encode_varint_field(&ptr, 2, 1);
	pb_encode_varint_field(&ptr, 3, 1);

	PB_SendFrame(client_sock, ESPHOME_MSG_BluetoothScannerStateResponse, payload, ptr - payload);
}

void ESPHome_API_Send_ConnectionsFreeResponse(int client_sock)
{
	uint8_t payload[32];
	uint8_t* ptr = payload;

	// free (1): 3 connections free
	// limit (2): 3 connections max
	pb_encode_varint_field(&ptr, 1, 3);
	pb_encode_varint_field(&ptr, 2, 3);

	PB_SendFrame(client_sock, ESPHOME_MSG_BluetoothConnectionsFreeResponse, payload, ptr - payload);
}

void ESPHome_API_Process_Packet(int client_sock, uint32_t type, uint8_t* payload, uint32_t size)
{
	ADDLOG_EXTRADEBUG(LOG_FEATURE_DRV, "ESPHomeAPI: Received API Msg %d (Len: %d)", type, size);
	switch(type)
	{
		case ESPHOME_MSG_HelloRequest:
			ADDLOG_EXTRADEBUG(LOG_FEATURE_DRV, "ESPHomeAPI: Handshake -> HelloRequest");
			ESPHome_API_Send_HelloResponse(client_sock);
			break;
		case ESPHOME_MSG_ConnectRequest:
			ADDLOG_EXTRADEBUG(LOG_FEATURE_DRV, "ESPHomeAPI: Handshake -> ConnectRequest");
			ESPHome_API_Send_ConnectResponse(client_sock);
			break;
		case ESPHOME_MSG_PingRequest:
			ESPHome_API_Send_PingResponse(client_sock);
			break;
		case ESPHOME_MSG_DeviceInfoRequest:
			ADDLOG_EXTRADEBUG(LOG_FEATURE_DRV, "ESPHomeAPI: Handshake -> DeviceInfoRequest");
			ESPHome_API_Send_DeviceInfoResponse(client_sock);
			break;
		case ESPHOME_MSG_ListEntitiesRequest:
			ADDLOG_EXTRADEBUG(LOG_FEATURE_DRV, "ESPHomeAPI: Handshake -> ListEntitiesRequest (Sending Done)");
			// We only expose BT Proxy, no manual entities right now. Just send Done.
			ESPHome_API_Send_ListEntitiesDoneResponse(client_sock);
			break;
		case ESPHOME_MSG_DisconnectRequest:
			ADDLOG_EXTRADEBUG(LOG_FEATURE_DRV, "ESPHomeAPI: Handshake -> DisconnectRequest");
			ESPHome_API_Send_DisconnectResponse(client_sock);
			break;
		case ESPHOME_MSG_SubscribeStatesRequest:
		case ESPHOME_MSG_SubscribeLogsRequest:
		case ESPHOME_MSG_SubscribeHomeassistantServicesRequest:
		case ESPHOME_MSG_SubscribeHomeAssistantStatesRequest:
			ADDLOG_EXTRADEBUG(LOG_FEATURE_DRV, "ESPHomeAPI: Handshake -> Ignored basic HA Subscribe form (%d)", type);
			break;
		case ESPHOME_MSG_SubscribeBluetoothLEAdvertisementsRequest:
			ADDLOG_EXTRADEBUG(LOG_FEATURE_DRV, "ESPHomeAPI: Home Assistant subscribed to BLE advertisements");
			g_bt_proxy_forwarding_active = true;

			// Immediately follow up with ScannerStateResponse (Msg 126) to tell HA we are actively scanning
			// HA won't trust "Scanning" status unless this is provided during the subscription event!
			ESPHome_API_Send_ScannerStateResponse(client_sock);
			break;
		case ESPHOME_MSG_SubscribeBluetoothConnectionsFreeRequest:
			ADDLOG_EXTRADEBUG(LOG_FEATURE_DRV, "ESPHomeAPI: Handshake -> SubscribeBluetoothConnectionsFreeRequest");
#if PLATFORM_ESPIDF
			ESPHome_API_Send_ConnectionsFreeResponse(client_sock);
#endif
			break;
		case ESPHOME_MSG_BluetoothDeviceRequest:
		{
			uint64_t address = 0;
			uint64_t request_type = 0;
			uint8_t* p = payload;
			int l = size;
			while(l > 0)
			{
				uint64_t tag;
				if(!pb_decode_varint(&p, &l, &tag)) break;
				int wire_type = tag & 0x07;
				int field_idx = tag >> 3;

				if(wire_type == 0)
				{
					uint64_t val;
					if(!pb_decode_varint(&p, &l, &val)) break;
					if(field_idx == 1) address = val;
					else if(field_idx == 2) request_type = val;
				}
				else if(wire_type == 2)
				{
					uint64_t len_val;
					if(!pb_decode_varint(&p, &l, &len_val)) break;
					p += len_val;
					l -= len_val;
				}
				else if(wire_type == 1 || wire_type == 5)
				{
					int skip = (wire_type == 1) ? 8 : 4;
					p += skip;
					l -= skip;
				}
			}
			ADDLOG_EXTRADEBUG(LOG_FEATURE_DRV, "ESPHomeAPI: HA DeviceRequest MAC: %02X:%02X:%02X:%02X:%02X:%02X Type: %d",
				(uint8_t)(address >> 40), (uint8_t)(address >> 32), (uint8_t)(address >> 24),
				(uint8_t)(address >> 16), (uint8_t)(address >> 8), (uint8_t)address,
				(int)request_type);
			break;
		}
		default:
			ADDLOG_EXTRADEBUG(LOG_FEATURE_DRV, "ESPHomeAPI: Unhandled message type %d", type);
			break;
	}
}

// Forward a BLE scan result to the connected ESPHome client
bool ESPHome_API_Hook_ScanResult(int client_sock, const uint8_t* mac, int rssi, uint8_t addr_type, const uint8_t* data, int data_len)
{
	if(client_sock == INVALID_SOCK || g_bt_proxy_forwarding_active == false) return false;
	if(!mac || !data || data_len <= 0) return false;

	// ESPHome Msg 93 (BluetoothLERawAdvertisementsResponse)
	// repeated BluetoothLERawAdvertisement advertisements = 1;

	// BluetoothLERawAdvertisement:
	// uint64 address = 1;
	// sint32 rssi = 2;
	// uint32 address_type = 3;
	// bytes data = 4;

	uint8_t payload[512];
	uint8_t* ptr = payload;

	// Convert MAC to uint64
	uint64_t mac_u64 = 0;
	for(int i = 0; i < 6; i++) mac_u64 |= ((uint64_t)mac[i]) << (i * 8);

	// Encode the single Advertisement message into a sub-buffer
	uint8_t adv_msg[256];
	uint8_t* adv_ptr = adv_msg;

	pb_encode_varint_field(&adv_ptr, 1, mac_u64);

	// RSSI (sint32 uses ZigZag encoding in protobuf)
	uint32_t zigzag_rssi = (rssi << 1) ^ (rssi >> 31);
	pb_encode_varint_field(&adv_ptr, 2, zigzag_rssi);

	pb_encode_varint_field(&adv_ptr, 3, addr_type);
	pb_encode_bytes_field(&adv_ptr, 4, data, data_len);

	// Add to main payload as repeated field (Wire Type 2 = Length Delimited)
	pb_encode_bytes_field(&ptr, 1, adv_msg, adv_ptr - adv_msg);

	PB_SendFrame(client_sock, ESPHOME_MSG_BluetoothLERawAdvertisementsResponse, payload, ptr - payload);
	return true;
}

#endif // ENABLE_DRIVER_ESPHOME_API
