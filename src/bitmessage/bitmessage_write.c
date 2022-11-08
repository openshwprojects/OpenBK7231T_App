#include "bitmessage_public.h"

void MSG_BeginWriting(bitMessage_t *msg, byte *data, int dataSize) {
	msg->position = 0;
	msg->totalSize = dataSize;
	msg->data = data;
}

int MSG_WriteBytes(bitMessage_t *msg, const void *p, int numBytes) {
	if(msg->position + numBytes >= msg->totalSize)
		return 0;
	memcpy(msg->data + msg->position, p, numBytes);
	msg->position += numBytes;
	return numBytes;
}
int MSG_WriteString(bitMessage_t *msg, const char *s) {
	int len;
	int res;

	len = strlen(s);
	len++;
	res = MSG_WriteBytes(msg, s, len);
	return res;
	//return MSG_WriteBytes(msg,s,strlen(s)+1);
}
int MSG_WriteU16(bitMessage_t *msg, unsigned short s) {
	if(msg->position + 2 >= msg->totalSize)
		return 0;

	return MSG_WriteBytes(msg, (const void *)&s, 2);
}
int MSG_WriteByte(bitMessage_t *msg, byte s) {
	return MSG_WriteBytes(msg,&s, sizeof(s));
}

int MSG_Write3Bytes(bitMessage_t *msg, int s) {
	return MSG_WriteBytes(msg,&s, 3);
}
