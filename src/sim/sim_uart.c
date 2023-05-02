#include "../new_common.h"



static byte *g_recvBuf = 0;
static int g_recvBufSize = 0;
static int g_recvBufIn = 0;
static int g_recvBufOut = 0;

void SIM_UART_InitReceiveRingBuffer(int size) {
	if (g_recvBuf != 0)
		free(g_recvBuf);
	g_recvBuf = (byte*)malloc(size);
	memset(g_recvBuf, 0, size);
	g_recvBufSize = size;
	g_recvBufIn = 0;
	g_recvBufOut = 0;
}
void SIM_ClearUART() {
	g_recvBufIn = 0;
	g_recvBufOut = 0;
}
int SIM_UART_GetDataSize()
{
	int remain_buf_size = 0;

	if (g_recvBufIn >= g_recvBufOut) {
		remain_buf_size = g_recvBufIn - g_recvBufOut;
	}
	else {
		remain_buf_size = g_recvBufIn + g_recvBufSize - g_recvBufOut;
	}

	return remain_buf_size;
}
byte SIM_UART_GetNextByte(int index) {
	int realIndex = g_recvBufOut + index;
	if (realIndex > g_recvBufSize)
		realIndex -= g_recvBufSize;

	return g_recvBuf[realIndex];
}
void SIM_UART_ConsumeBytes(int idx) {
	g_recvBufOut += idx;
	if (g_recvBufOut > g_recvBufSize)
		g_recvBufOut -= g_recvBufSize;
}

void SIM_AppendUARTByte(byte rc) {
	int curDataSize = SIM_UART_GetDataSize();

	if (curDataSize < (g_recvBufSize - 1))
	{
		g_recvBuf[g_recvBufIn++] = rc;
		if (g_recvBufIn >= g_recvBufSize) {
			g_recvBufIn = 0;
		}
	}
}


bool SIM_UART_ExpectAndConsumeHByte(byte b) {
	byte nextB;
	if (SIM_UART_GetDataSize() == 0)
		return false;
	nextB = SIM_UART_GetNextByte(0);
	if (nextB == b) {
		SIM_UART_ConsumeBytes(1);
		return true;
	}
	return false;
}
bool SIM_UART_ExpectAndConsumeHexStr(const char *hexString) {
	const char *p;
	byte b;
	int correctBytes;


	correctBytes = 0;
	p = hexString;
	while (*p) {
		if (*p == ' ' || *p == '\t') {
			p++;
			continue;
		}
		b = hexbyte(p);
		p += 2;
		if (SIM_UART_ExpectAndConsumeHByte(b) == false) {
			return false;
		}
		correctBytes++;
	}
	return true;
}