#include "deviceGroups_local.h"
#include "../bitmessage/bitmessage_public.h"
#include "../logging/logging.h"

void DRV_DGR_Dump(byte *message, int len);

int DGR_BeginWriting(bitMessage_t *msg, const char *groupName, unsigned short sequence, unsigned short flags) {
	if(MSG_WriteBytes(msg,TASMOTA_DEVICEGROUPS_HEADER,strlen(TASMOTA_DEVICEGROUPS_HEADER))==0) {
		//
		// It should not happen, do not waste flash space for warning text...
		//
		//addLogAdv(LOG_INFO, LOG_FEATURE_DGR,"DGR_BeginWriting: no space for header");
		return 1;
	}

	if(MSG_WriteString(msg,groupName) <= 0) {
		//
		// It should not happen, do not waste flash space for warning text...
		//
		//addLogAdv(LOG_INFO, LOG_FEATURE_DGR,"DGR_BeginWriting: no space for group name");
		return 1;
	}

	if(MSG_WriteU16(msg,sequence) <= 0) {
		//
		// It should not happen, do not waste flash space for warning text...
		//
		//addLogAdv(LOG_INFO, LOG_FEATURE_DGR,"DGR_BeginWriting: no space for sequence");
		return 1;
	}

	if(MSG_WriteU16(msg,flags) <= 0) {
		//
		// It should not happen, do not waste flash space for warning text...
		//
		//addLogAdv(LOG_INFO, LOG_FEATURE_DGR,"DGR_BeginWriting: no space for flags");
		return 1;
	}
	return 0;
}
void DGR_AppendPowerState(bitMessage_t *msg, int numChannels, int channelBits) {
	MSG_WriteByte(msg,DGR_ITEM_POWER);
	MSG_Write3Bytes(msg,channelBits);
	MSG_WriteByte(msg,numChannels);
}
void DGR_AppendNoStatusShare(bitMessage_t *msg, int noStatusShare) {
	MSG_WriteByte(msg, DGR_ITEM_NO_STATUS_SHARE);
	MSG_Write3Bytes(msg, noStatusShare);
	MSG_WriteByte(msg, noStatusShare >> 24);
}
void DGR_AppendColorRGBCW(bitMessage_t *msg, byte r, byte g, byte b, byte c, byte w) {
	static byte colorSequence = 0;
	MSG_WriteByte(msg,DGR_ITEM_LIGHT_CHANNELS);
	MSG_WriteByte(msg,6);
	MSG_WriteByte(msg,r);
	MSG_WriteByte(msg,g);
	MSG_WriteByte(msg,b);
	MSG_WriteByte(msg,c);
	MSG_WriteByte(msg,w);
	MSG_WriteByte(msg,++colorSequence);
}
void DGR_AppendFixedColor(bitMessage_t *msg, int colorIndex) {
	MSG_WriteByte(msg, DGR_ITEM_LIGHT_FIXED_COLOR);
	// This is a single 8 bit value. No need to put a size byte there.
	MSG_WriteByte(msg, colorIndex);
}
void DGR_AppendDimmer(bitMessage_t *msg, byte dimmValue) {
	MSG_WriteByte(msg,DGR_ITEM_LIGHT_BRI);
	MSG_WriteByte(msg,dimmValue);
}
void DGR_Append8Bit(bitMessage_t *msg, byte item, byte value) {
	MSG_WriteByte(msg, item);
	MSG_WriteByte(msg, value);
}
void DGR_Finish(bitMessage_t *msg) {
	MSG_WriteByte(msg,DGR_ITEM_EOL);

	DRV_DGR_Dump(msg->data, msg->position);
}


int DGR_Quick_FormatPowerState(byte *buffer, int maxSize, const char *groupName, uint16_t sequence, int flags, int channels, int numChannels) {
	bitMessage_t msg;
	MSG_BeginWriting(&msg,buffer,maxSize);
	DGR_BeginWriting(&msg,groupName, sequence,flags);
	DGR_AppendPowerState(&msg,numChannels,channels);
	DGR_Finish(&msg);
	return msg.position;
}

int DGR_Quick_FormatBrightness(byte *buffer, int maxSize, const char *groupName, uint16_t sequence,int flags, byte brightness) {
	bitMessage_t msg;
	MSG_BeginWriting(&msg,buffer,maxSize);
	DGR_BeginWriting(&msg,groupName, sequence,flags);
	DGR_AppendDimmer(&msg,brightness);
	DGR_Finish(&msg);
	return msg.position;
}

int DGR_Quick_FormatRGBCW(byte *buffer, int maxSize, const char *groupName, uint16_t sequence,int flags, byte r, byte g, byte b, byte c, byte w) {
	bitMessage_t msg;
	MSG_BeginWriting(&msg,buffer,maxSize);
	DGR_BeginWriting(&msg,groupName, sequence,flags);
	DGR_AppendColorRGBCW(&msg,r,g,b,c,w);
	DGR_Finish(&msg);
	return msg.position;
}
int DGR_Quick_FormatFixedColor(byte *buffer, int maxSize, const char *groupName, uint16_t sequence, int flags, int color) {
	bitMessage_t msg;
	MSG_BeginWriting(&msg, buffer, maxSize);
	DGR_BeginWriting(&msg, groupName, sequence, flags);
	DGR_AppendFixedColor(&msg, color);
	DGR_Finish(&msg);
	return msg.position;
}

// Format an ACK message (just header with ACK flag, no payload items)
int DGR_Quick_FormatACK(byte *buffer, int maxSize, const char *groupName, uint16_t sequence) {
	return DGR_Quick_FormatStatusRequestWithFlags(buffer, maxSize, groupName, sequence, DGR_FLAG_ACK);
}

// Format an Announcement message (heartbeat with just announcement flag)
int DGR_Quick_FormatAnnouncement(byte *buffer, int maxSize, const char *groupName, uint16_t sequence) {
	return DGR_Quick_FormatStatusRequestWithFlags(buffer, maxSize, groupName, sequence, DGR_FLAG_ANNOUNCEMENT);
}

// Format a Status Request message (asking other devices to send their status)
int DGR_Quick_FormatStatusRequest(byte *buffer, int maxSize, const char *groupName, uint16_t sequence) {
	return DGR_Quick_FormatStatusRequestWithFlags(buffer, maxSize, groupName, sequence, DGR_FLAG_STATUS_REQUEST);
}

int DGR_Quick_FormatStatusRequestWithFlags(byte *buffer, int maxSize, const char *groupName, uint16_t sequence, int flags) {
	bitMessage_t msg;
	MSG_BeginWriting(&msg, buffer, maxSize);
	DGR_BeginWriting(&msg, groupName, sequence, flags);
	DRV_DGR_Dump(msg.data, msg.position);
	return msg.position;
}

int DGR_Quick_FormatFullStatus(byte *buffer, int maxSize, const char *groupName, uint16_t sequence,
	int relayStates, int numChannels, int shareFlags, unsigned int noStatusShare,
	byte brightness, byte scheme, const byte *rgbcw) {
	bitMessage_t msg;
	shareFlags &= ~noStatusShare;
	MSG_BeginWriting(&msg, buffer, maxSize);
	if (DGR_BeginWriting(&msg, groupName, sequence, DGR_FLAG_FULL_STATUS)) {
		return 0;
	}
	DGR_AppendNoStatusShare(&msg, noStatusShare);
	if (shareFlags & DGR_SHARE_POWER) {
		DGR_AppendPowerState(&msg, numChannels, relayStates);
	}
	if (shareFlags & DGR_SHARE_LIGHT_BRI) {
		DGR_AppendDimmer(&msg, brightness);
	}
	if (shareFlags & DGR_SHARE_LIGHT_SCHEME) {
		DGR_Append8Bit(&msg, DGR_ITEM_LIGHT_SCHEME, scheme);
	}
	if ((shareFlags & DGR_SHARE_LIGHT_COLOR) && rgbcw) {
		DGR_AppendColorRGBCW(&msg, rgbcw[0], rgbcw[1], rgbcw[2], rgbcw[3], rgbcw[4]);
	}
	DGR_Finish(&msg);
	return msg.position;
}

static uint32_t dgr_command_values_8[4][64];
static uint32_t dgr_command_values_16[4][64];
static uint32_t dgr_command_values_32[4][64];

static uint32_t *DGR_CommandValueSlot(int stateIndex, byte item) {
	if (stateIndex < 0 || stateIndex >= 4) stateIndex = 0;
	if (item <= DGR_ITEM_MAX_8BIT) return &dgr_command_values_8[stateIndex][item];
	if (item <= DGR_ITEM_MAX_16BIT) return &dgr_command_values_16[stateIndex][item - DGR_ITEM_MAX_8BIT - 1];
	if (item <= DGR_ITEM_MAX_32BIT) return &dgr_command_values_32[stateIndex][item - DGR_ITEM_MAX_16BIT - 1];
	return 0;
}

void DGR_CommandSetValue(int stateIndex, byte item, uint32_t value) {
	uint32_t *slot = DGR_CommandValueSlot(stateIndex, item);
	if (slot) *slot = value;
}

int DGR_Quick_FormatCommand(byte *buffer, int maxSize, const char *groupName, uint16_t sequence,
	int stateIndex, const char *items) {
	bitMessage_t msg;
	const char *p = items;
	int itemCount = 0;
	MSG_BeginWriting(&msg, buffer, maxSize);
	if (DGR_BeginWriting(&msg, groupName, sequence, 0)) return -1;
	while (p && *p) {
		char *end;
		unsigned long parsedItem;
		byte item;
		bool noShare = false;
		while (*p == ' ') p++;
		if (!*p) break;
		parsedItem = strtoul(p, &end, 0);
		if (end == p || parsedItem == 0 || parsedItem > 255 || *end != '=') return -1;
		item = (byte)parsedItem;
		p = end + 1;
		if (*p == 'N' || *p == 'n') { noShare = true; p++; }
		if (noShare && (!MSG_WriteByte(&msg, DGR_ITEM_FLAGS) || !MSG_WriteByte(&msg, DGR_ITEM_FLAG_NO_SHARE))) return -1;
		if (!MSG_WriteByte(&msg, item)) return -1;
		if (item <= DGR_ITEM_MAX_32BIT) {
			uint32_t value;
			uint32_t *slot = DGR_CommandValueSlot(stateIndex, item);
			char oper = 0;
			if (*p == '@') { oper = p[1]; p += 2; }
			value = (*p >= '0' && *p <= '9') ? strtoul(p, &end, 0) : (oper == '^' ? 0xFFFFFFFF : 1);
			if (*p >= '0' && *p <= '9') p = end;
			if (oper && slot) {
				if (oper == '+') value = *slot + value;
				else if (oper == '-') value = *slot - value;
				else if (oper == '^') value = *slot ^ value;
				else if (oper == '|') value = *slot | value;
				else if (oper == '&') value = *slot & value;
				else return -1;
			}
			if (item == DGR_ITEM_POWER && !(value >> 24)) value |= 1UL << 24;
			DGR_CommandSetValue(stateIndex, item, value);
			if (!MSG_WriteByte(&msg, value & 0xFF)) return -1;
			if (item > DGR_ITEM_MAX_8BIT && !MSG_WriteByte(&msg, (value >> 8) & 0xFF)) return -1;
			if (item > DGR_ITEM_MAX_16BIT
				&& (!MSG_WriteByte(&msg, (value >> 16) & 0xFF) || !MSG_WriteByte(&msg, (value >> 24) & 0xFF))) return -1;
		} else if (item <= DGR_ITEM_MAX_STRING) {
			byte value[128];
			int valueLen = 0;
			bool escaped = false;
			while (*p && (*p != ' ' || escaped)) {
				char chr = *p++;
				if (chr == '\\' && !escaped) { escaped = true; continue; }
				escaped = false;
				if (valueLen >= (int)sizeof(value) - 1) return -1;
				value[valueLen++] = chr;
			}
			value[valueLen++] = 0;
			if (!MSG_WriteByte(&msg, valueLen) || !MSG_WriteBytes(&msg, value, valueLen)) return -1;
		} else if (item == DGR_ITEM_LIGHT_CHANNELS) {
			byte channels[6] = { 0 };
			int i;
			bool hex = false;
			if (*p == '#') { hex = true; p++; }
			for (i = 0; i < 6 && *p && *p != ' '; i++) {
				if (hex) {
					char pair[3];
					if (!p[1]) return -1;
					pair[0] = p[0]; pair[1] = p[1]; pair[2] = 0;
					channels[i] = strtoul(pair, &end, 16);
					if (end != pair + 2) return -1;
					p += 2;
				} else {
					channels[i] = strtoul(p, &end, 10);
					if (end == p) return -1;
					p = end;
				}
				if (*p == ',') p++;
			}
			if (!MSG_WriteByte(&msg, 6) || !MSG_WriteBytes(&msg, channels, 6)) return -1;
		} else {
			return -1;
		}
		itemCount++;
		if (itemCount >= 32) return -1;
	}
	if (!itemCount || !MSG_WriteByte(&msg, DGR_ITEM_EOL)) return -1;
	return msg.position;
}
