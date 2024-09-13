

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

#if ENABLE_DRIVER_SM16703P
#include "drv_spiLED.h"
#endif

static const char* group = "239.255.250.250";
static int port = 4048;
static int g_ddp_socket_receive = -1;
static int g_retry_delay = 5;
static int stat_packetsReceived = 0;
static int stat_bytesReceived = 0;
static char *g_ddp_buffer = 0;
static int g_ddp_bufferSize = 512;

void DRV_DDP_CreateSocket_Receive() {

    struct sockaddr_in addr;
    struct ip_mreq mreq;
    int flag = 1;
	int broadcast = 0;
	int iResult = 1;

    // create what looks like an ordinary UDP socket
    //
    g_ddp_socket_receive = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (g_ddp_socket_receive < 0) {
		g_ddp_socket_receive = -1;
		addLogAdv(LOG_INFO, LOG_FEATURE_DDP,"failed to do socket\n");
        return ;
    }

	if(broadcast)
	{

		iResult = setsockopt(g_ddp_socket_receive, SOL_SOCKET, SO_BROADCAST, (char *)&flag, sizeof(flag));
		if (iResult != 0)
		{
			addLogAdv(LOG_INFO, LOG_FEATURE_DDP,"failed to do setsockopt SO_BROADCAST\n");
			close(g_ddp_socket_receive);
			g_ddp_socket_receive = -1;
			return ;
		}
	}
	else{
		// allow multiple sockets to use the same PORT number
		//
		if (
			setsockopt(
				g_ddp_socket_receive, SOL_SOCKET, SO_REUSEADDR, (char*) &flag, sizeof(flag)
			) < 0
		){
			addLogAdv(LOG_INFO, LOG_FEATURE_DDP,"failed to do setsockopt SO_REUSEADDR\n");
			close(g_ddp_socket_receive);
			g_ddp_socket_receive = -1;
		  return ;
		}
	}

        // set up destination address
    //
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY); // differs from sender
    addr.sin_port = htons(port);

    // bind to receive address
    //
    if (bind(g_ddp_socket_receive, (struct sockaddr*) &addr, sizeof(addr)) < 0) {
		addLogAdv(LOG_INFO, LOG_FEATURE_DDP,"failed to do bind\n");
		close(g_ddp_socket_receive);
		g_ddp_socket_receive = -1;
        return ;
    }

    //if(0 != setsockopt(g_ddp_socket_receive,SOL_SOCKET,SO_BROADCAST,(const char*)&flag,sizeof(int))) {
    //    return 1;
    //}

	if(broadcast)
	{

	}
	else
	{

	  // use setsockopt() to request that the kernel join a multicast group
		//
		mreq.imr_multiaddr.s_addr = inet_addr(group);
		//mreq.imr_interface.s_addr = htonl(INADDR_ANY);
	mreq.imr_interface.s_addr = htonl(INADDR_ANY);//inet_addr(MY_CAPTURE_IP);
    	///mreq.imr_interface.s_addr = inet_addr("192.168.0.122");
	iResult = setsockopt(
				g_ddp_socket_receive, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*) &mreq, sizeof(mreq)
			);
		if (
			iResult < 0
		){
			addLogAdv(LOG_INFO, LOG_FEATURE_DDP,"failed to do setsockopt IP_ADD_MEMBERSHIP %i\n",iResult);
			close(g_ddp_socket_receive);
			g_ddp_socket_receive = -1;
			return ;
		}
	}

	lwip_fcntl(g_ddp_socket_receive, F_SETFL,O_NONBLOCK);

	addLogAdv(LOG_INFO, LOG_FEATURE_DDP,"Waiting for packets\n");
}
void DDP_Parse(byte *data, int len) {
	if(len > 12) {
		byte r, g, b;
		r = data[10];
		g = data[11];
		b = data[12];

#if ENABLE_DRIVER_SM16703P
		if (spiLED.ready) {
			// Note that this is limited by DDP msgbuf size
			uint32_t pixel = (len - 10) / 3;
			// This immediately activates the pixels, maybe we should read the PUSH flag
			SM16703P_setMultiplePixel(pixel, &data[10], true);
		} else {
			LED_SetFinalRGB(r,g,b);
		}
#else
		LED_SetFinalRGB(r,g,b);
#endif
	}
}
void DRV_DDP_RunFrame() {
	struct sockaddr_in addr;
	int nbytes;

	if(g_ddp_socket_receive<0) {
		g_retry_delay--;
		if (g_retry_delay <= 0) {
			g_retry_delay = 15;
			DRV_DDP_CreateSocket_Receive();
		}
		addLogAdv(LOG_INFO, LOG_FEATURE_DDP,"no sock\n");
            return ;
        }
    // now just enter a read-print loop
    //
	while (1) {
		socklen_t addrlen = sizeof(addr);
		nbytes = recvfrom(
			g_ddp_socket_receive,
			g_ddp_buffer,
			g_ddp_bufferSize,
			0,
			(struct sockaddr *) &addr,
			&addrlen
		);
		if (nbytes <= 0) {
			//addLogAdv(LOG_INFO, LOG_FEATURE_DDP,"nothing\n");
			return;
		}
		//addLogAdv(LOG_INFO, LOG_FEATURE_DDP,"Received %i bytes from %s\n",nbytes,inet_ntoa(((struct sockaddr_in *)&addr)->sin_addr));

		stat_packetsReceived++;
		stat_bytesReceived += nbytes;

		DDP_Parse((byte*)g_ddp_buffer, nbytes);

		if (stat_packetsReceived % 10 == 0) {
			rtos_delay_milliseconds(5);
		}
	}
}
void DRV_DDP_Shutdown()
{
	if(g_ddp_socket_receive>=0) {
		close(g_ddp_socket_receive);
		g_ddp_socket_receive = -1;
	}
}
void DRV_DDP_AppendInformationToHTTPIndexPage(http_request_t* request)
{
	hprintf255(request, "<h2>DDP received: %i packets, %i bytes</h2>", stat_packetsReceived, stat_bytesReceived);
}
void DRV_DDP_Init()
{
	g_ddp_bufferSize = Tokenizer_GetArgIntegerDefault(1, 512);
	if (g_ddp_buffer) {
		free(g_ddp_buffer);
	}
	g_ddp_buffer = malloc(g_ddp_bufferSize);
	DRV_DDP_CreateSocket_Receive();
}



