

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

int DRV_DGR_CreateSocket_Send() {

    struct sockaddr_in addr;
    int flag = 1;
	int fd;

    // create what looks like an ordinary UDP socket
    //
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        return 1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(group);
    addr.sin_port = htons(port);


	return 0;
}
int DRV_DGR_CreateSocket_Receive(beken_thread_arg_t arg) {

    struct sockaddr_in addr;
    struct ip_mreq mreq;
    int flag = 1;
	int fd;

    // create what looks like an ordinary UDP socket
    //
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        return 1;
    }

    // allow multiple sockets to use the same PORT number
    //
    if (
        setsockopt(
            fd, SOL_SOCKET, SO_REUSEADDR, (char*) &flag, sizeof(flag)
        ) < 0
    ){
       return 1;
    }

        // set up destination address
    //
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY); // differs from sender
    addr.sin_port = htons(port);

    // bind to receive address
    //
    if (bind(fd, (struct sockaddr*) &addr, sizeof(addr)) < 0) {
        return 1;
    }

    //if(0 != setsockopt(fd,SOL_SOCKET,SO_BROADCAST,(const char*)&flag,sizeof(int))) {
    //    return 1;
    //}

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(group);
    addr.sin_port = htons(port);

  // use setsockopt() to request that the kernel join a multicast group
    //
    mreq.imr_multiaddr.s_addr = inet_addr(group);
    //mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);//inet_addr(MY_CAPTURE_IP);
    if (
        setsockopt(
            fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*) &mreq, sizeof(mreq)
        ) < 0
    ){
        perror("setsockopt");
        return 1;
    }

	addLogAdv(LOG_INFO, LOG_FEATURE_BL0942,"Waiting for packets\n");
    // now just enter a read-print loop
    //
    while (1) {
        char msgbuf[64];
        int addrlen = sizeof(addr);
        int nbytes = recvfrom(
            fd,
            msgbuf,
            sizeof(msgbuf),
            0,
            (struct sockaddr *) &addr,
            &addrlen
        );
        if (nbytes < 0) {
            return 1;
        }
		addLogAdv(LOG_INFO, LOG_FEATURE_BL0942,"Received %i bytes\n",nbytes);
        msgbuf[nbytes] = '\0';
		//DGR_Parse(msgbuf, nbytes);
       // puts(msgbuf);
     }
	return 0;
}
xTaskHandle g_dgr_thread = NULL;
void DRV_DGR_Init()
{
    OSStatus err = kNoErr;

    err = rtos_create_thread( &g_dgr_thread, BEKEN_APPLICATION_PRIORITY, 
									"DGR Server", 
									(beken_thread_function_t)DRV_DGR_CreateSocket_Receive,
									0x800,
									(beken_thread_arg_t)0 );
    if(err != kNoErr)
    {
      addLogAdv(LOG_INFO, LOG_FEATURE_BL0942, "create \"TCP_server\" thread failed!\r\n");
    }
}



