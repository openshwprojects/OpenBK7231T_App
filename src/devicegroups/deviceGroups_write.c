#include "deviceGroups_local.h"
#include "../bitmessage/bitmessage_public.h"
#include "../logging/logging.h"

void DRV_DGR_Dump(byte *message, int len);

int DGR_BeginWriting(bitMessage_t *msg, const char *groupName, unsigned short sequence, unsigned short flags) {
	if(MSG_WriteBytes(msg,TASMOTA_DEVICEGROUPS_HEADER,strlen(TASMOTA_DEVICEGROUPS_HEADER))==0) {
		//
		// It should not happen, do not waste flash space for warning text...
		//
		//addLogAdv(LOG_INFO, LOG_FEATURE_DGR,"DGR_BeginWriting: no space for header\n");
		return 1;
	}

	if(MSG_WriteString(msg,groupName) <= 0) {
		//
		// It should not happen, do not waste flash space for warning text...
		//
		//addLogAdv(LOG_INFO, LOG_FEATURE_DGR,"DGR_BeginWriting: no space for group name\n");
		return 1;
	}

	if(MSG_WriteU16(msg,sequence) <= 0) {
		//
		// It should not happen, do not waste flash space for warning text...
		//
		//addLogAdv(LOG_INFO, LOG_FEATURE_DGR,"DGR_BeginWriting: no space for sequence\n");
		return 1;
	}

	if(MSG_WriteU16(msg,flags) <= 0) {
		//
		// It should not happen, do not waste flash space for warning text...
		//
		//addLogAdv(LOG_INFO, LOG_FEATURE_DGR,"DGR_BeginWriting: no space for flags\n");
		return 1;
	}
	return 0;
}
void DGR_AppendPowerState(bitMessage_t *msg, int numChannels, int channelBits) {
	MSG_WriteByte(msg,DGR_ITEM_POWER);
	MSG_Write3Bytes(msg,channelBits);
	MSG_WriteByte(msg,numChannels);
}
void DGR_AppendColorRGBCW(bitMessage_t *msg, byte r, byte g, byte b, byte c, byte w) {
	MSG_WriteByte(msg,DGR_ITEM_LIGHT_CHANNELS);
	MSG_WriteByte(msg,5);
	MSG_WriteByte(msg,r);
	MSG_WriteByte(msg,g);
	MSG_WriteByte(msg,b);
	MSG_WriteByte(msg,c);
	MSG_WriteByte(msg,w);
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





