/*



*/

#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../driver/drv_public.h"
#include "../logging/logging.h"
#include "lwip/sockets.h"
#include "lwip/ip_addr.h"
#include "lwip/inet.h"
#include "../httpserver/new_http.h"
#include "drv_local.h"
#include "../quicktick.h"


static int g_socket_ddpSend = -1;
static int stat_sendBytes = 0;
static int stat_sendPackets = 0;
static int stat_failedPackets = 0;

typedef struct ddpQueueItem_s {
	int curSize;
	int totalSize;
	byte *data;
	int delay;
	struct sockaddr_in adr;
	struct ddpQueueItem_s *next;
} ddpQueueItem_t;

ddpQueueItem_t *g_queue = 0;

int DRV_DDPSend_SendInternal(struct sockaddr_in *adr, const byte *frame, int numBytes) {
	int nbytes = sendto(
		g_socket_ddpSend,
		(const char*)frame,
		numBytes,
		0,
		(struct sockaddr*) adr,
		sizeof(*adr)
	);
	if (nbytes == numBytes) {
		stat_sendBytes += numBytes;
		stat_sendPackets++;
		return numBytes;
	}
	stat_failedPackets++;
	return 0;
}
void DRV_DDPSend_Send(const char *ip, int port, const byte *frame, int numBytes, int delay) {
	struct sockaddr_in adr;
	memset(&adr, 0, sizeof(adr));
	adr.sin_family = AF_INET;
	adr.sin_addr.s_addr = inet_addr(ip);
	adr.sin_port = htons(port);

	if (delay <= 0) {
		DRV_DDPSend_SendInternal(&adr, frame, numBytes);
		return;
	}

	ddpQueueItem_t *it = g_queue;
	while (it) {
		if (it->curSize == 0) {
			break; // found empty
		}
		it = it->next;
	}
	if (it == 0) {
		it = (ddpQueueItem_t*)malloc(sizeof(ddpQueueItem_t));
		if (it == 0) {
			// malloc failed
			return;
		}
		memset(it,0,sizeof(ddpQueueItem_t));
		it->adr = adr;
		it->totalSize = it->curSize = numBytes;
		it->data = malloc(numBytes);
		if (it->data == 0) {
			// malloc failed
			free(it);
			return;
		}
		it->next = g_queue;
		g_queue = it;
	}
	if (it->totalSize < numBytes) {
		byte *r = (byte*)realloc(it->data, numBytes);
		if (r == 0) {
			// realloc failed
			return;
		}
		it->data = r;
		it->totalSize = numBytes;
	}
	it->delay = delay;
	memcpy(it->data, frame, numBytes);
	it->curSize = numBytes;
}
void DRV_DDPSend_RunFrame() {
	ddpQueueItem_t *t;
	t = g_queue;
	while (t) {
		if (t->curSize > 0) {
			t->delay -= g_deltaTimeMS;
			if (t->delay <= 0) {
				int sendBytes = DRV_DDPSend_SendInternal(&t->adr, t->data, t->curSize);
				// TODO - do not clear if didn't send?
				t->curSize = 0; // mark as empty
			}
		}
		t = t->next;
	}
}
void DRV_DDPSend_Shutdown()
{
	if (g_socket_ddpSend >= 0) {
		close(g_socket_ddpSend);
		g_socket_ddpSend = -1;
	}
	ddpQueueItem_t *it = g_queue;
	while (it) {
		ddpQueueItem_t *n = it->next;
		free(it->data);
		free(it);
		it = n;
	}
	g_queue = 0;
}
void DRV_DDPSend_AppendInformationToHTTPIndexPage(http_request_t* request)
{
	hprintf255(request, "<h2>DDP sent: %i packets, %i bytes, errored packets: %i</h2>", 
		stat_sendPackets, stat_sendBytes, stat_failedPackets);
}

// ddp as in WLED
#define DDP_TYPE_RGB24  0x0B // 00 001 011 (RGB , 8 bits per channel, 3 channels)
#define DDP_TYPE_RGBW32 0x1B // 00 011 011 (RGBW, 8 bits per channel, 4 channels)
#define DDP_FLAGS1_VER1 0x40 // version=1

// https://github.com/wled/WLED/blob/main/wled00/udp.cpp
void DDP_SetHeader(byte *data, int pixelSize, int bytesCount) {
	// set ident
	data[0] = DDP_FLAGS1_VER1;

	// set pixel size
	if (pixelSize == 4) {
		data[2] = DDP_TYPE_RGBW32;
	}
	else if (pixelSize == 3) {
		data[2] = DDP_TYPE_RGB24;
	}

	// set bytes count
	data[8] = (byte)((bytesCount >> 8) & 0xFF); // MSB
	data[9] = (byte)(bytesCount & 0xFF);        // LSB
}
// startDriver DDPSend
// DDP_Send 192.168.0.226 3 0 FF000000
commandResult_t DDP_Send(const void* context, const char* cmd, const char* args, int cmdFlags) {
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
	if (Tokenizer_GetArgsCount() < 1) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	const char *ip = Tokenizer_GetArg(0);
	int port = 4048;// Tokenizer_GetArgInteger(1);
	int pixelSize = Tokenizer_GetArgInteger(1);
	int delay = Tokenizer_GetArgInteger(2);
	const char *pData = Tokenizer_GetArg(3);
	int numBytes = strlen(pData) / 2;
	int headerSize = 10;
	byte *data = malloc(headerSize+numBytes);
	int cur = headerSize;
	while (*pData) {
		data[cur] = CMD_ParseOrExpandHexByte(&pData);
		cur++;
	}
	DDP_SetHeader(data, pixelSize, (cur- headerSize));
	DRV_DDPSend_Send(ip, port, data, cur, delay);
	free(data);
	return CMD_RES_OK;
}
void DRV_DDPSend_Init()
{
	// create what looks like an ordinary UDP socket
	//
	g_socket_ddpSend = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (g_socket_ddpSend < 0) {
		g_socket_ddpSend = -1;
		addLogAdv(LOG_ERROR, LOG_FEATURE_HTTP, "DRV_DDPSend_Init: failed to do socket\n");
		return;
	}
	//cmddetail:{"name":"DDP_Send","args":"IP host pixelsize delay pData",
	//cmddetail:"descr":"",
	//cmddetail:"fn":"DDP_Send","file":"driver/drv_ddpSend.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("DDP_Send", DDP_Send, NULL);
}



