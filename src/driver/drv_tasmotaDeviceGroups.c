

#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../logging/logging.h"
#include "../devicegroups/deviceGroups_public.h"
#include "../devicegroups/deviceGroups_local.h"
#include "../bitmessage/bitmessage_public.h"
#include "lwip/sockets.h"
#include "lwip/ip_addr.h"
#include "lwip/inet.h"
#include "../httpserver/new_http.h"

/* FreeRTOS compatibility: portTICK_PERIOD_MS fallback
   TXW81X and OpenRDA5981 don't define portTICK_PERIOD_MS.
   Other builds already have it defined, so the #ifndef guard protects them.
*/
#ifndef portTICK_PERIOD_MS
  #define portTICK_PERIOD_MS 2
#endif

#define MAX_DGR_PACKET 128
#define MAX_DGR_QUEUE_SIZE 8
#define MAX_DGR_MEMBERS 32

typedef struct dgrMember_s {
	uint32_t ip;
	uint16_t lastSeq;
	uint16_t acked_sequence;
	uint32_t unicast_count;
} dgrMember_t;

typedef struct dgrRuntime_s {
	uint16_t send_seq;
	uint32_t next_announcement_time;
	uint32_t initial_discovery_remaining;
	uint32_t next_discovery_time;
	uint16_t discovery_sequence;
	byte reliable_message[MAX_DGR_PACKET];
	uint16_t reliable_message_length;
	uint16_t reliable_sequence;
	uint16_t last_full_status_sequence;
	uint16_t incoming_flags;
	uint32_t no_status_share;
	uint32_t next_ack_check_time;
	uint32_t member_timeout_time;
	uint16_t ack_check_interval;
	int retry_member_cursor;
	dgrMember_t members[MAX_DGR_MEMBERS];
	int member_count;
} dgrRuntime_t;

static dgrRuntime_t g_dgr_groups[CFG_DEVICE_GROUP_MAX];
static dgrRuntime_t *g_dgr_current = &g_dgr_groups[0];
static int g_dgr_current_index = 0;

#define g_dgr_send_seq (g_dgr_current->send_seq)
#define g_dgr_next_announcement_time (g_dgr_current->next_announcement_time)
#define g_dgr_initial_discovery_remaining (g_dgr_current->initial_discovery_remaining)
#define g_dgr_next_discovery_time (g_dgr_current->next_discovery_time)
#define g_dgr_discovery_sequence (g_dgr_current->discovery_sequence)
#define g_dgr_reliable_message (g_dgr_current->reliable_message)
#define g_dgr_reliable_message_length (g_dgr_current->reliable_message_length)
#define g_dgr_reliable_sequence (g_dgr_current->reliable_sequence)
#define g_dgr_last_full_status_sequence (g_dgr_current->last_full_status_sequence)
#define g_dgr_incoming_flags (g_dgr_current->incoming_flags)
#define g_dgr_no_status_share (g_dgr_current->no_status_share)
#define g_dgr_next_ack_check_time (g_dgr_current->next_ack_check_time)
#define g_dgr_member_timeout_time (g_dgr_current->member_timeout_time)
#define g_dgr_ack_check_interval (g_dgr_current->ack_check_interval)
#define g_dgr_retry_member_cursor (g_dgr_current->retry_member_cursor)
#define g_dgrMembers (g_dgr_current->members)
#define g_curDGRMembers (g_dgr_current->member_count)

static void DRV_DGR_ProcessTextCommand(const char *value, byte length) {
	char command[MAX_DGR_PACKET];
	size_t copyLength = length;
	if (copyLength && value[copyLength - 1] == '\0') copyLength--;
	if (copyLength >= sizeof(command)) copyLength = sizeof(command) - 1;
	memcpy(command, value, copyLength);
	command[copyLength] = '\0';
	if (command[0]) CMD_ExecuteCommand(command, COMMAND_FLAG_SOURCE_SCRIPT);
}

static void DRV_DGR_ProcessEvent(const char *value, byte length) {
	char command[MAX_DGR_PACKET];
	size_t valueLength = length;
	if (valueLength && value[valueLength - 1] == '\0') valueLength--;
	if (valueLength > sizeof(command) - 7) valueLength = sizeof(command) - 7;
	memcpy(command, "event ", 6);
	memcpy(command + 6, value, valueLength);
	command[6 + valueLength] = '\0';
	CMD_ExecuteCommand(command, COMMAND_FLAG_SOURCE_SCRIPT);
}

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

const char *HAL_GetMyIPString();

static uint32_t DGR_TicksToMilliseconds(uint32_t ticks) {
	return ticks * portTICK_PERIOD_MS;
}

void DRV_DGR_Dump(byte *message, int len);
void DRV_DGR_SendFullStatus(const char *groupName);

static void DGR_SelectGroup(int index) {
	if (index < 0 || index >= CFG_DEVICE_GROUP_MAX) index = 0;
	g_dgr_current_index = index;
	g_dgr_current = &g_dgr_groups[index];
}

static int DGR_SelectGroupByName(const char *groupName) {
	int i;
	if (groupName) {
		for (i = 0; i < CFG_DEVICE_GROUP_MAX; i++) {
			const char *configuredName = CFG_DeviceGroups_GetNameByIndex(i);
			if (configuredName[0] && strcmp(configuredName, groupName) == 0) {
				DGR_SelectGroup(i);
				return i;
			}
		}
	}
	DGR_SelectGroup(0);
	return -1;
}

static void DGR_ResetRuntime(int index) {
	dgrRuntime_t *runtime = &g_dgr_groups[index];
	memset(runtime, 0, sizeof(*runtime));
	runtime->send_seq = 1;
	runtime->last_full_status_sequence = 0xFFFF;
	runtime->ack_check_interval = DGR_ACK_INITIAL_INTERVAL;
}

static void DGR_ResetAllRuntimes(void) {
	int i;
	for (i = 0; i < CFG_DEVICE_GROUP_MAX; i++) DGR_ResetRuntime(i);
	DGR_SelectGroup(0);
}

//
// A DGR outgoing packets queue mechanism.
// Used to send all DGR on quick tick 
// (instead of doing it in-place, from MQTT callback etc)
typedef struct dgrPacket_s {
	struct dgrPacket_s *next;
	byte buffer[MAX_DGR_PACKET];
	uint16_t length;
	uint32_t target_ip;  // 0 = multicast, non-zero = unicast to this IP
} dgrPacket_t;

// the list is not allocated before first use
dgrPacket_t *dgr_pending = 0;
dgrPacket_t *dgr_pending_tail = 0;
int dgr_total_alloced_queue_size = 0;

static SemaphoreHandle_t g_mutex = 0;

// Adds a packet to DGR send queue. Can be called from anywhere, MQTT callback, etc.
// We don't send UDP DGR packets directly from MQTT callback, because it would crash device in some cases....
static bool DGR_AddToSendQueueInternal(byte *data, int len, uint32_t target_ip) {
	dgrPacket_t *p;
	bool taken;
	if(data == 0 || len <= 0 || len > MAX_DGR_PACKET) {
		addLogAdv(LOG_INFO, LOG_FEATURE_DGR, "DGR_AddToSendQueue: invalid DGR packet length - %i", len);
		return false;
	}	
	if (g_mutex == 0)
	{
		g_mutex = xSemaphoreCreateMutex();
		if (g_mutex == 0) {
			return false;
		}
	}
	taken = xSemaphoreTake(g_mutex, 10);
	if (taken == false) {
		return false;
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
			addLogAdv(LOG_INFO, LOG_FEATURE_DGR, "DGR_AddToSendQueue: DGR queue grew to big, will drop packet");
			xSemaphoreGive(g_mutex);
			return false;
		}
		p = malloc(sizeof(dgrPacket_t));
		if (p == 0) {
			xSemaphoreGive(g_mutex);
			return false;
		}
		dgr_total_alloced_queue_size++;
		p->next = 0;
		if (dgr_pending_tail) {
			dgr_pending_tail->next = p;
		} else {
			dgr_pending = p;
		}
		dgr_pending_tail = p;
	}
	p->length = len;
	p->target_ip = target_ip;
	memcpy(p->buffer,data,len);
	xSemaphoreGive(g_mutex);
	return true;
}
void DGR_AddToSendQueue(byte *data, int len) {
	DGR_AddToSendQueueInternal(data, len, 0);
}
void DGR_AddToUnicastSendQueue(byte *data, int len, uint32_t target_ip) {
	DGR_AddToSendQueueInternal(data, len, target_ip);
}

static void DGR_RecordReliableMessage(byte *data, int len, uint16_t sequence) {
	uint32_t now;
	if (len <= 0 || len > MAX_DGR_PACKET) {
		return;
	}
	memcpy(g_dgr_reliable_message, data, len);
	g_dgr_reliable_message_length = len;
	g_dgr_reliable_sequence = sequence;
	g_dgr_ack_check_interval = DGR_ACK_INITIAL_INTERVAL;
	now = DGR_TicksToMilliseconds(xTaskGetTickCount());
	g_dgr_next_ack_check_time = now + g_dgr_ack_check_interval;
	g_dgr_member_timeout_time = now + DGR_MEMBER_TIMEOUT;
	g_dgr_next_announcement_time = now + DGR_ANNOUNCEMENT_INTERVAL;
	g_dgr_retry_member_cursor = 0;
}

void DGR_FlushSendQueue() {
	dgrPacket_t *p;
	int nbytes;
	bool taken;

	if (g_mutex == 0)
	{
		g_mutex = xSemaphoreCreateMutex();
		if (g_mutex == 0) {
			return;
		}
	}
	taken = xSemaphoreTake(g_mutex, 1);
	if (taken == false) {
		return;
	}
	p = dgr_pending;
	while(p) {
		if(p->length != 0) {
			struct sockaddr_in dest_addr;
			memset(&dest_addr, 0, sizeof(dest_addr));
			dest_addr.sin_family = AF_INET;
			dest_addr.sin_port = htons(dgr_port);
			if (p->target_ip != 0) {
				dest_addr.sin_addr.s_addr = p->target_ip;
			} else {
				dest_addr.sin_addr.s_addr = inet_addr(dgr_group);
			}
			g_dgr_stat_sent++;
			nbytes = sendto(
				g_dgr_socket_send,
			   (const char*) p->buffer,
				p->length,
				0,
				(struct sockaddr*) &dest_addr,
				sizeof(dest_addr)
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

#if WINDOWS
int DGR_GetPendingPacketCountForTest(void) {
	dgrPacket_t *p;
	int count = 0;
	bool taken;
	if (g_mutex == 0) {
		return 0;
	}
	taken = xSemaphoreTake(g_mutex, 10);
	if (taken == false) {
		return -1;
	}
	for (p = dgr_pending; p; p = p->next) {
		if (p->length != 0) {
			count++;
		}
	}
	xSemaphoreGive(g_mutex);
	return count;
}

int DGR_GetPendingPacketByteForTest(int packetIndex, int byteIndex) {
	dgrPacket_t *p;
	int pendingIndex = 0;
	int result = -1;
	bool taken;
	if (packetIndex < 0 || byteIndex < 0 || g_mutex == 0) {
		return -1;
	}
	taken = xSemaphoreTake(g_mutex, 10);
	if (taken == false) {
		return -1;
	}
	for (p = dgr_pending; p; p = p->next) {
		if (p->length == 0) continue;
		if (pendingIndex == packetIndex) {
			if (byteIndex < p->length) result = p->buffer[byteIndex];
			break;
		}
		pendingIndex++;
	}
	xSemaphoreGive(g_mutex);
	return result;
}

#endif
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
		addLogAdv(LOG_INFO, LOG_FEATURE_DGR,"DRV_DGR_CreateSocket_Send: failed to do socket");
        return;
    }
	addLogAdv(LOG_INFO, LOG_FEATURE_DGR,"DRV_DGR_CreateSocket_Send: socket created");
}
static void DRV_DGR_Send_Generic(byte *message, int len, const char *groupName, int groupIndex) {
	// if this send is as a result of use RXing something, 
	// don't send it....
	if (g_inCmdProcessing || g_dgr_initial_discovery_remaining){
		return;
	}
	if (g_dgr_reliable_message_length && groupIndex >= 0) {
		// Tasmota merges new state into an unacknowledged update. A fresh full-status
		// packet provides the same convergence guarantee with this simpler formatter.
		DRV_DGR_SendFullStatus(groupName);
		return;
	}

	// This is here only because sending UDP from MQTT callback crashes BK for me
	// So instead, we are making a queue which is sent in quick tick
	if (DGR_AddToSendQueueInternal(message, len, 0)) {
		if (groupIndex >= 0) {
			DGR_RecordReliableMessage(message, len, g_dgr_send_seq);
		}
		g_dgr_send_seq++;
		if (g_dgr_send_seq == 0) {
			g_dgr_send_seq = 1;
		}
	}
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
	int groupIndex;
	byte message[64];
	// if this send is as a result of use RXing something, 
	// don't send it....
	if (g_inCmdProcessing){
		return;
	}

	groupIndex = DGR_SelectGroupByName(groupName);
	len = DGR_Quick_FormatPowerState(message,sizeof(message),groupName,g_dgr_send_seq, 0,channelValues, numChannels);

	DRV_DGR_Send_Generic(message,len,groupName,groupIndex);
}
void DRV_DGR_Send_Brightness(const char *groupName, byte brightness){
	int len;
	int groupIndex;
	byte message[64];
	// if this send is as a result of use RXing something, 
	// don't send it....
	if (g_inCmdProcessing){
		return;
	}

	groupIndex = DGR_SelectGroupByName(groupName);
	len = DGR_Quick_FormatBrightness(message,sizeof(message),groupName,g_dgr_send_seq, 0, brightness);

	DRV_DGR_Send_Generic(message,len,groupName,groupIndex);
}
void DRV_DGR_Send_RGBCW(const char *groupName, byte *rgbcw){
	int len;
	int groupIndex;
	byte message[64];
	// if this send is as a result of use RXing something, 
	// don't send it....
	if (g_inCmdProcessing){
		return;
	}

	groupIndex = DGR_SelectGroupByName(groupName);
	len = DGR_Quick_FormatRGBCW(message,sizeof(message),groupName,g_dgr_send_seq, 0, rgbcw[0],rgbcw[1],rgbcw[2],rgbcw[3],rgbcw[4]);

	DRV_DGR_Send_Generic(message,len,groupName,groupIndex);
}
void DRV_DGR_Send_FixedColor(const char *groupName, int colorIndex) {
	int len;
	int groupIndex;
	byte message[64];
	// if this send is as a result of use RXing something, 
	// don't send it....
	if (g_inCmdProcessing) {
		return;
	}

	groupIndex = DGR_SelectGroupByName(groupName);
	len = DGR_Quick_FormatFixedColor(message, sizeof(message), groupName, g_dgr_send_seq, 0, colorIndex);

	DRV_DGR_Send_Generic(message, len, groupName, groupIndex);
}

// Send announcement message (heartbeat)
void DRV_DGR_SendAnnouncement(const char *groupName) {
	int len;
	byte message[64];

	// Announcements are always sent, even during command processing
	DGR_SelectGroupByName(groupName);
	len = DGR_Quick_FormatAnnouncement(message, sizeof(message), groupName, g_dgr_send_seq);

	// Don't increment sequence for announcements (they use current sequence)
	// But we still queue them
	DGR_AddToSendQueue(message, len);
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
		addLogAdv(LOG_INFO, LOG_FEATURE_DGR,"DRV_DGR_CreateSocket_Receive: failed to do socket");
        return ;
    }

	if(broadcast)
	{
		iResult = setsockopt(g_dgr_socket_receive, SOL_SOCKET, SO_BROADCAST, (char *)&flag, sizeof(flag));
		if (iResult != 0)
		{
			addLogAdv(LOG_INFO, LOG_FEATURE_DGR,"DRV_DGR_CreateSocket_Receive: failed to do setsockopt SO_BROADCAST");
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
			addLogAdv(LOG_INFO, LOG_FEATURE_DGR,"DRV_DGR_CreateSocket_Receive: failed to do setsockopt SO_REUSEADDR");
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
		addLogAdv(LOG_INFO, LOG_FEATURE_DGR,"DRV_DGR_CreateSocket_Receive: failed to do bind");
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
			addLogAdv(LOG_INFO, LOG_FEATURE_DGR,"DRV_DGR_CreateSocket_Receive: failed to do setsockopt IP_ADD_MEMBERSHIP %i",iResult);
			close(g_dgr_socket_receive);
			g_dgr_socket_receive = -1;
			return ;
		}
	}

	lwip_fcntl(g_dgr_socket_receive, F_SETFL,O_NONBLOCK);

	addLogAdv(LOG_INFO, LOG_FEATURE_DGR,"DRV_DGR_CreateSocket_Receive: Socket created, waiting for packets");
}

#if ENABLE_LED_BASIC
void DRV_DGR_processRGBCW(byte *rgbcw) {
	addLogAdv(LOG_DEBUG, LOG_FEATURE_DGR, "DRV_DGR_setFinalRGBCW: %i,%i,%i,%i,%i", (int)rgbcw[0], (int)rgbcw[1], (int)rgbcw[2], (int)rgbcw[3], (int)rgbcw[4]);

	LED_SetFinalRGBCW(rgbcw);
}
#endif
void DRV_DGR_processPower(int relayStates, byte relaysCount) {
	int startIndex;
	int i;
	int ch;

	addLogAdv(LOG_DEBUG, LOG_FEATURE_DGR, "DRV_DGR_processPower: cnt %i, val %i", (int)relaysCount, relayStates);

	if (CFG_DeviceGroups_GetTie(g_dgr_current_index) > 0) {
		int relay = CFG_DeviceGroups_GetTie(g_dgr_current_index);
		startIndex = CHANNEL_HasChannelPinWithRoleOrRole(0, IOR_Relay, IOR_Relay_n) ? 0 : 1;
		ch = startIndex + relay - 1;
		CHANNEL_Set(ch, BIT_CHECK(relayStates, 0), 0);
		return;
	}

#if ENABLE_LED_BASIC
	if(PIN_CountPinsWithRoleOrRole(IOR_PWM,IOR_PWM_n) > 0 || LED_IsLedDriverChipRunning()) {
		LED_SetEnableAll(BIT_CHECK(relayStates,0));
	} else 
#endif
	{
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
#if ENABLE_LED_BASIC
void DRV_DGR_processBrightnessPowerOn(byte brightness) {
	addLogAdv(LOG_DEBUG, LOG_FEATURE_DGR,"DRV_DGR_processBrightnessPowerOn: %i",(int)brightness);

	LED_SetDimmer(Val255ToVal100(brightness));
}
void DRV_DGR_processLightFixedColor(byte fixedColor) {
	addLogAdv(LOG_DEBUG, LOG_FEATURE_DGR, "DRV_DGR_processLightFixedColor: %i", (int)fixedColor);

	LED_SetColorByIndex(fixedColor);
}
void DRV_DGR_processLightBrightness(byte brightness) {
	addLogAdv(LOG_DEBUG, LOG_FEATURE_DGR,"DRV_DGR_processLightBrightness: %i",(int)brightness);

	LED_SetDimmer(Val255ToVal100(brightness));
}
#endif
static struct sockaddr_in addr;

#if WINDOWS
int DGR_GetMemberCountForTest(void) {
	return g_curDGRMembers;
}
#endif

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
	g_dgrMembers[i].acked_sequence = 0;
	g_dgrMembers[i].unicast_count = 0;
	return &g_dgrMembers[i];
}

static bool DGR_IsSequenceNewer(uint16_t sequence, uint16_t previous) {
	return previous == 0 || (int16_t)(sequence - previous) > 0;
}

int DGR_CheckSequence(uint16_t seq) {
	dgrMember_t *m;

	m = findMember();
	
	if (m == 0) {
		addLogAdv(LOG_EXTRADEBUG, LOG_FEATURE_DGR, "DGR_CheckSequence: no member found");
		return 1;
	}
	addLogAdv(LOG_EXTRADEBUG, LOG_FEATURE_DGR, "DGR_CheckSequence: argument %i, last %i",(int)seq, (int)m->lastSeq);
	
	if (!DGR_IsSequenceNewer(seq, m->lastSeq)) {
		addLogAdv(LOG_EXTRADEBUG, LOG_FEATURE_DGR, "Seq failed");
		return 1;
	}
	m->lastSeq = seq;
	addLogAdv(LOG_EXTRADEBUG, LOG_FEATURE_DGR, "Seq ok");
	return 0;
}

void DGR_HandleIncomingACK(uint16_t seq) {
	dgrMember_t *m = findMember();
	if (m == 0) {
		return;
	}
	/* Sequence zero is not transmitted and therefore serves as the initial
	 * sentinel.  The signed delta handles normal uint16_t wrap-around without
	 * allowing a delayed ACK to move the member backwards. */
	if (DGR_IsSequenceNewer(seq, m->acked_sequence)) {
		m->acked_sequence = seq;
	}
}

static bool DGR_TimeReached(uint32_t now, uint32_t deadline) {
	return (int32_t)(now - deadline) >= 0;
}

static void DGR_StartDiscovery(void) {
	uint32_t now = DGR_TicksToMilliseconds(xTaskGetTickCount());
	g_dgr_initial_discovery_remaining = 10;
	g_dgr_discovery_sequence = g_dgr_send_seq++;
	if (g_dgr_send_seq == 0) {
		g_dgr_send_seq = 1;
	}
	g_dgr_next_discovery_time = now + DGR_DISCOVERY_START_DELAY;
	g_dgr_next_announcement_time = 0xFFFFFFFF;
	g_dgr_reliable_message_length = 0;
	g_dgr_next_ack_check_time = 0;
}

#if WINDOWS
int DGR_IsTimeReachedForTest(uint32_t now, uint32_t deadline) {
	return DGR_TimeReached(now, deadline);
}
int DGR_IsSequenceNewerForTest(uint16_t sequence, uint16_t previous) {
	return DGR_IsSequenceNewer(sequence, previous);
}
uint32_t DGR_TicksToMillisecondsForTest(uint32_t ticks) {
	return DGR_TicksToMilliseconds(ticks);
}
#endif

void DRV_DGR_RunEverySecond() {
	const char *myip;
	uint32_t now;
	const char *groupName;
	int groupIndex;

	// TODO: do it only on IP change?
	myip = HAL_GetMyIPString();
	g_mySockAddr.sin_addr.s_addr = inet_addr(myip);

	if(g_dgr_socket_receive<=0 || g_dgr_socket_send <= 0) {
		dgr_retry_time_left--;
		addLogAdv(LOG_INFO, LOG_FEATURE_DGR,"no sockets, will retry creation soon, in %i secs",dgr_retry_time_left);

		if(dgr_retry_time_left <= 0){
			dgr_retry_time_left = 5;
				if(g_dgr_socket_receive <= 0){
					DRV_DGR_CreateSocket_Receive();
				}
				if(g_dgr_socket_send <= 0){
					DRV_DGR_CreateSocket_Send();
				}
				if (g_dgr_socket_receive > 0 && g_dgr_socket_send > 0) {
					for (groupIndex = 0; groupIndex < CFG_DEVICE_GROUP_MAX; groupIndex++) {
						groupName = CFG_DeviceGroups_GetNameByIndex(groupIndex);
						if (groupName[0]) {
							DGR_SelectGroup(groupIndex);
							DGR_StartDiscovery();
						}
					}
				}
		}
		return;
	}

	now = DGR_TicksToMilliseconds(xTaskGetTickCount());  // Current time in milliseconds
	for (groupIndex = 0; groupIndex < CFG_DEVICE_GROUP_MAX; groupIndex++) {
		groupName = CFG_DeviceGroups_GetNameByIndex(groupIndex);
		if (!groupName[0]) continue;
		DGR_SelectGroup(groupIndex);
		if (!g_dgr_initial_discovery_remaining && DGR_TimeReached(now, g_dgr_next_announcement_time)) {
			DRV_DGR_SendAnnouncement(groupName);
			g_dgr_next_announcement_time = now + DGR_ANNOUNCEMENT_INTERVAL + (rand() % 10000);
		}
	}
}
void DGR_SpoofNextDGRPacketSource(const char *ipStrs) {
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(ipStrs);
	addr.sin_port = htons(dgr_port);
}

void DRV_DGR_SendFullStatus(const char *groupName) {
	byte message[128];
	byte rgbcw[5] = { 0 };
	int len;
	int relayStates = 0;
	int numChannels = 0;
	int startIndex;
	int shareFlags = CFG_DeviceGroups_GetSendFlags();
	byte brightness = 0;
	byte scheme = 0;
	int i;
	int tie;
	DGR_SelectGroupByName(groupName);
	if (g_dgr_initial_discovery_remaining) {
		return;
	}
	tie = CFG_DeviceGroups_GetTie(g_dgr_current_index);

#if ENABLE_LED_BASIC
	if(PIN_CountPinsWithRoleOrRole(IOR_PWM, IOR_PWM_n) > 0 || LED_IsLedDriverChipRunning()) {
		relayStates = LED_GetEnableAll() ? 1 : 0;
		numChannels = 1;
		brightness = Val100ToVal255(LED_GetDimmer());
		scheme = LED_GetMode();
		LED_GetFinalRGBCW(rgbcw);
	} else
#endif
	{
		if (CHANNEL_HasChannelPinWithRoleOrRole(0, IOR_Relay, IOR_Relay_n)
			|| CHANNEL_HasChannelPinWithRoleOrRole(0, IOR_LED, IOR_LED_n)) {
			startIndex = 0;
		} else {
			startIndex = 1;
		}
		for (i = 0; i < CHANNEL_MAX - startIndex && i < 24; i++) {
			int ch = i + startIndex;
			if (CHANNEL_HasChannelPinWithRoleOrRole(ch, IOR_Relay, IOR_Relay_n)
				|| CHANNEL_HasChannelPinWithRoleOrRole(ch, IOR_LED, IOR_LED_n)) {
				numChannels = i + 1;
				if (CHANNEL_Get(ch)) {
					relayStates |= (1 << i);
				}
			}
		}
	}
	if (tie > 0) {
		startIndex = CHANNEL_HasChannelPinWithRoleOrRole(0, IOR_Relay, IOR_Relay_n) ? 0 : 1;
		relayStates = CHANNEL_Get(startIndex + tie - 1) ? 1 : 0;
		numChannels = 1;
	}

	len = DGR_Quick_FormatFullStatus(message, sizeof(message), groupName, g_dgr_send_seq,
		relayStates, numChannels, shareFlags, g_dgr_no_status_share, brightness, scheme, rgbcw);
	if (len > 0 && DGR_AddToSendQueueInternal(message, len, 0)) {
		// Tasmota multicasts full status so every member can refresh its view.
		DGR_RecordReliableMessage(message, len, g_dgr_send_seq);
		g_dgr_last_full_status_sequence = g_dgr_send_seq;
		g_dgr_send_seq++;
		if (g_dgr_send_seq == 0) {
			g_dgr_send_seq = 1;
		}
	}
}

static void DRV_DGR_SendFullStatus_Callback(void) {
	dgrMember_t *member = findMember();
	if ((g_dgr_incoming_flags & DGR_FLAG_RESET) || member == 0
		|| member->acked_sequence != g_dgr_last_full_status_sequence) {
		DRV_DGR_SendFullStatus(CFG_DeviceGroups_GetNameByIndex(g_dgr_current_index));
	}
}

static void DGR_RunDiscovery(uint32_t now) {
	byte message[64];
	const char *groupName;
	int flags;
	int len;

	if (!g_dgr_initial_discovery_remaining || !DGR_TimeReached(now, g_dgr_next_discovery_time)) {
		return;
	}
	groupName = CFG_DeviceGroups_GetNameByIndex(g_dgr_current_index);
	flags = DGR_FLAG_STATUS_REQUEST;
	if (g_dgr_initial_discovery_remaining == 10) {
		flags |= DGR_FLAG_RESET;
	}
	len = DGR_Quick_FormatStatusRequestWithFlags(message, sizeof(message), groupName,
		g_dgr_discovery_sequence, flags);
	if (!DGR_AddToSendQueueInternal(message, len, 0)) {
		g_dgr_next_discovery_time = now + DGR_DISCOVERY_INTERVAL;
		return;
	}
	g_dgr_initial_discovery_remaining--;
	if (g_dgr_initial_discovery_remaining) {
		g_dgr_next_discovery_time = now + DGR_DISCOVERY_INTERVAL;
	} else {
		uint16_t fullStatusSequence = g_dgr_send_seq;
		DRV_DGR_SendFullStatus(groupName);
		if (g_dgr_last_full_status_sequence != fullStatusSequence) {
			g_dgr_initial_discovery_remaining = 1;
			g_dgr_next_discovery_time = now + DGR_DISCOVERY_INTERVAL;
		}
	}
}

static void DGR_RunReliability(uint32_t now) {
	bool allAcked = true;
	int memberCount;
	int startCursor;
	int visited;
	int i;

	if (!g_dgr_reliable_message_length || !g_dgr_next_ack_check_time
		|| !DGR_TimeReached(now, g_dgr_next_ack_check_time)) {
		return;
	}
	if (DGR_TimeReached(now, g_dgr_member_timeout_time)) {
		for (i = 0; i < g_curDGRMembers; i++) {
			if (g_dgrMembers[i].acked_sequence == g_dgr_reliable_sequence) {
				continue;
			}
			if (i < g_curDGRMembers - 1) {
				memmove(&g_dgrMembers[i], &g_dgrMembers[i + 1],
					(g_curDGRMembers - i - 1) * sizeof(dgrMember_t));
			}
			g_curDGRMembers--;
			i--;
		}
		g_dgr_retry_member_cursor = 0;
	}
	memberCount = g_curDGRMembers;
	startCursor = g_dgr_retry_member_cursor;
	for (visited = 0; visited < memberCount; visited++) {
		i = (startCursor + visited) % memberCount;
		if (g_dgrMembers[i].acked_sequence != g_dgr_reliable_sequence) {
			allAcked = false;
			if (!DGR_AddToSendQueueInternal(g_dgr_reliable_message,
				g_dgr_reliable_message_length, g_dgrMembers[i].ip)) {
				break;
			}
			g_dgrMembers[i].unicast_count++;
			g_dgr_retry_member_cursor = (i + 1) % memberCount;
		}
	}
	if (allAcked) {
		g_dgr_reliable_message_length = 0;
		g_dgr_next_ack_check_time = 0;
	} else {
		g_dgr_ack_check_interval *= 2;
		if (g_dgr_ack_check_interval > DGR_ACK_MAX_INTERVAL) {
			g_dgr_ack_check_interval = DGR_ACK_MAX_INTERVAL;
		}
		g_dgr_next_ack_check_time = now + g_dgr_ack_check_interval;
	}
}

void DGR_ProcessIncomingPacket(char *msgbuf, int nbytes) {
	dgrDevice_t def;
	bitMessage_t msg;
	char groupName[32];
	uint16_t sequence;
	uint16_t flags;
	dgrMember_t *member;

	if (msgbuf == 0 || nbytes <= 0 || nbytes >= 128) {
		return;
	}
	if (nbytes < 128) {
		msgbuf[nbytes] = '\0';
	}

	// Parse the header to get group name, sequence, and flags
	MSG_BeginReading(&msg, (byte*)msgbuf, nbytes);
	if(MSG_CheckAndSkip(&msg, TASMOTA_DEVICEGROUPS_HEADER, strlen(TASMOTA_DEVICEGROUPS_HEADER)) == 0) {
		return;  // Bad header
	}
	if(MSG_ReadString(&msg, groupName, sizeof(groupName)) <= 0) {
		return;  // Failed to read group name
	}
	if (msg.position + 4 > nbytes) {
		return;  // Missing sequence or flags
	}
	sequence = MSG_ReadU16(&msg);
	flags = MSG_ReadU16(&msg);
	if (DGR_SelectGroupByName(groupName) < 0) {
		return;
	}
	g_dgr_incoming_flags = flags;
	member = findMember();
	if (member == 0) {
		return;
	}
	/* A peer starts discovery with RESET|STATUS_REQUEST after reboot. Seed its
	 * receive sequence here so its following low-numbered status update is not
	 * rejected as stale state from the previous boot. */
	if (flags & DGR_FLAG_RESET) {
		member->lastSeq = sequence;
	}

	// Intercept ACKs before DGR_Parse — update acked_sequence only, never lastSeq
	if (flags == DGR_FLAG_ACK) {
		DGR_HandleIncomingACK(sequence);
		return;
	}

	memset(&def, 0, sizeof(def));
	strcpy_safe(def.gr.groupName, CFG_DeviceGroups_GetNameByIndex(g_dgr_current_index), sizeof(def.gr.groupName));
	def.gr.devGroupShare_In = CFG_DeviceGroups_GetRecvFlags();
	def.gr.devGroupShare_Out = CFG_DeviceGroups_GetSendFlags();
	def.gr.noStatusShare = &g_dgr_no_status_share;
	def.gr.stateIndex = g_dgr_current_index;
#if ENABLE_LED_BASIC
	def.cbs.processBrightnessPowerOn = DRV_DGR_processBrightnessPowerOn;
	def.cbs.processLightBrightness = DRV_DGR_processLightBrightness;
	def.cbs.processLightFixedColor = DRV_DGR_processLightFixedColor;
	def.cbs.processRGBCW = DRV_DGR_processRGBCW;
#endif
	def.cbs.processPower = DRV_DGR_processPower;
	def.cbs.processEvent = DRV_DGR_ProcessEvent;
	def.cbs.processCommand = DRV_DGR_ProcessTextCommand;
	def.cbs.checkSequence = DGR_CheckSequence;
	def.cbs.sendFullStatus = DRV_DGR_SendFullStatus_Callback;

	// don't send things that result from something we rxed...
	g_inCmdProcessing = 1;
#ifdef DGRLOADMOREDEBUG	
	DRV_DGR_Dump((byte*)msgbuf, nbytes);
#endif
	if (DGR_Parse((byte*)msgbuf, nbytes, &def, (struct sockaddr *)&addr) < 0) {
		g_inCmdProcessing = 0;
		return;
	}

	// Exact ACKs returned above; Tasmota ACKs every other received message except announcements and partial packets.
	if(flags != DGR_FLAG_ANNOUNCEMENT && !(flags & DGR_FLAG_MORE_TO_COME)) {
		byte ackBuffer[64];
		int ackLen = DGR_Quick_FormatACK(ackBuffer, sizeof(ackBuffer), groupName, sequence);
		if(ackLen > 0) {
			DGR_AddToUnicastSendQueue(ackBuffer, ackLen, addr.sin_addr.s_addr);
		}
	}

	g_inCmdProcessing = 0;

}

void DRV_DGR_RunQuickTick() {
    char msgbuf[128];
	socklen_t addrlen;
	int nbytes;
	int i;

	if(g_dgr_socket_receive<=0 || g_dgr_socket_send <= 0) {
		return ;
	}
	{
		uint32_t now = DGR_TicksToMilliseconds(xTaskGetTickCount());
		int groupIndex;
		for (groupIndex = 0; groupIndex < CFG_DEVICE_GROUP_MAX; groupIndex++) {
			if (!CFG_DeviceGroups_GetNameByIndex(groupIndex)[0]) continue;
			DGR_SelectGroup(groupIndex);
			DGR_RunDiscovery(now);
			DGR_RunReliability(now);
		}
	}
    // send pending
	DGR_FlushSendQueue();
	
	// NOTE: 'addr' is global, and used in callbacks to determine the member.
	for (i = 0; i < 10; i++) {
		addrlen = sizeof(addr);
		nbytes = recvfrom(
			g_dgr_socket_receive,
			msgbuf,
			sizeof(msgbuf) - 1,
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
			addLogAdv(LOG_EXTRADEBUG, LOG_FEATURE_DGR, "Received %i bytes from %s", nbytes, inet_ntoa(((struct sockaddr_in *)&addr)->sin_addr));
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
	DGR_ResetAllRuntimes();
}

void DRV_DGR_AppendInformationToHTTPIndexPage(http_request_t* request, int bPreState) {
	if (bPreState){
		return;
	}
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
	addLogAdv(LOG_INFO, LOG_FEATURE_DGR,"CMD_DGR_SendPower: sent message to group %s",groupName);

	return CMD_RES_OK;
}
void DRV_DGR_OnLedDimmerChange(int iVal) {
	//addLogAdv(LOG_INFO, LOG_FEATURE_DGR,"DRV_DGR_OnLedDimmerChange: called");
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
	//addLogAdv(LOG_INFO, LOG_FEATURE_DGR,"DRV_DGR_OnLedFinalColorsChange: called");
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
	int groupIndex;
	int relayNumber;

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
	groupName = CFG_DeviceGroups_GetNameByIndex(0);

	// we have channel indices starting from 0 but some people start with 1
	// check if we need to offset
	if (CHANNEL_HasChannelPinWithRole(0, IOR_Relay) || CHANNEL_HasChannelPinWithRole(0, IOR_Relay_n)
		|| CHANNEL_HasChannelPinWithRole(0, IOR_LED) || CHANNEL_HasChannelPinWithRole(0, IOR_LED_n)) {
		firstChannelOffset = 0;
	}
	else {
		firstChannelOffset = 1;
	}
	relayNumber = ch - firstChannelOffset + 1;
	for (groupIndex = 0; groupIndex < CFG_DEVICE_GROUP_MAX; groupIndex++) {
		int tie = CFG_DeviceGroups_GetTie(groupIndex);
		const char *tiedGroup = CFG_DeviceGroups_GetNameByIndex(groupIndex);
		if (tie > 0 && tie == relayNumber && tiedGroup[0]) {
			DRV_DGR_Send_Power(tiedGroup, value ? 1 : 0, 1);
		}
	}
	if (CFG_DeviceGroups_GetTie(0) > 0) return;


	for(i = 0; i < CHANNEL_MAX-1; i++) {
		int chIndex = i + firstChannelOffset;
		if(CHANNEL_HasChannelPinWithRole(chIndex,IOR_Relay) || CHANNEL_HasChannelPinWithRole(chIndex,IOR_Relay_n)
			|| CHANNEL_HasChannelPinWithRole(chIndex,IOR_LED) || CHANNEL_HasChannelPinWithRole(chIndex,IOR_LED_n)
			|| CHANNEL_HasChannelPinWithRole(chIndex, IOR_Button) || CHANNEL_HasChannelPinWithRole(chIndex, IOR_Button_n)
			|| CHANNEL_HasChannelPinWithRole(chIndex, IOR_Button_pd) || CHANNEL_HasChannelPinWithRole(chIndex, IOR_Button_pd_n)) {
			channelsCount = i + 1;
			if(CHANNEL_Get(chIndex)) {
				BIT_SET(channelValues ,i);
			}
		} 
	}
	if(channelsCount > 0 && groupName[0]){
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
	addLogAdv(LOG_INFO, LOG_FEATURE_DGR,"DGR_SendBrightness: sent message to group %s",groupName);

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
	addLogAdv(LOG_INFO, LOG_FEATURE_DGR,"DGR_SendRGBCW: sent message to group %s",groupName);

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
	addLogAdv(LOG_INFO, LOG_FEATURE_DGR, "CMD_DGR_SendFixedColor: sent message to group %s", groupName);

	return CMD_RES_OK;
}

static commandResult_t CMD_DGR_DevGroupSend(const void *context, const char *cmd, const char *args, int flags) {
	byte message[MAX_DGR_PACKET];
	dgrDevice_t def;
	int len;
	int groupIndex = (int)(intptr_t)context;
	const char *groupName = CFG_DeviceGroups_GetNameByIndex(groupIndex);
	if (!groupName || !groupName[0] || !args || !args[0]) return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	DGR_SelectGroup(groupIndex);
	/* Tasmota reserves "DevGroupSend 1" for publishing every current item. */
	if (strcmp(args, "1") == 0) {
		g_dgr_no_status_share = 0;
		DRV_DGR_SendFullStatus(groupName);
		return CMD_RES_OK;
	}
	len = DGR_Quick_FormatCommand(message, sizeof(message), groupName, g_dgr_send_seq, groupIndex, args);
	if (len <= 0) return CMD_RES_BAD_ARGUMENT;
	if (!DGR_AddToSendQueueInternal(message, len, 0)) return CMD_RES_ERROR;
	DGR_RecordReliableMessage(message, len, g_dgr_send_seq);
	g_dgr_send_seq++;
	if (!g_dgr_send_seq) g_dgr_send_seq = 1;

	/* Tasmota's DevGroupSend applies the update locally as well. */
	memset(&def, 0, sizeof(def));
	strcpy_safe(def.gr.groupName, groupName, sizeof(def.gr.groupName));
	def.gr.devGroupShare_In = 0xFFFFFFFF;
	def.gr.noStatusShare = &g_dgr_no_status_share;
	def.gr.local = true;
	def.gr.stateIndex = groupIndex;
#if ENABLE_LED_BASIC
	def.cbs.processBrightnessPowerOn = DRV_DGR_processBrightnessPowerOn;
	def.cbs.processLightBrightness = DRV_DGR_processLightBrightness;
	def.cbs.processLightFixedColor = DRV_DGR_processLightFixedColor;
	def.cbs.processRGBCW = DRV_DGR_processRGBCW;
#endif
	def.cbs.processPower = DRV_DGR_processPower;
	def.cbs.processEvent = DRV_DGR_ProcessEvent;
	def.cbs.processCommand = DRV_DGR_ProcessTextCommand;
	g_inCmdProcessing = 1;
	DGR_Parse(message, len, &def, (struct sockaddr *)&g_mySockAddr);
	g_inCmdProcessing = 0;
	return CMD_RES_OK;
}

static commandResult_t CMD_DGR_DevGroupName(const void *context, const char *cmd, const char *args, int flags) {
	int groupIndex = (int)(intptr_t)context;
	if (args && args[0]) {
		CFG_DeviceGroups_SetNameByIndex(groupIndex, (strcmp(args, "0") == 0 || strcmp(args, "\"") == 0) ? "" : args);
		CFG_Save_IfThereArePendingChanges();
		DGR_ResetRuntime(groupIndex);
		DGR_SelectGroup(groupIndex);
		if (CFG_DeviceGroups_GetNameByIndex(groupIndex)[0]
			&& g_dgr_socket_receive > 0 && g_dgr_socket_send > 0) DGR_StartDiscovery();
	}
	addLogAdv(LOG_INFO, LOG_FEATURE_DGR, "DevGroupName%u %s", groupIndex + 1, CFG_DeviceGroups_GetNameByIndex(groupIndex));
	return CMD_RES_OK;
}

static commandResult_t CMD_DGR_DevGroupTie(const void *context, const char *cmd, const char *args, int flags) {
	int groupIndex = (int)(intptr_t)context;
	if (args && args[0]) {
		int relay = strtol(args, 0, 0);
		if (relay < 0 || relay > 24) return CMD_RES_BAD_ARGUMENT;
		CFG_DeviceGroups_SetTie(groupIndex, relay);
		CFG_Save_IfThereArePendingChanges();
	}
	addLogAdv(LOG_INFO, LOG_FEATURE_DGR, "DevGroupTie%u %u", groupIndex + 1, CFG_DeviceGroups_GetTie(groupIndex));
	return CMD_RES_OK;
}

static commandResult_t CMD_DGR_DevGroupShare(const void *context, const char *cmd, const char *args, int flags) {
	char *end;
	uint32_t shareIn;
	uint32_t shareOut;
	if (args && args[0]) {
		shareIn = strtoul(args, &end, 0);
		while (*end == ' ' || *end == ',') end++;
		shareOut = *end ? strtoul(end, 0, 0) : shareIn;
		CFG_DeviceGroups_SetRecvFlags(shareIn);
		CFG_DeviceGroups_SetSendFlags(shareOut);
		CFG_Save_IfThereArePendingChanges();
	}
	addLogAdv(LOG_INFO, LOG_FEATURE_DGR, "DevGroupShare In=%08X Out=%08X",
		CFG_DeviceGroups_GetRecvFlags(), CFG_DeviceGroups_GetSendFlags());
	return CMD_RES_OK;
}

static commandResult_t CMD_DGR_DevGroupStatus(const void *context, const char *cmd, const char *args, int flags) {
	int i;
	int groupIndex = (int)(intptr_t)context;
	DGR_SelectGroup(groupIndex);
	addLogAdv(LOG_INFO, LOG_FEATURE_DGR, "DevGroupStatus Index=%u GroupName=%s MessageSeq=%u MemberCount=%i",
		groupIndex + 1, CFG_DeviceGroups_GetNameByIndex(groupIndex), g_dgr_send_seq, g_curDGRMembers);
	for (i = 0; i < g_curDGRMembers; i++) {
		struct in_addr memberAddress;
		memberAddress.s_addr = g_dgrMembers[i].ip;
		addLogAdv(LOG_INFO, LOG_FEATURE_DGR, "Member %s ResendCount=%u LastRcvdSeq=%u LastAckedSeq=%u",
			inet_ntoa(memberAddress), g_dgrMembers[i].unicast_count,
			g_dgrMembers[i].lastSeq, g_dgrMembers[i].acked_sequence);
	}
	return CMD_RES_OK;
}

static void DGR_RegisterIndexedCommands(const char *const names[CFG_DEVICE_GROUP_MAX], commandHandler_t handler) {
	int i;
	for (i = 0; i < CFG_DEVICE_GROUP_MAX; i++) {
		CMD_RegisterCommand(names[i], handler, (void *)(intptr_t)i);
	}
}

void DRV_DGR_Init()
{
	static const char *const sendCommands[CFG_DEVICE_GROUP_MAX] = {
		"DevGroupSend1", "DevGroupSend2", "DevGroupSend3", "DevGroupSend4"
	};
	static const char *const nameCommands[CFG_DEVICE_GROUP_MAX] = {
		"DevGroupName1", "DevGroupName2", "DevGroupName3", "DevGroupName4"
	};
	static const char *const statusCommands[CFG_DEVICE_GROUP_MAX] = {
		"DevGroupStatus1", "DevGroupStatus2", "DevGroupStatus3", "DevGroupStatus4"
	};
	static const char *const tieCommands[CFG_DEVICE_GROUP_MAX] = {
		"DevGroupTie1", "DevGroupTie2", "DevGroupTie3", "DevGroupTie4"
	};
	int groupIndex;
	DGR_ResetAllRuntimes();

	DRV_DGR_CreateSocket_Receive();
	DRV_DGR_CreateSocket_Send();

	// Start initial discovery if sockets were created successfully
	if (g_dgr_socket_receive > 0 && g_dgr_socket_send > 0) {
		for (groupIndex = 0; groupIndex < CFG_DEVICE_GROUP_MAX; groupIndex++) {
			const char *groupName = CFG_DeviceGroups_GetNameByIndex(groupIndex);
			if (groupName[0]) {
				DGR_SelectGroup(groupIndex);
				DGR_StartDiscovery();
			}
		}
	}

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
	CMD_RegisterCommand("DevGroupSend", CMD_DGR_DevGroupSend, (void *)(intptr_t)0);
	CMD_RegisterCommand("DevGroupName", CMD_DGR_DevGroupName, (void *)(intptr_t)0);
	CMD_RegisterCommand("DevGroupShare", CMD_DGR_DevGroupShare, NULL);
	CMD_RegisterCommand("DevGroupStatus", CMD_DGR_DevGroupStatus, (void *)(intptr_t)0);
	CMD_RegisterCommand("DevGroupTie", CMD_DGR_DevGroupTie, (void *)(intptr_t)0);
	DGR_RegisterIndexedCommands(sendCommands, CMD_DGR_DevGroupSend);
	DGR_RegisterIndexedCommands(nameCommands, CMD_DGR_DevGroupName);
	DGR_RegisterIndexedCommands(statusCommands, CMD_DGR_DevGroupStatus);
	DGR_RegisterIndexedCommands(tieCommands, CMD_DGR_DevGroupTie);
}
