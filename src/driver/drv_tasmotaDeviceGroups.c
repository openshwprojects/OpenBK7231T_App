

#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../logging/logging.h"
#include "../devicegroups/deviceGroups_public.h"
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

static int g_dgr_socket_receive = -1;
static int g_dgr_socket_send = -1;
static int g_dgr_send_seq = 0;
void DRV_DGR_CreateSocket_Send() {
    // create what looks like an ordinary UDP socket
    //
    g_dgr_socket_send = socket(AF_INET, SOCK_DGRAM, 0);
    if (g_dgr_socket_send < 0) {
        return;
    }



}
void DRV_DGR_Send_Generic(char *message, int len) {
    struct sockaddr_in addr;
	int nbytes;

	g_dgr_send_seq++;

    // set up destination address
    //
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(group);
    addr.sin_port = htons(port);

    nbytes = sendto(
            g_dgr_socket_send,
            message,
            len,
            0,
            (struct sockaddr*) &addr,
            sizeof(addr)
        );
}
void DRV_DGR_Send_Power(const char *groupName, int channelValues, int numChannels){
	int len;
	char message[64];

	len = DGR_Quick_FormatPowerState(message,sizeof(message),groupName,g_dgr_send_seq, 0,channelValues, numChannels);

	DRV_DGR_Send_Generic(message,len);
}
void DRV_DGR_Send_Brightness(const char *groupName, byte brightness){
	int len;
	char message[64];

	len = DGR_Quick_FormatBrightness(message,sizeof(message),groupName,g_dgr_send_seq, 0, brightness);

	DRV_DGR_Send_Generic(message,len);
}
void DRV_DGR_CreateSocket_Receive() {

    struct sockaddr_in addr;
    struct ip_mreq mreq;
    int flag = 1;
	int broadcast = 0;
	int iResult = 1;

    // create what looks like an ordinary UDP socket
    //
    g_dgr_socket_receive = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (g_dgr_socket_receive < 0) {
		g_dgr_socket_receive = -1;
		addLogAdv(LOG_INFO, LOG_FEATURE_DGR,"failed to do socket\n");
        return ;
    }

	if(broadcast)
	{

		iResult = setsockopt(g_dgr_socket_receive, SOL_SOCKET, SO_BROADCAST, (char *)&flag, sizeof(flag));
		if (iResult != 0)
		{
			addLogAdv(LOG_INFO, LOG_FEATURE_DGR,"failed to do setsockopt SO_BROADCAST\n");
			close(g_dgr_socket_receive);
			g_dgr_socket_receive = -1;
			return ;
		}
	}
	else{
		// allow multiple sockets to use the same PORT number
		//
		if (
			setsockopt(
				g_dgr_socket_receive, SOL_SOCKET, SO_REUSEADDR, (char*) &flag, sizeof(flag)
			) < 0
		){
			addLogAdv(LOG_INFO, LOG_FEATURE_DGR,"failed to do setsockopt SO_REUSEADDR\n");
			close(g_dgr_socket_receive);
			g_dgr_socket_receive = -1;
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
    if (bind(g_dgr_socket_receive, (struct sockaddr*) &addr, sizeof(addr)) < 0) {
		addLogAdv(LOG_INFO, LOG_FEATURE_DGR,"failed to do bind\n");
		close(g_dgr_socket_receive);
		g_dgr_socket_receive = -1;
        return ;
    }

    //if(0 != setsockopt(g_dgr_socket_receive,SOL_SOCKET,SO_BROADCAST,(const char*)&flag,sizeof(int))) {
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
				g_dgr_socket_receive, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*) &mreq, sizeof(mreq)
			);
		if (
			iResult < 0
		){
			addLogAdv(LOG_INFO, LOG_FEATURE_DGR,"failed to do setsockopt IP_ADD_MEMBERSHIP %i\n",iResult);
			close(g_dgr_socket_receive);
			g_dgr_socket_receive = -1;
			return ;
		}
	}

	lwip_fcntl(g_dgr_socket_receive, F_SETFL,O_NONBLOCK);

	addLogAdv(LOG_INFO, LOG_FEATURE_DGR,"Waiting for packets\n");
}

void DRV_DGR_processRGBCW(byte r, byte g, byte b, byte c, byte w) {

}
void DRV_DGR_processPower(int relayStates, byte relaysCount) {
	int startIndex;
	int i;
	int ch;

	LED_SetEnableAll(BIT_CHECK(relayStates,0));
	//if(CHANNEL_HasChannelSomeOutputPin(0)) {
	//	startIndex = 0;
	//} else {
	//	startIndex = 1;
	//}
	//for(i = 0; i < relaysCount; i++) {
	//	int bOn;
	//	bOn = BIT_CHECK(relayStates,i);
	//	ch = startIndex+i;
	//	if(bOn) {
	//		if(CHANNEL_HasChannelPinWithRole(ch,IOR_PWM)) {

	//		} else {
	//			CHANNEL_Set(ch,1,0);
	//		}
	//	} else {
	//		CHANNEL_Set(ch,0,0);
	//	}
	//}
}
byte Val255ToVal100(byte v){ 
	float fr;
	// convert to our 0-100 range
	fr = v / 255.0f;
	v = fr * 100;
	return v;
}
void DRV_DGR_processBrightnessPowerOn(byte brightness) {
	addLogAdv(LOG_INFO, LOG_FEATURE_DGR,"DRV_DGR_processBrightnessPowerOn: %i\n",(int)brightness);

	LED_SetDimmer(Val255ToVal100(brightness));

	//numPWMs = PIN_CountPinsWithRole(IOR_PWM);
	//idx_pin = PIN_FindPinIndexForRole(IOR_PWM,0);
	//idx_channel = PIN_GetPinChannelForPinIndex(idx_pin);

	//CHANNEL_Set(idx_channel,brightness,0);
	
}
void DRV_DGR_processLightBrightness(byte brightness) {
	addLogAdv(LOG_INFO, LOG_FEATURE_DGR,"DRV_DGR_processLightBrightness: %i\n",(int)brightness);

	LED_SetDimmer(Val255ToVal100(brightness));
	
}
void DRV_DGR_RunFrame() {
    struct sockaddr_in addr;
	dgrDevice_t def;
    char msgbuf[64];

	if(g_dgr_socket_receive<0) {
		addLogAdv(LOG_INFO, LOG_FEATURE_DGR,"no sock\n");
            return ;
        }
    // now just enter a read-print loop
    //
        int addrlen = sizeof(addr);
        int nbytes = recvfrom(
            g_dgr_socket_receive,
            msgbuf,
            sizeof(msgbuf),
            0,
            (struct sockaddr *) &addr,
            &addrlen
        );
        if (nbytes <= 0) {
			//addLogAdv(LOG_INFO, LOG_FEATURE_DGR,"nothing\n");
            return ;
        }
		addLogAdv(LOG_INFO, LOG_FEATURE_DGR,"Received %i bytes\n",nbytes);
        msgbuf[nbytes] = '\0';

		strcpy(def.gr.groupName,CFG_DeviceGroups_GetName());
		def.gr.devGroupShare_In = CFG_DeviceGroups_GetRecvFlags();
		def.gr.devGroupShare_Out = CFG_DeviceGroups_GetSendFlags();
		def.cbs.processBrightnessPowerOn = DRV_DGR_processBrightnessPowerOn;
		def.cbs.processLightBrightness = DRV_DGR_processLightBrightness;
		def.cbs.processPower = DRV_DGR_processPower;
		def.cbs.processRGBCW = DRV_DGR_processRGBCW;

		DGR_Parse(msgbuf, nbytes, &def);
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
void DRV_DGR_Shutdown()
{
	if(g_dgr_socket_receive>=0) {
		close(g_dgr_socket_receive);
		g_dgr_socket_receive = -1;
	}
}

// DGR_SendPower testSocket 1 1
int CMD_DGR_SendPower(const void *context, const char *cmd, const char *args, int flags) {
	const char *groupName;
	int channelValues;
	int channelsCount;

	Tokenizer_TokenizeString(args);

	if(Tokenizer_GetArgsCount() < 3) {
		addLogAdv(LOG_INFO, LOG_FEATURE_DGR,"Command requires 3 arguments - groupname, channelvalues, valuescount\n");
		return 0;
	}
	groupName = Tokenizer_GetArg(0);
	channelValues = Tokenizer_GetArgInteger(1);
	channelsCount = Tokenizer_GetArgInteger(2);

	DRV_DGR_Send_Power(groupName,channelValues,channelsCount);

	return 1;
}
void DRV_DGR_OnChannelChanged(int ch, int value) {
	int channelValues;
	int channelsCount;
	int i;
	const char *groupName;

	if((CFG_DeviceGroups_GetSendFlags() & DGR_SHARE_POWER)==0) {

		return;
	}
	channelValues = 0;
	channelsCount = 0;
	groupName = CFG_DeviceGroups_GetName();

	for(i = 0; i < CHANNEL_MAX; i++) {
		if(CHANNEL_HasChannelPinWithRole(i,IOR_Relay) || CHANNEL_HasChannelPinWithRole(i,IOR_Relay_n)
			|| CHANNEL_HasChannelPinWithRole(i,IOR_LED) || CHANNEL_HasChannelPinWithRole(i,IOR_LED_n)) {
			channelsCount = i + 1;
			if(CHANNEL_Get(i)) {
				BIT_SET(channelValues ,i);
			}
		} 
	}

	DRV_DGR_Send_Power(groupName,channelValues,channelsCount);

	
}
// DGR_SendBrightness roomLEDstrips 128
int CMD_DGR_SendBrightness(const void *context, const char *cmd, const char *args, int flags) {
	const char *groupName;
	int brightness;

	Tokenizer_TokenizeString(args);

	if(Tokenizer_GetArgsCount() < 2) {
		addLogAdv(LOG_INFO, LOG_FEATURE_DGR,"Command requires 2 arguments - groupname, brightess\n");
		return 0;
	}
	groupName = Tokenizer_GetArg(0);
	brightness = Tokenizer_GetArgInteger(1);

	DRV_DGR_Send_Brightness(groupName,brightness);

	return 1;
}
void DRV_DGR_Init()
{
#if 0
	DRV_DGR_StartThread();
#else
	DRV_DGR_CreateSocket_Receive();
	DRV_DGR_CreateSocket_Send();

#endif
    CMD_RegisterCommand("DGR_SendPower", "", CMD_DGR_SendPower, "qqq", NULL);
    CMD_RegisterCommand("DGR_SendBrightness", "", CMD_DGR_SendBrightness, "qqq", NULL);
}



