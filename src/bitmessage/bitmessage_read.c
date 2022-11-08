#include "bitmessage_public.h"



void MSG_BeginReading(bitMessage_t *msg, const byte *data, int dataSize) {
	msg->position = 0;
	msg->totalSize = dataSize;
	msg->data = (byte*) data;
}
const char *MSG_GetStringPointerAtCurrentPosition(bitMessage_t *msg) {
	const char *r = (const char*)(msg->data+msg->position);
	return r;
}
int MSG_SkipBytes(bitMessage_t *msg, int c) {
	if(msg->position + c > msg->totalSize)
		return 0;
	msg->position += c;
	return c;
}
int MSG_ReadString(bitMessage_t *msg, char *out, int outBufferSize) {
	const char *start;
	int len;

	start = (const char*)(msg->data + msg->position);
	len = 0;
	while(1) {
		if(msg->position >= msg->totalSize) {
			return -1;
		}
		if(msg->data[msg->position] == 0) {
			msg->position++;
			break;
		}
		msg->position++;
		len++;
	} 
	
	strcpy_safe(out, start,outBufferSize);
	out[len] = 0;
	return strlen(out);
}
unsigned short MSG_ReadU16(bitMessage_t *msg) {
	byte *p;
	unsigned short ret = 0;

	if(msg->position + sizeof(unsigned short) > msg->totalSize)
		return 0;

	// ##### NOTE IS THIS CORRECT BYTE ORDERINGFOR TAS???
	p = (byte *)&ret;
	*p = MSG_ReadByte(msg);
	p++;
	*p = MSG_ReadByte(msg);
	return ret;
}
int MSG_Read3Bytes(bitMessage_t *msg) {
	int ret = 0;

	if(msg->position + 3 > msg->totalSize)
		return 0;

	memcpy(&ret,msg->data + msg->position, 3);

	msg->position += 3;

	return ret;

}
byte MSG_ReadByte(bitMessage_t *msg) {
	byte ret;

	if(msg->position + sizeof(byte) > msg->totalSize)
		return 0;

	ret = *(byte*)(msg->data + msg->position);

	msg->position += sizeof(byte);

	return ret;
}
int MSG_CheckAndSkip(bitMessage_t *msg, const char *s, int len) {
	int i;
	if(msg->position + len > msg->totalSize)
		return 0;
	for(i = 0; i < len; i++) {
		if(msg->data[msg->position+i] != s[i])
			return 0;
	}	
	msg->position += len;
	return len;
}
int MSG_EOF(bitMessage_t *msg) {
	if(msg->position >= msg->totalSize)
		return 1;
	return 0;
}


