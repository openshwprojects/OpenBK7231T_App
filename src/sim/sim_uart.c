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
    return (g_recvBufIn >= g_recvBufOut
                ? g_recvBufIn - g_recvBufOut
                : g_recvBufIn + (g_recvBufSize - g_recvBufOut) + 1);
}

byte SIM_UART_GetByte(int idx) {
    return g_recvBuf[(g_recvBufOut + idx) % g_recvBufSize];
}

void SIM_UART_ConsumeBytes(int idx) {
    g_recvBufOut += idx;
    g_recvBufOut %= g_recvBufSize;
}

void SIM_AppendUARTByte(byte rc) {
    if (SIM_UART_GetDataSize() < (g_recvBufSize - 1)) {
        g_recvBuf[g_recvBufIn++] = rc;
        g_recvBufIn %= g_recvBufSize;
    }
}

bool SIM_UART_ExpectAndConsumeHByte(byte b) {
	byte nextB;
	int dataSize = SIM_UART_GetDataSize();
	if (dataSize == 0)
		return false;
	nextB = SIM_UART_GetByte(0);
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
