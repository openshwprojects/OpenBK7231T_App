

#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../logging/logging.h"
#include "lwip/sockets.h"
#include "lwip/ip_addr.h"
#include "lwip/inet.h"
#include "../httpserver/new_http.h"

static const char* group = "239.255.250.250";
static int port = 4048;
static int g_ddp_socket_receive = -1;
static int g_retry_delay = 5;
static int stat_packetsReceived = 0;
static int stat_bytesReceived = 0;

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
		byte r, g, b, w, type;
		type = data[2];
		r = data[10];
		g = data[11];
		b = data[12];
		w = data[13];

	        //addLogAdv(LOG_INFO, LOG_FEATURE_DDP, "DDP data: type=%u r=%u g=%u b=%u w=%u", type, r, g, b, w);
		if(type == 0x1B) {
		    // TODO: support W
		    LED_SetFinalRGB(r,g,b);
		} else {
		    LED_SetFinalRGB(r,g,b);
		}
	}
}
void DRV_DDP_RunFrame() {
    char msgbuf[64];
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
			msgbuf,
			sizeof(msgbuf),
			0,
			(struct sockaddr *) &addr,
			&addrlen
		);
		if (nbytes <= 0) {
			//addLogAdv(LOG_INFO, LOG_FEATURE_DDP,"nothing\n");
			return;
		}
		//addLogAdv(LOG_INFO, LOG_FEATURE_DDP,"Received %i bytes from %s\n",nbytes,inet_ntoa(((struct sockaddr_in *)&addr)->sin_addr));
		msgbuf[nbytes] = '\0';

		stat_packetsReceived++;
		stat_bytesReceived += nbytes;

		DDP_Parse((byte*)msgbuf, nbytes);

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
	DRV_DDP_CreateSocket_Receive();
}



