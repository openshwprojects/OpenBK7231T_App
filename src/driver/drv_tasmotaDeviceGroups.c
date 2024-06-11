

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
#include "../httpserver/new_http.h"

static const char* dgr_group = "239.255.250.250";
static int dgr_port = 4447;
static int dgr_retry_time_left = 5;
static int g_inCmdProcessing = 0;
static int g_dgr_socket_receive = -1;
static int g_dgr_socket_send = -1;
// statistics
static int g_dgr_stat_sent = 0;
static int g_dgr_stat_received = 0;
struct sockaddr_in g_mySockAddr;

static uint16_t g_dgr_send_seq = 0;

const char *HAL_GetMyIPString();

void DRV_DGR_Dump(byte *message, int len);

//
// A DGR outgoing packets queue mechanism.
// Used to send all DGR on quick tick 
// (instead of doing it in-place, from MQTT callback etc)
//
// TODO: MUTEX !!!!!
//
// Maximum number of bytes in pendings DGR packet
#define MAX_DGR_PACKET 128
// limits the total number of dgrPacket_t we can alloc
#define MAX_DGR_QUEUE_SIZE 8

typedef struct dgrPacket_s {
	struct dgrPacket_s *next;
	byte buffer[MAX_DGR_PACKET];
	byte length;
} dgrPacket_t;

// the list is not allocated before first use
dgrPacket_t *dgr_pending = 0;
int dgr_total_alloced_queue_size = 0;

static SemaphoreHandle_t g_mutex = 0;

// Adds a packet to DGR send queue. Can be called from anywhere, MQTT callback, etc.
// We don't send UDP DGR packets directly from MQTT callback, because it would crash device in some cases....
void DGR_AddToSendQueue(byte *data, int len) {
	dgrPacket_t *p;
	bool taken;
	if(len > MAX_DGR_PACKET) {
		addLogAdv(LOG_INFO, LOG_FEATURE_DGR, "DGR_AddToSendQueue: DGR packet too long - %i\n",len);
		return;
	}	
	if (g_mutex == 0)
	{
		g_mutex = xSemaphoreCreateMutex();
	}
	taken = xSemaphoreTake(g_mutex, 10);
	if (taken == false) {
		return;
	}
	p = dgr_pending;
	while(p) {
		if(p->length == 0) {
			// this packet can be reused
			break;
		}
		p = p->next;
	}
	if(p == 0) {
		if (dgr_total_alloced_queue_size >= MAX_DGR_QUEUE_SIZE) {
			addLogAdv(LOG_INFO, LOG_FEATURE_DGR, "DGR_AddToSendQueue: DGR queue grew to big, will drop packet\n");
			return;
		}
		dgr_total_alloced_queue_size++;
		p = malloc(sizeof(dgrPacket_t));
		p->next = dgr_pending;
		dgr_pending = p;
	}
	p->length = len;
	memcpy(p->buffer,data,len);
	xSemaphoreGive(g_mutex);
}
void DGR_FlushSendQueue() {
	dgrPacket_t *p;
    struct sockaddr_in addr;
	int nbytes;
	bool taken;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(dgr_group);
    addr.sin_port = htons(dgr_port);

	if (g_mutex == 0)
	{
		g_mutex = xSemaphoreCreateMutex();
	}
	taken = xSemaphoreTake(g_mutex, 1);
	if (taken == false) {
		return;
	}
	p = dgr_pending;
	while(p) {
		if(p->length != 0) {
			g_dgr_stat_sent++;
			nbytes = sendto(
				g_dgr_socket_send,
			   (const char*) p->buffer,
				p->length,
				0,
				(struct sockaddr*) &addr,
				sizeof(addr)
			);
#if 0
			rtos_delay_milliseconds(1);
			nbytes = sendto(
				g_dgr_socket_send,
			   (const char*) p->buffer,
				p->length,
				0,
				(struct sockaddr*) &addr,
				sizeof(addr)
			);
#endif
			p->length = 0;
		}
		p = p->next;
	}
	xSemaphoreGive(g_mutex);

}
byte Val255ToVal100(byte v){ 
	float fr;
	// convert to our 0-100 range
	fr = v / 255.0f;
	v = fr * 100;
	return v;
}
byte Val100ToVal255(byte v){ 
	float fr;
	fr = v / 100.0f;
	v = fr * 255;
	return v;
}
void DRV_DGR_CreateSocket_Send() {
    // create what looks like an ordinary UDP socket
    g_dgr_socket_send = socket(AF_INET, SOCK_DGRAM, 0);
    if (g_dgr_socket_send < 0) {
		addLogAdv(LOG_INFO, LOG_FEATURE_DGR,"DRV_DGR_CreateSocket_Send: failed to do socket\n");
        return;
    }
	addLogAdv(LOG_INFO, LOG_FEATURE_DGR,"DRV_DGR_CreateSocket_Send: socket created\n");
}
void DRV_DGR_Send_Generic(byte *message, int len) {
	// if this send is as a result of use RXing something, 
	// don't send it....
	if (g_inCmdProcessing){
		return;
	}

	g_dgr_send_seq++;

	// This is here only because sending UDP from MQTT callback crashes BK for me
	// So instead, we are making a queue which is sent in quick tick
	DGR_AddToSendQueue(message, len);
	addLogAdv(LOG_EXTRADEBUG, LOG_FEATURE_DGR, "DGR adds to queue %i",len);
}

void DRV_DGR_Dump(byte *message, int len){
	char tmp[100];
	char *p = tmp;
	for (int i = 0; i < len && i < 49; i++){
		sprintf(p, "%02X", message[i]);
		p+=2;
	}
	*p = 0;
	addLogAdv(LOG_INFO, LOG_FEATURE_DGR,"DRV_DGR_Send_Generic: %s",tmp);
}

void DRV_DGR_Send_Power(const char *groupName, int channelValues, int numChannels){
	int len;
	byte message[64];
	// if this send is as a result of use RXing something, 
	// don't send it....
	if (g_inCmdProcessing){
		return;
	}

	len = DGR_Quick_FormatPowerState(message,sizeof(message),groupName,g_dgr_send_seq, 0,channelValues, numChannels);

	DRV_DGR_Send_Generic(message,len);
}
void DRV_DGR_Send_Brightness(const char *groupName, byte brightness){
	int len;
	byte message[64];
	// if this send is as a result of use RXing something, 
	// don't send it....
	if (g_inCmdProcessing){
		return;
	}

	len = DGR_Quick_FormatBrightness(message,sizeof(message),groupName,g_dgr_send_seq, 0, brightness);

	DRV_DGR_Send_Generic(message,len);
}
void DRV_DGR_Send_RGBCW(const char *groupName, byte *rgbcw){
	int len;
	byte message[64];
	// if this send is as a result of use RXing something, 
	// don't send it....
	if (g_inCmdProcessing){
		return;
	}

	len = DGR_Quick_FormatRGBCW(message,sizeof(message),groupName,g_dgr_send_seq, 0, rgbcw[0],rgbcw[1],rgbcw[2],rgbcw[3],rgbcw[4]);

	DRV_DGR_Send_Generic(message,len);
}
void DRV_DGR_Send_FixedColor(const char *groupName, int colorIndex) {
	int len;
	byte message[64];
	// if this send is as a result of use RXing something, 
	// don't send it....
	if (g_inCmdProcessing) {
		return;
	}

	len = DGR_Quick_FormatFixedColor(message, sizeof(message), groupName, g_dgr_send_seq, 0, colorIndex);

	DRV_DGR_Send_Generic(message, len);
}
void DRV_DGR_CreateSocket_Receive() {

    struct sockaddr_in addr;
    struct ip_mreq mreq;
    int flag = 1;
	int broadcast = 0;
	int iResult = 1;

    // create what looks like an ordinary UDP socket
    g_dgr_socket_receive = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (g_dgr_socket_receive < 0) {
		g_dgr_socket_receive = -1;
		addLogAdv(LOG_INFO, LOG_FEATURE_DGR,"DRV_DGR_CreateSocket_Receive: failed to do socket\n");
        return ;
    }

	if(broadcast)
	{
		iResult = setsockopt(g_dgr_socket_receive, SOL_SOCKET, SO_BROADCAST, (char *)&flag, sizeof(flag));
		if (iResult != 0)
		{
			addLogAdv(LOG_INFO, LOG_FEATURE_DGR,"DRV_DGR_CreateSocket_Receive: failed to do setsockopt SO_BROADCAST\n");
			close(g_dgr_socket_receive);
			g_dgr_socket_receive = -1;
			return ;
		}
	}
	else{
		// allow multiple sockets to use the same PORT number
		if (
			setsockopt(
				g_dgr_socket_receive, SOL_SOCKET, SO_REUSEADDR, (char*) &flag, sizeof(flag)
			) < 0
		){
			addLogAdv(LOG_INFO, LOG_FEATURE_DGR,"DRV_DGR_CreateSocket_Receive: failed to do setsockopt SO_REUSEADDR\n");
			close(g_dgr_socket_receive);
			g_dgr_socket_receive = -1;
		  return ;
		}
	}

    // set up destination address
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY); // differs from sender
    addr.sin_port = htons(dgr_port);

    // bind to receive address
    if (bind(g_dgr_socket_receive, (struct sockaddr*) &addr, sizeof(addr)) < 0) {
		addLogAdv(LOG_INFO, LOG_FEATURE_DGR,"DRV_DGR_CreateSocket_Receive: failed to do bind\n");
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
		mreq.imr_multiaddr.s_addr = inet_addr(dgr_group);
		//mreq.imr_interface.s_addr = htonl(INADDR_ANY);
		mreq.imr_interface.s_addr = htonl(INADDR_ANY);//inet_addr(MY_CAPTURE_IP);
    	///mreq.imr_interface.s_addr = inet_addr("192.168.0.122");
		iResult = setsockopt(g_dgr_socket_receive, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*) &mreq, sizeof(mreq));
		if (iResult < 0) {
			addLogAdv(LOG_INFO, LOG_FEATURE_DGR,"DRV_DGR_CreateSocket_Receive: failed to do setsockopt IP_ADD_MEMBERSHIP %i\n",iResult);
			close(g_dgr_socket_receive);
			g_dgr_socket_receive = -1;
			return ;
		}
	}

	lwip_fcntl(g_dgr_socket_receive, F_SETFL,O_NONBLOCK);

	addLogAdv(LOG_INFO, LOG_FEATURE_DGR,"DRV_DGR_CreateSocket_Receive: Socket created, waiting for packets\n");
}

void DRV_DGR_processRGBCW(byte *rgbcw) {
	addLogAdv(LOG_DEBUG, LOG_FEATURE_DGR, "DRV_DGR_setFinalRGBCW: %i,%i,%i,%i,%i\n", (int)rgbcw[0], (int)rgbcw[1], (int)rgbcw[2], (int)rgbcw[3], (int)rgbcw[4]);

	LED_SetFinalRGBCW(rgbcw);
}
void DRV_DGR_processPower(int relayStates, byte relaysCount) {
	int startIndex;
	int i;
	int ch;

	addLogAdv(LOG_DEBUG, LOG_FEATURE_DGR, "DRV_DGR_processPower: cnt %i, val %i\n", (int)relaysCount, relayStates);

	if(PIN_CountPinsWithRoleOrRole(IOR_PWM,IOR_PWM_n) > 0 || LED_IsLedDriverChipRunning()) {
		LED_SetEnableAll(BIT_CHECK(relayStates,0));
	} else {
		// does indexing starts with zero?
		if(CHANNEL_HasChannelPinWithRoleOrRole(0, IOR_Relay, IOR_Relay_n)) {
			startIndex = 0;
		} else {
			startIndex = 1;
		}
		for(i = 0; i < relaysCount; i++) {
			int bOn;
			bOn = BIT_CHECK(relayStates,i);
			ch = startIndex+i;
			if(bOn) {
				if(CHANNEL_HasChannelPinWithRoleOrRole(ch,IOR_PWM,IOR_PWM_n)) {

				} else {
					CHANNEL_Set(ch,1,0);
				}
			} else {
				CHANNEL_Set(ch,0,0);
			}
		}
	}
}
void DRV_DGR_processBrightnessPowerOn(byte brightness) {
	addLogAdv(LOG_DEBUG, LOG_FEATURE_DGR,"DRV_DGR_processBrightnessPowerOn: %i\n",(int)brightness);

	LED_SetDimmer(Val255ToVal100(brightness));
}
void DRV_DGR_processLightFixedColor(byte fixedColor) {
	addLogAdv(LOG_DEBUG, LOG_FEATURE_DGR, "DRV_DGR_processLightFixedColor: %i\n", (int)fixedColor);

	LED_SetColorByIndex(fixedColor);
}
void DRV_DGR_processLightBrightness(byte brightness) {
	addLogAdv(LOG_DEBUG, LOG_FEATURE_DGR,"DRV_DGR_processLightBrightness: %i\n",(int)brightness);

	LED_SetDimmer(Val255ToVal100(brightness));
}
typedef struct dgrMmember_s {
	int ip;
	uint16_t lastSeq;
} dgrMember_t;

#define MAX_DGR_MEMBERS 32
static dgrMember_t g_dgrMembers[MAX_DGR_MEMBERS];
static int g_curDGRMembers = 0;
static struct sockaddr_in addr;

dgrMember_t *findMember() {
	int i, ip;

	ip = addr.sin_addr.s_addr;;

	for(i = 0; i < g_curDGRMembers; i++) {
		if(g_dgrMembers[i].ip == ip) {
			return &g_dgrMembers[i];
		}
	}
	i = g_curDGRMembers;
	if(i>=MAX_DGR_MEMBERS)
		return 0;
	g_curDGRMembers ++;
	g_dgrMembers[i].ip = ip;
	g_dgrMembers[i].lastSeq = 0;
	return &g_dgrMembers[i];
}

int DGR_CheckSequence(uint16_t seq) {
	dgrMember_t *m;
	
	m = findMember();
	
	if (m == 0) {
		addLogAdv(LOG_EXTRADEBUG, LOG_FEATURE_DGR, "DGR_CheckSequence: no member found");
		return 1;
	}
	addLogAdv(LOG_EXTRADEBUG, LOG_FEATURE_DGR, "DGR_CheckSequence: argument %i, last %i",(int)seq, (int)m->lastSeq);
	
	// make it work past wrap at
	if((seq > m->lastSeq) || (seq+10 > m->lastSeq+10)) {
		if(seq != (m->lastSeq+1)){
			//addLogAdv(LOG_INFO, LOG_FEATURE_DGR,"Seq for %s skip %i->%i\n",inet_ntoa(m->ip), m->lastSeq, seq);
		}
		addLogAdv(LOG_EXTRADEBUG, LOG_FEATURE_DGR, "Seq ok");
		m->lastSeq = seq;
		return 0;
	}
	if(seq + 16 < m->lastSeq) {
		addLogAdv(LOG_EXTRADEBUG, LOG_FEATURE_DGR, "Seq Hard reset");
		// hard reset
		m->lastSeq = seq;
		return 0;
	}
	addLogAdv(LOG_EXTRADEBUG, LOG_FEATURE_DGR, "Seq failed");
	return 1;
}

void DRV_DGR_RunEverySecond() {
	const char *myip;

	// TODO: do it only on IP change?
	myip = HAL_GetMyIPString();
	g_mySockAddr.sin_addr.s_addr = inet_addr(myip);

	if(g_dgr_socket_receive<=0 || g_dgr_socket_send <= 0) {
		dgr_retry_time_left--;
		addLogAdv(LOG_INFO, LOG_FEATURE_DGR,"no sockets, will retry creation soon, in %i secs\n",dgr_retry_time_left);

		if(dgr_retry_time_left <= 0){
			dgr_retry_time_left = 5;
			if(g_dgr_socket_receive <= 0){
				DRV_DGR_CreateSocket_Receive();
			}
			if(g_dgr_socket_send <= 0){
				DRV_DGR_CreateSocket_Send();
			}
		}
	}
}
void DGR_SpoofNextDGRPacketSource(const char *ipStrs) {
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(ipStrs);
	addr.sin_port = htons(dgr_port);
}
void DGR_ProcessIncomingPacket(char *msgbuf, int nbytes) {
	dgrDevice_t def;

	msgbuf[nbytes] = '\0';

	strcpy(def.gr.groupName, CFG_DeviceGroups_GetName());
	def.gr.devGroupShare_In = CFG_DeviceGroups_GetRecvFlags();
	def.gr.devGroupShare_Out = CFG_DeviceGroups_GetSendFlags();
	def.cbs.processBrightnessPowerOn = DRV_DGR_processBrightnessPowerOn;
	def.cbs.processLightBrightness = DRV_DGR_processLightBrightness;
	def.cbs.processLightFixedColor = DRV_DGR_processLightFixedColor;
	def.cbs.processPower = DRV_DGR_processPower;
	def.cbs.processRGBCW = DRV_DGR_processRGBCW;
	def.cbs.checkSequence = DGR_CheckSequence;

	// don't send things that result from something we rxed...
	g_inCmdProcessing = 1;
#ifdef DGRLOADMOREDEBUG	
	DRV_DGR_Dump((byte*)msgbuf, nbytes);
#endif
	DGR_Parse((byte*)msgbuf, nbytes, &def, (struct sockaddr *)&addr);
	g_inCmdProcessing = 0;

}

void DRV_DGR_RunQuickTick() {
    char msgbuf[64];
	socklen_t addrlen;
	int nbytes;
	int i;

	if(g_dgr_socket_receive<=0 || g_dgr_socket_send <= 0) {
		return ;
	}
    // send pending
	DGR_FlushSendQueue();
	
	// NOTE: 'addr' is global, and used in callbacks to determine the member.
	for (i = 0; i < 10; i++) {
		addrlen = sizeof(addr);
		nbytes = recvfrom(
			g_dgr_socket_receive,
			msgbuf,
			sizeof(msgbuf),
			0,
			(struct sockaddr *) &addr,
			&addrlen
		);
		if (nbytes <= 0) {
			return;
		}

		if (g_mySockAddr.sin_addr.s_addr == addr.sin_addr.s_addr) {
			continue;
		}

		g_dgr_stat_received++;

		// IMPORTANT: do not call inet_ntoa if log level is not extradebug...
		if (g_loglevel >= LOG_EXTRADEBUG) {
			addLogAdv(LOG_EXTRADEBUG, LOG_FEATURE_DGR, "Received %i bytes from %s\n", nbytes, inet_ntoa(((struct sockaddr_in *)&addr)->sin_addr));
		}

		DGR_ProcessIncomingPacket(msgbuf, nbytes);
	}
}
void DRV_DGR_Shutdown()
{
	if(g_dgr_socket_receive>=0) {
#if WINDOWS
		closesocket(g_dgr_socket_receive);
#else
		close(g_dgr_socket_receive);
#endif
		g_dgr_socket_receive = -1;
	}
	if (g_dgr_socket_send >= 0) {
#if WINDOWS
		closesocket(g_dgr_socket_send);
#else
		close(g_dgr_socket_send);
#endif
		g_dgr_socket_send = -1;
	}
	dgr_retry_time_left = 5;
	g_inCmdProcessing = 0;
	g_dgr_send_seq = 0;
}

void DRV_DGR_AppendInformationToHTTPIndexPage(http_request_t* request) {
	hprintf255(request, "<h4>DGR received: %i, send: %i</h4>", g_dgr_stat_received, g_dgr_stat_sent);
}
// DGR_SendPower testSocket 1 1
// DGR_SendPower stringGroupName integerChannelValues integerChannelsCount
commandResult_t CMD_DGR_SendPower(const void *context, const char *cmd, const char *args, int flags) {
	const char *groupName;
	int channelValues;
	int channelsCount;

	Tokenizer_TokenizeString(args,0);
	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 3)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	groupName = Tokenizer_GetArg(0);
	channelValues = Tokenizer_GetArgInteger(1);
	channelsCount = Tokenizer_GetArgInteger(2);

	DRV_DGR_Send_Power(groupName,channelValues,channelsCount);
	addLogAdv(LOG_INFO, LOG_FEATURE_DGR,"CMD_DGR_SendPower: sent message to group %s\n",groupName);

	return CMD_RES_OK;
}
void DRV_DGR_OnLedDimmerChange(int iVal) {
	//addLogAdv(LOG_INFO, LOG_FEATURE_DGR,"DRV_DGR_OnLedDimmerChange: called\n");
	if (g_dgr_socket_receive == 0) {
		return;
	}
	// if this send is as a result of use RXing something, 
	// don't send it....
	if (g_inCmdProcessing) {
		return;
	}
	if ((CFG_DeviceGroups_GetSendFlags() & DGR_SHARE_LIGHT_BRI) == 0) {

		return;
	}
	DRV_DGR_Send_Brightness(CFG_DeviceGroups_GetName(), Val100ToVal255(iVal));
}

void DRV_DGR_OnLedFinalColorsChange(byte rgbcw[5]) {
	//addLogAdv(LOG_INFO, LOG_FEATURE_DGR,"DRV_DGR_OnLedFinalColorsChange: called\n");
	if (g_dgr_socket_receive == 0) {
		return;
	}
	// if this send is as a result of use RXing something, 
	// don't send it....
	if (g_inCmdProcessing) {
		return;
	}
	if ((CFG_DeviceGroups_GetSendFlags() & DGR_SHARE_LIGHT_COLOR) == 0) {

		return;
	}
	DRV_DGR_Send_RGBCW(CFG_DeviceGroups_GetName(), rgbcw);
}


void DRV_DGR_OnLedEnableAllChange(int iVal) {
	if(g_dgr_socket_receive==0) {
		return;
	}
	// if this send is as a result of use RXing something, 
	// don't send it....
	if (g_inCmdProcessing){
		return;
	}

	if((CFG_DeviceGroups_GetSendFlags() & DGR_SHARE_POWER)==0) {

		return;
	}

	DRV_DGR_Send_Power(CFG_DeviceGroups_GetName(), iVal, 1);
}
void DRV_DGR_OnChannelChanged(int ch, int value) {
	int channelValues;
	int channelsCount;
	int i;
	const char *groupName;
	int firstChannelOffset;

	if(g_dgr_socket_receive==0) {
		return;
	}
	// if this send is as a result of use RXing something, 
	// don't send it....
	if (g_inCmdProcessing){
		return;
	}

	if((CFG_DeviceGroups_GetSendFlags() & DGR_SHARE_POWER)==0) {

		return;
	}
	channelValues = 0;
	channelsCount = 0;
	groupName = CFG_DeviceGroups_GetName();

	// we have channel indices starting from 0 but some people start with 1
	// check if we need to offset
	if (CHANNEL_HasChannelPinWithRole(0, IOR_Relay) || CHANNEL_HasChannelPinWithRole(0, IOR_Relay_n)
		|| CHANNEL_HasChannelPinWithRole(0, IOR_LED) || CHANNEL_HasChannelPinWithRole(0, IOR_LED_n)) {
		firstChannelOffset = 0;
	}
	else {
		firstChannelOffset = 1;
	}


	for(i = 0; i < CHANNEL_MAX-1; i++) {
		int chIndex = i + firstChannelOffset;
		if(CHANNEL_HasChannelPinWithRole(chIndex,IOR_Relay) || CHANNEL_HasChannelPinWithRole(chIndex,IOR_Relay_n)
			|| CHANNEL_HasChannelPinWithRole(chIndex,IOR_LED) || CHANNEL_HasChannelPinWithRole(chIndex,IOR_LED_n)
			|| CHANNEL_HasChannelPinWithRole(chIndex, IOR_Button) || CHANNEL_HasChannelPinWithRole(chIndex, IOR_Button_n)) {
			channelsCount = i + 1;
			if(CHANNEL_Get(chIndex)) {
				BIT_SET(channelValues ,i);
			}
		} 
	}
	if(channelsCount>0){
		DRV_DGR_Send_Power(groupName,channelValues,channelsCount);
	}
}
// DGR_SendBrightness roomLEDstrips 128
// DGR_SendBrightness stringGroupName integerBrightness
commandResult_t CMD_DGR_SendBrightness(const void *context, const char *cmd, const char *args, int flags) {
	const char *groupName;
	int brightness;

	Tokenizer_TokenizeString(args,0);
	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 2)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	groupName = Tokenizer_GetArg(0);
	brightness = Tokenizer_GetArgInteger(1);

	DRV_DGR_Send_Brightness(groupName,brightness);
	addLogAdv(LOG_INFO, LOG_FEATURE_DGR,"DGR_SendBrightness: sent message to group %s\n",groupName);

	return CMD_RES_OK;
}
// DGR_SendRGBCW roomLEDstrips 255 0 0
// DGR_SendRGBCW stringGroupName r g b
// Alternate usage:
// DGR_SendRGBCW roomLEDstrips FF00BB
commandResult_t CMD_DGR_SendRGBCW(const void *context, const char *cmd, const char *args, int flags) {
	const char *groupName;
	byte rgbcw[5];
	const char *c;
	int i;
	char tmp[3];
	int val = 0;

	Tokenizer_TokenizeString(args,0);
	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 2)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	groupName = Tokenizer_GetArg(0);
	if (Tokenizer_GetArgsCount() == 2) {
		c = Tokenizer_GetArg(1);
		if (*c == '#')
			c++;
		i = 0;
		while (*c && i < 5) {
			int r;
			tmp[0] = *(c++);
			if (!*c)
				break;
			tmp[1] = *(c++);
			tmp[2] = '\0';
			r = sscanf(tmp, "%x", &val);
			rgbcw[i] = val;
			i++;
		}
	}
	else {
		rgbcw[0] = Tokenizer_GetArgInteger(1);
		rgbcw[1] = Tokenizer_GetArgInteger(2);
		rgbcw[2] = Tokenizer_GetArgInteger(3);
		rgbcw[3] = Tokenizer_GetArgInteger(4);
		rgbcw[4] = Tokenizer_GetArgInteger(5);
	}

	DRV_DGR_Send_RGBCW(groupName,rgbcw);
	addLogAdv(LOG_INFO, LOG_FEATURE_DGR,"DGR_SendRGBCW: sent message to group %s\n",groupName);

	return CMD_RES_OK;
}
// CMD_DGR_SendFixedColor stringGroupName tasmotaColorIndex
commandResult_t CMD_DGR_SendFixedColor(const void *context, const char *cmd, const char *args, int flags) {
	const char *groupName;
	int colorIndex;

	Tokenizer_TokenizeString(args, 0);
	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 2)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	groupName = Tokenizer_GetArg(0);
	colorIndex = Tokenizer_GetArgInteger(1);

	DRV_DGR_Send_FixedColor(groupName, colorIndex);
	addLogAdv(LOG_INFO, LOG_FEATURE_DGR, "CMD_DGR_SendFixedColor: sent message to group %s\n", groupName);

	return CMD_RES_OK;
}
void DRV_DGR_Init()
{
	memset(&g_dgrMembers[0],0,sizeof(g_dgrMembers));
	g_curDGRMembers = 0;

	DRV_DGR_CreateSocket_Receive();
	DRV_DGR_CreateSocket_Send();

	//cmddetail:{"name":"DGR_SendPower","args":"[GroupName][ChannelValues][ChannelsCount]",
	//cmddetail:"descr":"Sends a POWER message to given Tasmota Device Group with no reliability. Requires no prior setup and can control any group, but won't retransmit.",
	//cmddetail:"fn":"CMD_DGR_SendPower","file":"driver/drv_tasmotaDeviceGroups.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("DGR_SendPower", CMD_DGR_SendPower, NULL);
	//cmddetail:{"name":"DGR_SendBrightness","args":"[GroupName][Brightness]",
	//cmddetail:"descr":"Sends a Brightness message to given Tasmota Device Group with no reliability. Requires no prior setup and can control any group, but won't retransmit.",
	//cmddetail:"fn":"CMD_DGR_SendBrightness","file":"driver/drv_tasmotaDeviceGroups.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("DGR_SendBrightness", CMD_DGR_SendBrightness, NULL);
	//cmddetail:{"name":"DGR_SendRGBCW","args":"[GroupName][HexRGBCW]",
	//cmddetail:"descr":"Sends a RGBCW message to given Tasmota Device Group with no reliability. Requires no prior setup and can control any group, but won't retransmit. You can use this command in two ways, first is like DGR_SendRGBCW GroupName 255 255 0, etc, second is DGR_SendRGBCW GroupName FF00FF00 etc etc.",
	//cmddetail:"fn":"CMD_DGR_SendRGBCW","file":"driver/drv_tasmotaDeviceGroups.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("DGR_SendRGBCW", CMD_DGR_SendRGBCW, NULL);
	//cmddetail:{"name":"DGR_SendFixedColor","args":"[GroupName][TasColorIndex]",
	//cmddetail:"descr":"Sends a FixedColor message to given Tasmota Device Group with no reliability. Requires no prior setup and can control any group, but won't retransmit.",
	//cmddetail:"fn":"CMD_DGR_SendFixedColor","file":"driver/drv_tasmotaDeviceGroups.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("DGR_SendFixedColor", CMD_DGR_SendFixedColor, NULL);
}



