//
// Generic GirierMCU information
//

#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
#include "../quicktick.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../logging/logging.h"
#include "../mqtt/new_mqtt.h"
#include "drv_girierMCU.h"
#include "drv_uart.h"

#define GIRIERMCU_BUFFER_SIZE 	256

void GirierMCU_RunFrame();

typedef struct GirierMCUMapping_s {
	// internal Tuya variable index
	byte dpId;
	// data point type (one of the DP_TYPE_xxx defines)
	byte dpType;
	// true if it's supposed to be sent in dp cache
	byte obkFlags;
	// could be renamed to flags later?
	byte inv;
	// target channel
	short channel;
	// store last channel value to avoid sending it again
	int prevValue;
	// allow storing raw data for later usage
	byte *rawData;
	int rawBufferSize;
	int rawDataLen;
	// not really useful as long as we have integer channels
	float mult;
	float delta;
	float delta2;
	float delta3;
	// TODO
	//int mode;
	// list
	struct GirierMCUMapping_s* next;
} girierMCUMapping_t;

girierMCUMapping_t* g_girierMappings = 0;

/**
 * Dimmer range
 *
 * Map OpenBK7231T_App's dimmer range of 0..100 to the dimmer range used by GirierMCU (0..255).
 * 
 */
 // minimum dimmer value as reported by GirierMCU dimmer
static int g_dimmerRangeMin = 0;
// maximum dimmer value as reported by GirierMCU dimmer
static int g_dimmerRangeMax = 100;

// serial baud rate used to communicate with the GirierMCU
// common baud rates are 9600 bit/s and 115200 bit/s
static int g_baudRate = 9600;

// global mcu time
static int g_girierNextRequestDelay;

// ?? it's unused atm
//static char *prod_info = NULL;
static bool working_mode_valid = false;
static bool self_processing_mode = true;

static byte *g_GirierMCUpayloadBuffer = 0;
static int g_GirierMCUpayloadBufferSize = 0;

typedef struct GirierMCUPacket_s {
	byte *data;
	int size;
	int allocated;
	struct GirierMCUPacket_s *next;
} GirierMCUPacket_t;

GirierMCUPacket_t *gmcu_emptyPackets = 0;
GirierMCUPacket_t *gmcu_sendPackets = 0;

GirierMCUPacket_t *GirierMCU_AddToQueue(int len) {
	GirierMCUPacket_t *toUse;
	if (gmcu_emptyPackets) {
		toUse = gmcu_emptyPackets;
		gmcu_emptyPackets = toUse->next;

		if (len > toUse->allocated) {
			toUse->data = realloc(toUse->data, len);
			toUse->allocated = len;
		}
	}
	else {
		toUse = malloc(sizeof(GirierMCUPacket_t));
		int toAlloc = 128;
		if (len > toAlloc)
			toAlloc = len;
		toUse->allocated = toAlloc;
		toUse->data = malloc(toUse->allocated);
	}
	toUse->size = len;
	if (gmcu_sendPackets == 0) {
		gmcu_sendPackets = toUse;
	}
	else {
		GirierMCUPacket_t *p = gmcu_sendPackets;
		while (p->next) {
			p = p->next;
		}
		p->next = toUse;
	}
	toUse->next = 0;
	return toUse;
}

bool GirierMCU_SendFromQueue() {
	GirierMCUPacket_t *toUse;
	if (gmcu_sendPackets == 0)
		return false;
	toUse = gmcu_sendPackets;
	gmcu_sendPackets = toUse->next;

	UART_SendByte(0x00);
	UART_SendByte(0x00);
	UART_SendByte(0xff);
	UART_SendByte(0x55);
	
	for (int i = 0; i < toUse->size; i++) {
		UART_SendByte(toUse->data[i]);
	}

	UART_SendByte(0x05);
	UART_SendByte(0xdc);
	UART_SendByte(0x0a);
	UART_SendByte(0x00);
	UART_SendByte(0x00);
	
	toUse->next = gmcu_emptyPackets;
	gmcu_emptyPackets = toUse;
	return true;
}

girierMCUMapping_t* GirierMCU_FindDefForID(int dpId) {
	addLogAdv(LOG_DEBUG, LOG_FEATURE_TUYAMCU,
		"_FindDefForID(dpId=%d) called\n", dpId);
	girierMCUMapping_t* cur;

	cur = g_girierMappings;
	while (cur) {
		if (cur->dpId == dpId)
			return cur;
		cur = cur->next;
	}
	return 0;
}

girierMCUMapping_t* GirierMCU_FindDefForChannel(int channel) {
	addLogAdv(LOG_DEBUG, LOG_FEATURE_TUYAMCU,
		"_FindDefForChannel(channel=%d) called\n", channel);
	girierMCUMapping_t* cur;

	cur = g_girierMappings;
	while (cur) {
		if (cur->channel == channel)
			return cur;
		cur = cur->next;
	}
	return 0;
}

girierMCUMapping_t* GirierMCU_MapIDToChannel(int dpId, int dpType, int channel, int obkFlags, float mul, int inv, float delta, float delta2, float delta3) {
	addLogAdv(
		LOG_DEBUG, LOG_FEATURE_TUYAMCU,
		"_MapIDToChannel(dpId=%d, dpType=%d, channel=%d, obkFlags=%d, mul=%f, inv=%d, delta=%f, delta2=%f, delta3=%f) called", 
		dpId, dpType, channel, obkFlags, mul, inv, delta, delta2, delta3
	);
	girierMCUMapping_t* cur;

	cur = GirierMCU_FindDefForID(dpId);

	if (cur == 0) {
		cur = (girierMCUMapping_t*)malloc(sizeof(girierMCUMapping_t));
		cur->next = g_girierMappings;
		cur->rawData = 0;
		cur->rawDataLen = 0;
		cur->rawBufferSize = 0;
		g_girierMappings = cur;
	}
	cur->dpId = dpId;
	cur->dpType = dpType;
	cur->obkFlags = obkFlags;
	cur->mult = mul;
	cur->delta = delta;
	cur->delta2 = delta2;
	cur->delta3 = delta3;
	cur->inv = inv;
	cur->prevValue = 0;
	cur->channel = channel;
	return cur;
}

void GirierMCU_SendInit() {

	    byte girier_hello[] = {
			0x55, 0xaa, 0x00, 0xfe, 0x00, 0x05, 0x03, 0x03, 0xff, 0x03, 0xff, 0x09,
			0x00, 0x00, 0xff, 0x55, 0x01, 0x00, 0x79, 0x05, 0xdc, 0x0a, 0x00, 0x00,
			0x00, 0x00, 0xff, 0x55, 0x02, 0x79, 0x00, 0x05, 0xdc, 0x0a, 0x00, 0x00
		 };

		UART_InitUART(g_baudRate, 0, false);
		UART_SendByte(0x00);

		for (size_t i = 0; i < sizeof(girier_hello); i++) {
			UART_SendByte(girier_hello[i]);
		}     		
}


// append header, len, everything, checksum
void GirierMCU_SendCommandWithData(byte* data, int payload_len) {
	addLogAdv(
		LOG_DEBUG, LOG_FEATURE_TUYAMCU,
		"_SendCommandWithData(data=%p, payload_len=%d) called\n",
		 data, payload_len
	);
	int i;
	
	UART_InitUART(g_baudRate, 0, false);
	 if (CFG_HasFlag(OBK_FLAG_TUYAMCU_USE_QUEUE)) {
		GirierMCUPacket_t *p = GirierMCU_AddToQueue(payload_len);
		p->data[0] = payload_len & 0xFF;
		memcpy(p->data, data, payload_len);

	} else {
		UART_SendByte(0x00);
		UART_SendByte(0x00);
		UART_SendByte(0xff); 
		UART_SendByte(0x55);  
		     			
		for (i = 0; i < payload_len; i++) {
			byte b = data[i];
			UART_SendByte(b);
		}

		UART_SendByte(0x05);
		UART_SendByte(0xdc);
		UART_SendByte(0x0a);
		UART_SendByte(0x00);
		UART_SendByte(0x00);

	}
}

int GirierMCU_AppendStateInternal(byte *buffer, int bufferMax, int currentLen, uint8_t dpId, void* value, int dataLen) {
	addLogAdv(
		LOG_DEBUG, LOG_FEATURE_TUYAMCU,
		"_AppendStateInternal(buffer=%p, bufferMax=%d, currentLen=%d, id=%d,value=%p, dataLen=%d) called\n", 
		buffer, bufferMax, currentLen, dpId, value, dataLen
	);

	if (currentLen + 1 + dataLen >= bufferMax) {
		addLogAdv(LOG_ERROR, LOG_FEATURE_TUYAMCU, "Girier buff overflow");
		return 0;
	}
	buffer[currentLen + 0] = dpId;
	memcpy(buffer + (currentLen + 1), value, dataLen);

	return currentLen + 1 + dataLen;
}

void GirierMCU_SendStateInternal(uint8_t dpId, void* value, int dataLen) {
	addLogAdv(
		LOG_DEBUG, LOG_FEATURE_TUYAMCU,
		"_SendStateInternal(id=%d, value=%p, dataLen=%d) called\n", 
		dpId, value, dataLen
	);
	uint16_t payload_len = 0;
	
	payload_len = GirierMCU_AppendStateInternal(
		g_GirierMCUpayloadBuffer,
		g_GirierMCUpayloadBufferSize,
		payload_len, 
		dpId, 
		value, 
		dataLen
	);

	GirierMCU_SendCommandWithData(g_GirierMCUpayloadBuffer, payload_len);
}

void GirierMCU_SendValue(uint8_t dpId, int value) {
    uint8_t brightness = (uint8_t)(value & 0xFF);

    // two-byte payload (channel 1 + channel 2 brightness)
    uint8_t brightnessData[2] = { 0x00, 0x00 };

    if (dpId == 1) {
        brightnessData[0] = brightness;   // CH1 value in first byte
    } else if (dpId == 2) {
        brightnessData[1] = brightness;   // CH2 value in second byte
    } else {
        addLogAdv(LOG_WARN, LOG_FEATURE_TUYAMCU,
                  "Invalid dpId=%u (expected 1 or 2)", dpId);
        return;
    }

    addLogAdv(LOG_DEBUG, LOG_FEATURE_TUYAMCU,
              "dpId=%u, brightness=%u (data[0]=%u, data[1]=%u)",
              dpId, brightness, brightnessData[0], brightnessData[1]);

    // send both bytes (MCU might expect a fixed 2-byte frame)
    GirierMCU_SendStateInternal(dpId, brightnessData, sizeof(brightnessData));
}

commandResult_t GirierMCU_LinkGirierMCUOutputToChannel(const void* context, const char* cmd, const char* args, int cmdFlags) {
	addLogAdv(
		LOG_DEBUG, LOG_FEATURE_TUYAMCU,
		"_LinkGirierMCUOutputToChannel(context=%p, cmd=%s, args=%s, cmdFlags=%d) called\n",
		context, cmd, args, cmdFlags
	);
	const char* dpTypeString;
	byte dpId;
	byte dpType;
	int channelID;
	byte argsCount;
	byte obkFlags;
	float mult, delta, delta2, delta3;
	byte inv;

	// linkGirierMCUOutputToChannel [dpId] [varType] [channelID] [obkFlags] [mult] [inv] [delta]
	// linkGirierMCUOutputToChannel 1 val 1
	Tokenizer_TokenizeString(args, 0);

	argsCount = Tokenizer_GetArgsCount();
	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 2)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	dpId = Tokenizer_GetArgInteger(0);
	dpTypeString = Tokenizer_GetArg(1);

	if (argsCount < 3) {
		channelID = -999;
	}
	else {
		channelID = Tokenizer_GetArgInteger(2);
	}
	dpType = 0; // TODO - uninitialized
	obkFlags = Tokenizer_GetArgInteger(3);
	mult = Tokenizer_GetArgFloatDefault(4, 1.0f);
	inv = Tokenizer_GetArgInteger(5);
	delta = Tokenizer_GetArgFloatDefault(6, 0.0f);
	delta2 = Tokenizer_GetArgFloatDefault(7, 0.0f);
	delta3 = Tokenizer_GetArgFloatDefault(8, 0.0f);

	GirierMCU_MapIDToChannel(dpId, dpType, channelID, obkFlags, mult, inv, delta, delta2, delta3);

	return CMD_RES_OK;
}

bool GirierMCU_IsChannelUsedByGirierMCU(int channel) {
	addLogAdv(LOG_DEBUG, LOG_FEATURE_TUYAMCU, "_IsChannelUsedByGirierMCU(channel=%d) called", channel);
	girierMCUMapping_t* mapping;

	// find mapping
	mapping = GirierMCU_FindDefForChannel(channel);

	if (mapping == 0) {
		return false;
	}
	return true;
}

void GirierMCU_OnChannelChanged(int channel, int iVal) {
	addLogAdv(
		LOG_DEBUG, LOG_FEATURE_TUYAMCU,
		"_OnChannelChanged(channel=%d, iVal=%d) called", 
		channel, iVal
	);
	girierMCUMapping_t* mapping;

	// find mapping
	mapping = GirierMCU_FindDefForChannel(channel);

	if (mapping == 0) {
		return;
	}

	if (mapping->inv) {
		iVal = !iVal;
	}

	int mappediVal = ((iVal - g_dimmerRangeMin) * 255) / (g_dimmerRangeMax - g_dimmerRangeMin);

	addLogAdv(
		LOG_DEBUG, LOG_FEATURE_TUYAMCU,
		"g_dimmerRangeMax:%d, g_dimmerRangeMin:%d, mapped dp %i value %d to %d\n",
		g_dimmerRangeMax, g_dimmerRangeMin,  channel, iVal, mappediVal
	);

	// this might be a callback from CHANNEL_Set in GirierMCU_ApplyMapping. If we should set exactly the
	// same value, skip it
	if (mapping->prevValue == iVal) {
		return;
	}

	if (iVal != mappediVal) {
		addLogAdv(
			LOG_DEBUG, LOG_FEATURE_TUYAMCU,
			"OnChannelChanged: mapped value %d (OpenBK7231T_App range) to %d (GirierMCU range)\n",
			iVal,
			mappediVal
		);
	}
	mapping->prevValue = iVal;
	// send value to GirierMCU
	GirierMCU_SendValue(mapping->dpId, mappediVal);
		
}

#define CALIB_IF_NONZERO(x,d) if(x) { x += d; }

int girier_timer_send = 0;
void GirierMCU_RunFrame() {
	if (girier_timer_send > 0) {
		girier_timer_send -= g_deltaTimeMS;
	}
	else {
		if (GirierMCU_SendFromQueue()) {
			girier_timer_send = 100;
		}
	}
}

void GirierMCU_RunEverySecond() {

}

commandResult_t GirierMCU_SetBaudRate(const void* context, const char* cmd, const char* args, int cmdFlags) {
	addLogAdv(
		LOG_DEBUG, LOG_FEATURE_TUYAMCU,
		 "_SetBaudRate(context=%p, cmd=%s, args=%s, cmdFlags=%d) called",
		  context, cmd, args, cmdFlags
	);

	Tokenizer_TokenizeString(args, 0);
	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

	g_baudRate = Tokenizer_GetArgInteger(0);
	UART_InitUART(g_baudRate, 0, false);

	return CMD_RES_OK;
}

static SemaphoreHandle_t g_mutex = 0;

void GirierMCU_Shutdown() {
	addLogAdv(LOG_DEBUG, LOG_FEATURE_TUYAMCU, "_Shutdown() called");
	girierMCUMapping_t *tmp, *nxt;
	GirierMCUPacket_t *packet, *next_packet;

	// free the girierMCUMapping_t linked list
	tmp = g_girierMappings;
	while (tmp) {
		nxt = tmp->next;
		// free rawData if allocated
		if (tmp->rawData) {
			free(tmp->rawData);
			tmp->rawData = NULL;
			tmp->rawBufferSize = 0;
			tmp->rawDataLen = 0;
		}
		free(tmp);
		tmp = nxt;
	}
	g_girierMappings = NULL;

	// free the GirierMCUpayloadBuffer
	if (g_GirierMCUpayloadBuffer) {
		free(g_GirierMCUpayloadBuffer);
		g_GirierMCUpayloadBuffer = NULL;
		g_GirierMCUpayloadBufferSize = 0;
	}

	// free the gmcu_emptyPackets queue
	packet = gmcu_emptyPackets;
	while (packet) {
		next_packet = packet->next;
		if (packet->data) {
			free(packet->data);
			packet->data = NULL;
			packet->allocated = 0;
			packet->size = 0;
		}
		free(packet);
		packet = next_packet;
	}
	gmcu_emptyPackets = NULL;

	// free the gmcu_sendPackets queue
	packet = gmcu_sendPackets;
	while (packet) {
		next_packet = packet->next;
		if (packet->data) {
			free(packet->data);
			packet->data = NULL;
			packet->allocated = 0;
			packet->size = 0;
		}
		free(packet);
		packet = next_packet;
	}
	gmcu_sendPackets = NULL;

	// free the mutex
	if (g_mutex) {
		//vSemaphoreDelete(g_mutex);
		g_mutex = NULL;
	}
}

void GirierMCU_ForcePublishChannelValues() {
	addLogAdv(LOG_DEBUG, LOG_FEATURE_TUYAMCU, "_ForcePublishChannelValues() called");
#if ENABLE_MQTT
	girierMCUMapping_t* cur;

	cur = g_girierMappings;
	while (cur) {
		MQTT_ChannelPublish(cur->channel, 0);
		cur = cur->next;
	}
#endif
}

void GirierMCU_Init() {
	addLogAdv(LOG_DEBUG, LOG_FEATURE_TUYAMCU, "_Init() called");
	g_girierNextRequestDelay = 1;
	if (g_GirierMCUpayloadBuffer == 0) {
		g_GirierMCUpayloadBufferSize = GIRIERMCU_BUFFER_SIZE;
		g_GirierMCUpayloadBuffer = (byte*)malloc(GIRIERMCU_BUFFER_SIZE);
	}

	UART_InitUART(g_baudRate, 0, false);
	UART_InitReceiveRingBuffer(1024);
	GirierMCU_SendInit();
	CMD_RegisterCommand("linkGirierMCUOutputToChannel", GirierMCU_LinkGirierMCUOutputToChannel, NULL);
	CMD_RegisterCommand("GirierMCU_setBaudRate", GirierMCU_SetBaudRate, NULL);

}

