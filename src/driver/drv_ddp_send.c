

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

static int g_socket_ddpSend = -1;

typedef struct ddpQueueItem_s {
	int curSize;
	int totalSize;
	byte *data;
	struct sockaddr_in adr;
	struct ddpQueueItem_s *next;
} ddpQueueItem_t;

ddpQueueItem_t *g_queue = 0;

void DRV_DDPSend_SendInternal(struct sockaddr_in *adr, const byte *frame, int numBytes) {

	int nbytes = sendto(
		g_socket_ddpSend,
		(const char*)frame,
		numBytes,
		0,
		(struct sockaddr*) adr,
		sizeof(*adr)
	);
}
void DRV_DDPSend_Send(const char *ip, int port, const byte *frame, int numBytes, int delay) {
	struct sockaddr_in adr;
	memset(&adr, 0, sizeof(adr));
	adr.sin_family = AF_INET;
	adr.sin_addr.s_addr = inet_addr(ip);
	adr.sin_port = htons(port);

	if (delay <= 0) {
		DRV_DDPSend_SendInternal(ip, port, frame, numBytes);
		return;
	}

	ddpQueueItem_t *it = g_queue;
	while (it) {
		if (it->curSize == 0) {
			break;
		}
		it = it->next;
	}
	if (it == 0) {
		it = (ddpQueueItem_t*)malloc(sizeof(ddpQueueItem_t));
		memset(it,0,sizeof(ddpQueueItem_t));
	}
}
void DRV_DDPSend_RunFrame() {
	
}
void DRV_DDPSend_Shutdown()
{
	if (g_socket_ddpSend >= 0) {
		close(g_socket_ddpSend);
		g_socket_ddpSend = -1;
	}
}
void DRV_DDPSend_AppendInformationToHTTPIndexPage(http_request_t* request)
{

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
}



