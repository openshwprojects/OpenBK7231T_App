#include "../new_common.h"

typedef struct bitMessage_s {
	byte *data;
	int position;
	int totalSize;
} bitMessage_t;

// bitmessage_read.c
void MSG_BeginReading(bitMessage_t *msg, const byte *data, int dataSize);
int MSG_ReadString(bitMessage_t *msg, char *out, int outBufferSize);
int MSG_SkipBytes(bitMessage_t *msg, int c);
int MSG_CheckAndSkip(bitMessage_t *msg, const char *s, int len);
unsigned short MSG_ReadU16(bitMessage_t *msg);
byte MSG_ReadByte(bitMessage_t *msg);
int MSG_Read3Bytes(bitMessage_t *msg);
int MSG_EOF(bitMessage_t *msg);
const char *MSG_GetStringPointerAtCurrentPosition(bitMessage_t *msg);

// bitmessage_write.c
void MSG_BeginWriting(bitMessage_t *msg, byte *data, int dataSize);
int MSG_WriteBytes(bitMessage_t *msg, const void *p, int numBytes);
int MSG_WriteString(bitMessage_t *msg, const char *s);
int MSG_WriteU16(bitMessage_t *msg, unsigned short s);
int MSG_WriteByte(bitMessage_t *msg, byte s);
int MSG_Write3Bytes(bitMessage_t *msg, int s);

