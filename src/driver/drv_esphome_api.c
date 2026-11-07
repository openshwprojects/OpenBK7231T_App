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

// client_sock >= 0 means we are on the API server thread replying to a request;
// anything else (e.g. -1 from the BT task) goes through the TX queue.
static void PB_SendFrame(int client_sock, uint32_t msg_type, uint8_t* payload, uint32_t payload_len)
{
	if(client_sock >= 0) ESPHome_API_SendFrameNow(client_sock, msg_type, payload, payload_len);
	else ESPHome_API_SendFrame(msg_type, payload, payload_len, false);
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
#if PLATFORM_ESPIDF || ENABLE_BT_PROXY_GATT
		// 2 = ACTIVE_CONNECTIONS, 4 = REMOTE_CACHING (HA writes CCCDs itself)
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
	uint8_t payload[64];
	uint8_t* ptr = payload;

#if ENABLE_BT_PROXY_GATT
	uint64_t allocated[BT_PROXY_GATT_MAX_LINKS];
	int limit = 0;
	int used = HAL_BTProxy_GattGetAllocated(allocated, BT_PROXY_GATT_MAX_LINKS, &limit);
	pb_encode_varint_field(&ptr, 1, limit - used); // free
	pb_encode_varint_field(&ptr, 2, limit);        // limit
	for(int i = 0; i < used; i++)
	{
		pb_encode_varint_field(&ptr, 3, allocated[i]); // allocated (repeated)
	}
#else
	// free (1): 3 connections free
	// limit (2): 3 connections max
	pb_encode_varint_field(&ptr, 1, 3);
	pb_encode_varint_field(&ptr, 2, 3);
#endif

	PB_SendFrame(client_sock, ESPHOME_MSG_BluetoothConnectionsFreeResponse, payload, ptr - payload);
}

// ---------------------------------------------------------------------------
// Minimal protobuf message parser: collects varint and length-delimited
// fields by field number (1..PB_MAX_FIELDS).
// ---------------------------------------------------------------------------
#define PB_MAX_FIELDS 8
typedef struct
{
	uint64_t varint[PB_MAX_FIELDS + 1];
	const uint8_t* bytes[PB_MAX_FIELDS + 1];
	uint32_t bytes_len[PB_MAX_FIELDS + 1];
} pb_fields_t;

static bool pb_parse_fields(uint8_t* payload, int size, pb_fields_t* out)
{
	memset(out, 0, sizeof(*out));
	uint8_t* p = payload;
	int l = size;
	while(l > 0)
	{
		uint64_t tag;
		if(!pb_decode_varint(&p, &l, &tag)) return false;
		int wire_type = tag & 0x07;
		int field_idx = (int)(tag >> 3);
		if(wire_type == 0)
		{
			uint64_t val;
			if(!pb_decode_varint(&p, &l, &val)) return false;
			if(field_idx > 0 && field_idx <= PB_MAX_FIELDS) out->varint[field_idx] = val;
		}
		else if(wire_type == 2)
		{
			uint64_t len_val;
			if(!pb_decode_varint(&p, &l, &len_val)) return false;
			if((int)len_val > l) return false;
			if(field_idx > 0 && field_idx <= PB_MAX_FIELDS)
			{
				out->bytes[field_idx] = p;
				out->bytes_len[field_idx] = (uint32_t)len_val;
			}
			p += len_val;
			l -= (int)len_val;
		}
		else if(wire_type == 1 || wire_type == 5)
		{
			int skip = (wire_type == 1) ? 8 : 4;
			if(skip > l) return false;
			p += skip;
			l -= skip;
		}
		else
		{
			return false;
		}
	}
	return true;
}

static void ESPHome_API_Send_AddrResult(uint32_t msg_type, uint64_t address, bool ok, int error)
{
	uint8_t payload[32];
	uint8_t* ptr = payload;
	pb_encode_varint_field(&ptr, 1, address);
	if(ok) pb_encode_varint_field(&ptr, 2, 1);
	if(error) pb_encode_varint_field(&ptr, 3, (uint32_t)error);
	ESPHome_API_SendFrame(msg_type, payload, ptr - payload, false);
}

// Also used by the non-GATT fallback (requests rejected with an ATT error).
void ESPHome_API_BT_OnError(uint64_t address, uint16_t handle, int error)
{
	uint8_t payload[32];
	uint8_t* ptr = payload;
	pb_encode_varint_field(&ptr, 1, address);
	pb_encode_varint_field(&ptr, 2, handle);
	pb_encode_varint_field(&ptr, 3, (uint32_t)error);
	ADDLOG_INFO(LOG_FEATURE_DRV, "ESPHomeAPI: GATT error handle 0x%04X err %d", handle, error);
	ESPHome_API_SendFrame(ESPHOME_MSG_BluetoothGATTErrorResponse, payload, ptr - payload, false);
}

#if ENABLE_BT_PROXY_GATT

static void ESPHome_API_Send_AddrHandle(uint32_t msg_type, uint64_t address, uint16_t handle)
{
	uint8_t payload[32];
	uint8_t* ptr = payload;
	pb_encode_varint_field(&ptr, 1, address);
	pb_encode_varint_field(&ptr, 2, handle);
	ESPHome_API_SendFrame(msg_type, payload, ptr - payload, false);
}

void ESPHome_API_BT_OnConnection(uint64_t address, bool connected, uint16_t mtu, int error)
{
	uint8_t payload[32];
	uint8_t* ptr = payload;
	pb_encode_varint_field(&ptr, 1, address);
	if(connected) pb_encode_varint_field(&ptr, 2, 1);
	if(mtu) pb_encode_varint_field(&ptr, 3, mtu);
	// int32 (not sint32) on the wire: negative values would be 10 bytes; errors are positive here
	if(error) pb_encode_varint_field(&ptr, 4, (uint32_t)error);
	ADDLOG_INFO(LOG_FEATURE_DRV, "ESPHomeAPI: BLE %012llX %s (mtu %d, err %d)",
		(unsigned long long)address, connected ? "connected" : "disconnected", mtu, error);
	ESPHome_API_SendFrame(ESPHOME_MSG_BluetoothDeviceConnectionResponse, payload, ptr - payload, false);
}

// 128-bit UUID (LE byte array) -> ESPHome repeated uint64 [high, low]
static void pb_encode_uuid(uint8_t** buf, int field_idx, const uint8_t* uuid)
{
	uint64_t high = 0, low = 0;
	for(int i = 15; i >= 8; i--) high = (high << 8) | uuid[i];
	for(int i = 7; i >= 0; i--) low = (low << 8) | uuid[i];
	pb_encode_varint_field(buf, field_idx, high);
	pb_encode_varint_field(buf, field_idx, low);
}

// Only the BT task builds service messages, so static buffers are safe.
static uint8_t s_svc_payload[1400];
static uint8_t s_svc_tmp[1200];
static uint8_t s_chr_tmp[256];

void ESPHome_API_BT_OnService(uint64_t address, const void* service)
{
	const bt_gatt_service_t* svc = (const bt_gatt_service_t*)service;
	uint8_t* sp = s_svc_tmp;
	uint8_t* send = s_svc_tmp + sizeof(s_svc_tmp);

	pb_encode_uuid(&sp, 1, svc->uuid);
	pb_encode_varint_field(&sp, 2, svc->handle);
	for(int c = 0; c < svc->char_count; c++)
	{
		const bt_gatt_char_t* ch = &svc->chars[c];
		uint8_t* cp = s_chr_tmp;
		pb_encode_uuid(&cp, 1, ch->uuid);
		pb_encode_varint_field(&cp, 2, ch->handle);
		pb_encode_varint_field(&cp, 3, ch->properties);
		for(int d = 0; d < ch->desc_count; d++)
		{
			const bt_gatt_desc_t* de = &svc->descs[ch->desc_first + d];
			uint8_t dbuf[40];
			uint8_t* dp = dbuf;
			pb_encode_uuid(&dp, 1, de->uuid);
			pb_encode_varint_field(&dp, 2, de->handle);
			if(cp + (dp - dbuf) + 4 > s_chr_tmp + sizeof(s_chr_tmp)) break;
			pb_encode_bytes_field(&cp, 4, dbuf, dp - dbuf);
		}
		if(sp + (cp - s_chr_tmp) + 4 > send)
		{
			ADDLOG_ERROR(LOG_FEATURE_DRV, "ESPHomeAPI: service 0x%04X too large, truncated", svc->handle);
			break;
		}
		pb_encode_bytes_field(&sp, 3, s_chr_tmp, cp - s_chr_tmp);
	}

	uint8_t* ptr = s_svc_payload;
	pb_encode_varint_field(&ptr, 1, address);
	pb_encode_bytes_field(&ptr, 2, s_svc_tmp, sp - s_svc_tmp);
	ESPHome_API_SendFrame(ESPHOME_MSG_BluetoothGATTGetServicesResponse, s_svc_payload, ptr - s_svc_payload, false);
}

void ESPHome_API_BT_OnServicesDone(uint64_t address)
{
	uint8_t payload[16];
	uint8_t* ptr = payload;
	pb_encode_varint_field(&ptr, 1, address);
	ESPHome_API_SendFrame(ESPHOME_MSG_BluetoothGATTGetServicesDoneResponse, payload, ptr - payload, false);
}

static uint8_t s_data_payload[600];

static void ESPHome_API_Send_AddrHandleData(uint32_t msg_type, uint64_t address, uint16_t handle, const uint8_t* data, uint16_t len)
{
	if(len > sizeof(s_data_payload) - 32) len = sizeof(s_data_payload) - 32;
	uint8_t* ptr = s_data_payload;
	pb_encode_varint_field(&ptr, 1, address);
	pb_encode_varint_field(&ptr, 2, handle);
	pb_encode_bytes_field(&ptr, 3, data, len);
	ESPHome_API_SendFrame(msg_type, s_data_payload, ptr - s_data_payload, false);
}

void ESPHome_API_BT_OnRead(uint64_t address, uint16_t handle, const uint8_t* data, uint16_t len)
{
	ESPHome_API_Send_AddrHandleData(ESPHOME_MSG_BluetoothGATTReadResponse, address, handle, data, len);
}

void ESPHome_API_BT_OnNotifyData(uint64_t address, uint16_t handle, const uint8_t* data, uint16_t len)
{
	ESPHome_API_Send_AddrHandleData(ESPHOME_MSG_BluetoothGATTNotifyDataResponse, address, handle, data, len);
}

void ESPHome_API_BT_OnWrite(uint64_t address, uint16_t handle)
{
	ESPHome_API_Send_AddrHandle(ESPHOME_MSG_BluetoothGATTWriteResponse, address, handle);
}

void ESPHome_API_BT_OnNotifyAck(uint64_t address, uint16_t handle)
{
	ESPHome_API_Send_AddrHandle(ESPHOME_MSG_BluetoothGATTNotifyResponse, address, handle);
}

void ESPHome_API_BT_OnConnectionsFreeChanged(void)
{
	ESPHome_API_Send_ConnectionsFreeResponse(-1);
}

#endif // ENABLE_BT_PROXY_GATT

static void ESPHome_API_Process_BT_Request(int client_sock, uint32_t type, uint8_t* payload, uint32_t size)
{
	pb_fields_t f;
	if(!pb_parse_fields(payload, size, &f))
	{
		ADDLOG_ERROR(LOG_FEATURE_DRV, "ESPHomeAPI: malformed BT request %d", type);
		return;
	}
	uint64_t address = f.varint[1];
	uint16_t handle = (uint16_t)f.varint[2];

	if(type == ESPHOME_MSG_BluetoothDeviceRequest)
	{
		int request_type = (int)f.varint[2];
		uint8_t address_type = (uint8_t)f.varint[4];
		ADDLOG_INFO(LOG_FEATURE_DRV, "ESPHomeAPI: DeviceRequest %012llX type %d addr_type %d",
			(unsigned long long)address, request_type, address_type);
		switch(request_type)
		{
#if ENABLE_BT_PROXY_GATT
			case ESPHOME_BT_REQ_CONNECT:
			case ESPHOME_BT_REQ_CONNECT_V3_WITH_CACHE:
			case ESPHOME_BT_REQ_CONNECT_V3_WITHOUT_CACHE:
				if(!HAL_BTProxy_GattConnect(address, address_type))
				{
					ESPHome_API_BT_OnConnection(address, false, 0, 0x0b /* conn limit / busy */);
				}
				break;
			case ESPHOME_BT_REQ_DISCONNECT:
				if(!HAL_BTProxy_GattDisconnect(address))
				{
					ESPHome_API_BT_OnConnection(address, false, 0, 0);
				}
				break;
#else
			case ESPHOME_BT_REQ_CONNECT:
			case ESPHOME_BT_REQ_CONNECT_V3_WITH_CACHE:
			case ESPHOME_BT_REQ_CONNECT_V3_WITHOUT_CACHE:
			{
				uint8_t p[32];
				uint8_t* ptr = p;
				pb_encode_varint_field(&ptr, 1, address);
				pb_encode_varint_field(&ptr, 4, 0x0b);
				ESPHome_API_SendFrame(ESPHOME_MSG_BluetoothDeviceConnectionResponse, p, ptr - p, false);
				break;
			}
#endif
			case ESPHOME_BT_REQ_PAIR:
				ESPHome_API_Send_AddrResult(ESPHOME_MSG_BluetoothDevicePairingResponse, address, false, 1);
				break;
			case ESPHOME_BT_REQ_UNPAIR:
				ESPHome_API_Send_AddrResult(ESPHOME_MSG_BluetoothDeviceUnpairingResponse, address, false, 1);
				break;
			case ESPHOME_BT_REQ_CLEAR_CACHE:
				ESPHome_API_Send_AddrResult(ESPHOME_MSG_BluetoothDeviceClearCacheResponse, address, false, 1);
				break;
			default:
				break;
		}
		return;
	}

#if ENABLE_BT_PROXY_GATT
	bool queued = false;
	switch(type)
	{
		case ESPHOME_MSG_BluetoothGATTGetServicesRequest:
			queued = HAL_BTProxy_GattGetServices(address);
			break;
		case ESPHOME_MSG_BluetoothGATTReadRequest:
		case ESPHOME_MSG_BluetoothGATTReadDescriptorRequest:
			queued = HAL_BTProxy_GattRead(address, handle);
			break;
		case ESPHOME_MSG_BluetoothGATTWriteRequest:
			// response = 3, data = 4
			queued = HAL_BTProxy_GattWrite(address, handle, f.varint[3] != 0, f.bytes[4], (uint16_t)f.bytes_len[4]);
			break;
		case ESPHOME_MSG_BluetoothGATTWriteDescriptorRequest:
			// data = 3, descriptor writes always use Write Request
			queued = HAL_BTProxy_GattWrite(address, handle, true, f.bytes[3], (uint16_t)f.bytes_len[3]);
			break;
		case ESPHOME_MSG_BluetoothGATTNotifyRequest:
			queued = HAL_BTProxy_GattNotify(address, handle, f.varint[3] != 0);
			break;
	}
	if(!queued)
	{
		ESPHome_API_BT_OnError(address, handle, 0x0e /* ATT unlikely error */);
	}
#else
	ESPHome_API_BT_OnError(address, handle, 0x0e);
#endif
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
#if PLATFORM_ESPIDF || ENABLE_BT_PROXY_GATT
			ESPHome_API_Send_ConnectionsFreeResponse(client_sock);
#endif
			break;
		case ESPHOME_MSG_UnsubscribeBluetoothLEAdvertisementsRequest:
			g_bt_proxy_forwarding_active = false;
			break;
		case ESPHOME_MSG_BluetoothDeviceRequest:
		case ESPHOME_MSG_BluetoothGATTGetServicesRequest:
		case ESPHOME_MSG_BluetoothGATTReadRequest:
		case ESPHOME_MSG_BluetoothGATTWriteRequest:
		case ESPHOME_MSG_BluetoothGATTReadDescriptorRequest:
		case ESPHOME_MSG_BluetoothGATTWriteDescriptorRequest:
		case ESPHOME_MSG_BluetoothGATTNotifyRequest:
			ESPHome_API_Process_BT_Request(client_sock, type, payload, size);
			break;
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

	return ESPHome_API_SendFrame(ESPHOME_MSG_BluetoothLERawAdvertisementsResponse, payload, ptr - payload, true);
}

#endif // ENABLE_DRIVER_ESPHOME_API
