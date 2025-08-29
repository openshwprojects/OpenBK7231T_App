

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

static int g_socket_ddpSend;
void DRV_DDPSend_SendInternal(const char *ip, int port, const byte *frame, int numBytes) {
	struct sockaddr_in adr;

	memset(&adr, 0, sizeof(adr));
	adr.sin_family = AF_INET;
	adr.sin_addr.s_addr = inet_addr(ip);
	adr.sin_port = htons(port);

	int nbytes = sendto(
		g_socket_ddpSend,
		(const char*)frame,
		numBytes,
		0,
		(struct sockaddr*) &adr,
		sizeof(adr)
	);
}
void DRV_DDPSend_Send(const char *ip, int port, const byte *frame, int numBytes, int delay) {
	if (delay <= 0) {
		DRV_DDPSend_SendInternal(ip, port, frame, numBytes);
		return;
	}
}
void DRV_DDPSend_RunFrame() {
	
}
void DRV_DDPSend_Shutdown()
{

}
void DRV_DDPSend_AppendInformationToHTTPIndexPage(http_request_t* request)
{

}
void DRV_DDPSend_Init()
{

}



