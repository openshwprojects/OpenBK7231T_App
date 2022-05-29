

#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../logging/logging.h"
#include "lwip/sockets.h"
#include "lwip/ip_addr.h"
#include "lwip/inet.h"

const char* group = "239.255.250.250"; 
int port = 4447;
//
//int DRV_DGR_CreateSocket_Send() {
//
//    struct sockaddr_in addr;
//    int flag = 1;
//	int fd;
//
//    // create what looks like an ordinary UDP socket
//    //
//    fd = socket(AF_INET, SOCK_DGRAM, 0);
//    if (fd < 0) {
//        return 1;
//    }
//
//    memset(&addr, 0, sizeof(addr));
//    addr.sin_family = AF_INET;
//    addr.sin_addr.s_addr = inet_addr(group);
//    addr.sin_port = htons(port);
//
//
//	return 0;
//}

static int g_dgr_socket = 0;
void DRV_DGR_CreateSocket_Receive() {

    struct sockaddr_in addr;
    struct ip_mreq mreq;
    int flag = 1;
	int broadcast = 0;
	int iResult = 1;

    // create what looks like an ordinary UDP socket
    //
    g_dgr_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (g_dgr_socket < 0) {
		addLogAdv(LOG_INFO, LOG_FEATURE_DGR,"failed to do socket\n");
        return ;
    }

	if(broadcast) 
	{

		iResult = setsockopt(g_dgr_socket, SOL_SOCKET, SO_BROADCAST, (char *)&flag, sizeof(flag));
		if (iResult != 0) 
		{
			addLogAdv(LOG_INFO, LOG_FEATURE_DGR,"failed to do setsockopt SO_BROADCAST\n");
			return ;
		}
	}
	else{
		// allow multiple sockets to use the same PORT number
		//
		if (
			setsockopt(
				g_dgr_socket, SOL_SOCKET, SO_REUSEADDR, (char*) &flag, sizeof(flag)
			) < 0
		){
			addLogAdv(LOG_INFO, LOG_FEATURE_DGR,"failed to do setsockopt SO_REUSEADDR\n");
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
    if (bind(g_dgr_socket, (struct sockaddr*) &addr, sizeof(addr)) < 0) {
		addLogAdv(LOG_INFO, LOG_FEATURE_DGR,"failed to do bind\n");
        return ;
    }

    //if(0 != setsockopt(g_dgr_socket,SOL_SOCKET,SO_BROADCAST,(const char*)&flag,sizeof(int))) {
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
				g_dgr_socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*) &mreq, sizeof(mreq)
			);
		if (
			iResult < 0
		){
			addLogAdv(LOG_INFO, LOG_FEATURE_DGR,"failed to do setsockopt IP_ADD_MEMBERSHIP %i\n",iResult);
			return ;
		}
	}

	addLogAdv(LOG_INFO, LOG_FEATURE_DGR,"Waiting for packets\n");
}
void DRV_DGR_RunFrame() {
    struct sockaddr_in addr;
	
	if(g_dgr_socket==0) {
		addLogAdv(LOG_INFO, LOG_FEATURE_DGR,"no sock\n");
            return ;
        }
    // now just enter a read-print loop
    //
        char msgbuf[64];
        int addrlen = sizeof(addr);
        int nbytes = recvfrom(
            g_dgr_socket,
            msgbuf,
            sizeof(msgbuf),
            0,
            (struct sockaddr *) &addr,
            &addrlen
        );
        if (nbytes <= 0) {
			addLogAdv(LOG_INFO, LOG_FEATURE_DGR,"nothing\n");
            return ;
        }
		addLogAdv(LOG_INFO, LOG_FEATURE_DGR,"Received %i bytes\n",nbytes);
        msgbuf[nbytes] = '\0';
		//DGR_Parse(msgbuf, nbytes);
       // puts(msgbuf);
}
//static void DRV_DGR_Thread(beken_thread_arg_t arg) {
//
//    (void)( arg );
//
//	DRV_DGR_CreateSocket_Receive();
//	while(1) {
//		DRV_DGR_RunFrame();
//	}
//
//	return ;
//}
//xTaskHandle g_dgr_thread = NULL;

//void DRV_DGR_StartThread()
//{
//     OSStatus err = kNoErr;
//
//
//    err = rtos_create_thread( &g_dgr_thread, BEKEN_APPLICATION_PRIORITY, 
//									"DGR_server", 
//									(beken_thread_function_t)DRV_DGR_Thread,
//									0x100,
//									(beken_thread_arg_t)0 );
//   
//}

void DRV_DGR_Init()
{
#if 0
	DRV_DGR_StartThread();
#else
	DRV_DGR_CreateSocket_Receive();
   
	lwip_fcntl(g_dgr_socket, F_SETFL,O_NONBLOCK);
#endif
}



