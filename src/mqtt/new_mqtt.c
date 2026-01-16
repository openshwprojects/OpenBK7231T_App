
#include "../obk_config.h"

#if ENABLE_MQTT 

#include "new_mqtt.h"
#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
#include "../logging/logging.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../hal/hal_wifi.h"
#include "../driver/drv_public.h"
#include "../driver/drv_ntp.h"
#include "../driver/drv_deviceclock.h"
#include "../driver/drv_tuyaMCU.h"
#include "../hal/hal_ota.h"
#include <math.h>
#ifndef WINDOWS
#include <lwip/dns.h>
#endif

#define BUILD_AND_VERSION_FOR_MQTT "Open" PLATFORM_MCU_NAME " " USER_SW_VER " " __DATE__ " " __TIME__ 

#if MQTT_USE_TLS
#include "lwip/altcp_tls.h"
#include "lwip/apps/mqtt_priv.h"
#include "apps/altcp_tls/altcp_tls_mbedtls_structs.h"
#include "mbedtls/ssl.h"
struct altcp_tls_config {
	mbedtls_ssl_config conf;
	mbedtls_x509_crt* cert;
	mbedtls_pk_context* pkey;
	u8_t cert_count;
	u8_t cert_max;
	u8_t pkey_count;
	u8_t pkey_max;
	mbedtls_x509_crt* ca;
#if defined(MBEDTLS_SSL_CACHE_C) && ALTCP_MBEDTLS_USE_SESSION_CACHE
	struct mbedtls_ssl_cache_context cache;
#endif
#if defined(MBEDTLS_SSL_SESSION_TICKETS) && ALTCP_MBEDTLS_USE_SESSION_TICKETS
	mbedtls_ssl_ticket_context ticket_ctx;
#endif
};
#if ALTCP_MBEDTLS_DEBUG
	#include "mbedtls/ssl_internal.h"
	#include "mbedtls/debug.h"
	static int mbedtls_verify_cb(void* data, mbedtls_x509_crt* crt, int depth, uint32_t* flags);
	static void mbedtls_debug_cb(void* ctx, int level, const char* file, int line, const char* str);
	void mbedtls_dump_conf(mbedtls_ssl_config* conf, mbedtls_ssl_context* ssl);
#endif
#endif

#ifndef LWIP_MQTT_EXAMPLE_IPADDR_INIT
#if LWIP_IPV4
#define LWIP_MQTT_EXAMPLE_IPADDR_INIT = IPADDR4_INIT(PP_HTONL(IPADDR_LOOPBACK))
#else
#define LWIP_MQTT_EXAMPLE_IPADDR_INIT
#endif
#endif

#ifdef PLATFORM_BEKEN
#include <tcpip.h>
// from hal_main_bk7231.c
// triggers a one-shot timer to cause read.
extern void MQTT_TriggerRead();
#endif

// these won't exist except on Beken?
#ifndef LOCK_TCPIP_CORE
#define LOCK_TCPIP_CORE()
#endif

#ifndef UNLOCK_TCPIP_CORE
#define UNLOCK_TCPIP_CORE()
#endif

//
// Variables for periodical self state broadcast
//
// current time left (counting down)
static int g_secondsBeforeNextFullBroadcast = 30;
// constant value, how much interval between self state broadcast (enabled by flag)
// You can change it with command: mqtt_broadcastInterval 60
static int g_intervalBetweenMQTTBroadcasts = 60;
// While doing self state broadcast, it limits the number of publishes 
// per second in order not to overload LWIP
static int g_maxBroadcastItemsPublishedPerSecond = 1;
// interval for automatic publish of tasmota tele/sensor and tele/state
static short g_teleState_interval = 120;
static short g_teleSensor_interval = 3;

/////////////////////////////////////////////////////////////
// mqtt receive buffer, so we can action in our threads, not
// in tcp_thread
//
#define MQTT_RX_BUFFER_MAX 4096
unsigned char mqtt_rx_buffer[MQTT_RX_BUFFER_MAX];
int mqtt_rx_buffer_head;
int mqtt_rx_buffer_tail;
int mqtt_rx_buffer_count;
unsigned char temp_topic[128];
unsigned char temp_data[2048];

int addLenData(int len, const unsigned char *data){
	mqtt_rx_buffer[mqtt_rx_buffer_head] = (len >> 8) & 0xff;
	mqtt_rx_buffer_head = (mqtt_rx_buffer_head + 1) % MQTT_RX_BUFFER_MAX;
	mqtt_rx_buffer_count++;
	mqtt_rx_buffer[mqtt_rx_buffer_head] = (len) & 0xff;
	mqtt_rx_buffer_head = (mqtt_rx_buffer_head + 1) % MQTT_RX_BUFFER_MAX;
	mqtt_rx_buffer_count++;
	for (int i = 0; i < len; i++){
		mqtt_rx_buffer[mqtt_rx_buffer_head] = data[i];
		mqtt_rx_buffer_head = (mqtt_rx_buffer_head + 1) % MQTT_RX_BUFFER_MAX;
		mqtt_rx_buffer_count++;
	}
	return len + 2;
}

int getLenData(int *len, unsigned char *data, int maxlen){
	int l;
	l = mqtt_rx_buffer[mqtt_rx_buffer_tail];
	mqtt_rx_buffer_tail = (mqtt_rx_buffer_tail + 1) % MQTT_RX_BUFFER_MAX;
	mqtt_rx_buffer_count--;
	l = l<<8;
	l |= mqtt_rx_buffer[mqtt_rx_buffer_tail];
	mqtt_rx_buffer_tail = (mqtt_rx_buffer_tail + 1) % MQTT_RX_BUFFER_MAX;
	mqtt_rx_buffer_count--;

	for (int i = 0; i < l; i++){
		if (i < maxlen){
			data[i] = mqtt_rx_buffer[mqtt_rx_buffer_tail];
		}
		mqtt_rx_buffer_tail = (mqtt_rx_buffer_tail + 1) % MQTT_RX_BUFFER_MAX;
		mqtt_rx_buffer_count--;
	}
	if (mqtt_rx_buffer_count < 0){
		addLogAdv(LOG_ERROR, LOG_FEATURE_MQTT, "MQTT_rx buffer underflow!!!");
		mqtt_rx_buffer_count = 0;
		mqtt_rx_buffer_tail = mqtt_rx_buffer_head = 0;
	}

	if (l > maxlen){
		*len = maxlen;
	} else {
		*len = l;
	}
	return l + 2;
}

static SemaphoreHandle_t g_mutex = 0;

static bool MQTT_Mutex_Take(int del) {
	int taken;

	if (g_mutex == 0)
	{
		g_mutex = xSemaphoreCreateMutex();
	}
	taken = xSemaphoreTake(g_mutex, del);
	if (taken == pdTRUE) {
		return true;
	}
	return false;
}

static void MQTT_Mutex_Free()
{
	xSemaphoreGive(g_mutex);
}

// this is called from tcp_thread context to queue received mqtt,
// and then we'll retrieve them from our own thread for processing.
//
// NOTE: this function is now public, but only because my unit tests
// system can use it to spoof MQTT packets to check if MQTT commands
// are working...
int MQTT_Post_Received(const char *topic, int topiclen, const unsigned char *data, int datalen){
	MQTT_Mutex_Take(100);
	if ((MQTT_RX_BUFFER_MAX - 1 - mqtt_rx_buffer_count) < topiclen + datalen + 2 + 2){
		addLogAdv(LOG_ERROR, LOG_FEATURE_MQTT, "MQTT_rx buffer overflow for topic %s", topic);
	} else {
		addLenData(topiclen, (unsigned char *)topic);
		addLenData(datalen, data);
	}
	MQTT_Mutex_Free();


#ifdef PLATFORM_BEKEN
	MQTT_TriggerRead();
#endif
	return 1;
}
int MQTT_Post_Received_Str(const char *topic, const char *data) {
	return MQTT_Post_Received(topic, strlen(topic), (const unsigned char*)data, strlen(data));
}
int get_received(char **topic, int *topiclen, unsigned char **data, int *datalen){
	int res = 0;
	MQTT_Mutex_Take(100);
	if (mqtt_rx_buffer_tail != mqtt_rx_buffer_head){
		getLenData(topiclen, temp_topic, sizeof(temp_topic)-1);
		temp_topic[*topiclen] = 0;
		getLenData(datalen, temp_data, sizeof(temp_data)-1);
		temp_data[*datalen] = 0;
		*topic = (char *)temp_topic;
		*data = temp_data;
		res = 1;
	}
	MQTT_Mutex_Free();
	return res;
}
//
//////////////////////////////////////////////////////////////////////

#define MQTT_QUEUE_ITEM_IS_REUSABLE(x)  (x->topic[0] == 0)
#define MQTT_QUEUE_ITEM_SET_REUSABLE(x) (x->topic[0] = 0)

MqttPublishItem_t* g_MqttPublishQueueHead = NULL;
int g_MqttPublishItemsQueued = 0;   //Items in the queue waiting to be published. This is not the queue length.

// from mqtt.c
extern void mqtt_disconnect(mqtt_client_t* client);

static int g_my_reconnect_mqtt_after_time = -1;
ip_addr_t mqtt_ip LWIP_MQTT_EXAMPLE_IPADDR_INIT;
mqtt_client_t* mqtt_client;
static int g_timeSinceLastMQTTPublish = 0;
static int mqtt_initialised = 0;
static int mqtt_connect_events = 0;
static int mqtt_connect_result = ERR_OK;
static char mqtt_status_message[256];
static int mqtt_published_events = 0;
static int mqtt_publish_errors = 0;
static int mqtt_received_events = 0;

static int g_just_connected = 0;


typedef struct mqtt_callback_tag {
	char* topic;
	char* subscriptionTopic;
	int ID;
	mqtt_callback_fn callback;
} mqtt_callback_t;

#define MAX_MQTT_CALLBACKS 32
static mqtt_callback_t* callbacks[MAX_MQTT_CALLBACKS];
static int numCallbacks = 0;
// note: only one incomming can be processed at a time.
static obk_mqtt_request_t g_mqtt_request;
static obk_mqtt_request_t g_mqtt_request_cb;

#define LOOPS_WITH_DISCONNECTED 15
int mqtt_loopsWithDisconnected = 0;
int mqtt_reconnect = 0;
// set for the device to broadcast self state on start
int g_bPublishAllStatesNow = 0;
int g_publishItemIndex = PUBLISHITEM_ALL_INDEX_FIRST;
//static bool g_firstFullBroadcast = true;  //Flag indicating that we need to do a full broadcast

int g_memoryErrorsThisSession = 0;
int g_mqtt_bBaseTopicDirty = 0;

void MQTT_PublishWholeDeviceState_Internal(bool bAll)
{
	g_bPublishAllStatesNow = 1;
	if (bAll) {
		g_publishItemIndex = PUBLISHITEM_ALL_INDEX_FIRST;
	}
	else {
		g_publishItemIndex = PUBLISHITEM_DYNAMIC_INDEX_FIRST;
	}
}

void MQTT_PublishWholeDeviceState()
{
	//Publish all status items once. Publish only dynamic items after that.
	//MQTT_PublishWholeDeviceState_Internal(g_firstFullBroadcast);
	MQTT_PublishWholeDeviceState_Internal(1);
}

void MQTT_PublishOnlyDeviceChannelsIfPossible()
{
	if (g_bPublishAllStatesNow == 1)
		return;
	g_bPublishAllStatesNow = 1;
	//Start with light channels
	g_publishItemIndex = PUBLISHITEM_SELF_DYNAMIC_LIGHTSTATE;
}

static struct mqtt_connect_client_info_t mqtt_client_info =
{
  "test",
  // do not fil those settings, they are overriden when read from memory
  "user", /* user */
  "pass", /* pass */
  100,  /* keep alive */
  NULL, /* will_topic */
  NULL, /* will_msg */
  1,    /* will_qos */
  0     /* will_retain */
#if LWIP_ALTCP && LWIP_ALTCP_TLS
  , NULL
#endif
};

// channel set callback
int channelSet(obk_mqtt_request_t* request);
int channelGet(obk_mqtt_request_t* request);
static int MQTT_do_connect(mqtt_client_t* client);
static void mqtt_connection_cb(mqtt_client_t* client, void* arg, mqtt_connection_status_t status);

int MQTT_GetConnectEvents(void)
{
	return mqtt_connect_events;
}

int MQTT_GetPublishEventCounter(void)
{
	return mqtt_published_events;
}

int MQTT_GetPublishErrorCounter(void)
{
	return mqtt_publish_errors;
}

int MQTT_GetReceivedEventCounter(void)
{
	return mqtt_received_events;
}

int MQTT_GetConnectResult(void)
{
	return mqtt_connect_result;
}

//Based on mqtt_connection_status_t and https://www.nongnu.org/lwip/2_1_x/group__mqtt.html
const char* get_callback_error(int reason) {
	switch (reason)
	{
	case MQTT_CONNECT_REFUSED_PROTOCOL_VERSION: return "Refused protocol version";
	case MQTT_CONNECT_REFUSED_IDENTIFIER: return "Refused identifier";
	case MQTT_CONNECT_REFUSED_SERVER: return "Refused server";
	case MQTT_CONNECT_REFUSED_USERNAME_PASS: return "Refused user credentials";
	case MQTT_CONNECT_REFUSED_NOT_AUTHORIZED_: return "Refused not authorized";
	case MQTT_CONNECT_DISCONNECTED: return "Disconnected";
	case MQTT_CONNECT_TIMEOUT: return "Timeout";
	}
	return "";
}

const char* get_error_name(int err)
{
	switch (err)
	{
	case ERR_OK: return "ERR_OK";
	case ERR_MEM: return "ERR_MEM";
		/** Buffer error.            */
	case ERR_BUF: return "ERR_BUF";
		/** Timeout.                 */
	case ERR_TIMEOUT: return "ERR_TIMEOUT";
		/** Routing problem.         */
	case ERR_RTE: return "ERR_RTE";
		/** Operation in progress    */
	case ERR_INPROGRESS: return "ERR_INPROGRESS";
		/** Illegal value.           */
	case ERR_VAL: return "ERR_VAL";
		/** Operation would block.   */
	case ERR_WOULDBLOCK: return "ERR_WOULDBLOCK";
		/** Address in use.          */
	case ERR_USE: return "ERR_USE";
#if defined(ERR_ALREADY)
		/** Already connecting.      */
	case ERR_ALREADY: return "ERR_ALREADY";
#endif
		/** Conn already established.*/
	case ERR_ISCONN: return "ERR_ISCONN";
		/** Not connected.           */
	case ERR_CONN: return "ERR_CONN";
		/** Low-level netif error    */
	case ERR_IF: return "ERR_IF";
		/** Connection aborted.      */
	case ERR_ABRT: return "ERR_ABRT";
		/** Connection reset.        */
	case ERR_RST: return "ERR_RST";
		/** Connection closed.       */
	case ERR_CLSD: return "ERR_CLSD";
		/** Illegal argument.        */
	case ERR_ARG: return "ERR_ARG";
	}
	return "";
}

char* MQTT_GetStatusMessage(void)
{
	return mqtt_status_message;
}

void MQTT_ClearCallbacks() {
	int i;
	for (i = 0; i < MAX_MQTT_CALLBACKS; i++) {
		if (callbacks[i]) {
			free(callbacks[i]->topic);
			free(callbacks[i]->subscriptionTopic);
			free(callbacks[i]);
			callbacks[i] = 0;
		}
	}
}
// this can REPLACE callbacks, since we MAY wish to change the root topic....
// in which case we would re-resigster all callbacks?
int MQTT_RegisterCallback(const char* basetopic, const char* subscriptiontopic, int ID, mqtt_callback_fn callback) {
	int index;
	int i;
	int subscribechange = 0;
	if (!basetopic || !subscriptiontopic || !callback) {
		return -1;
	}
	addLogAdv(LOG_INFO, LOG_FEATURE_MQTT, "MQTT_RegisterCallback called for bT %s subT %s", basetopic, subscriptiontopic);

	// find existing to replace
	for (index = 0; index < numCallbacks; index++) {
		if (callbacks[index]) {
			if (callbacks[index]->ID == ID) {
				break;
			}
		}
	}

	// find empty if any (empty by MQTT_RemoveCallback)
	if (index == numCallbacks) {
		for (index = 0; index < numCallbacks; index++) {
			if (!callbacks[index]) {
				break;
			}
		}
	}

	if (index >= MAX_MQTT_CALLBACKS) {
		return -4;
	}
	if (!callbacks[index]) {
		callbacks[index] = (mqtt_callback_t*)os_malloc(sizeof(mqtt_callback_t));
		if (callbacks[index] != 0) {
			memset(callbacks[index], 0, sizeof(mqtt_callback_t));
		}
	}
	if (!callbacks[index]) {
		return -2;
	}
	if (!callbacks[index]->topic || strcmp(callbacks[index]->topic, basetopic)) {
		if (callbacks[index]->topic) {
			os_free(callbacks[index]->topic);
		}
		callbacks[index]->topic = (char*)os_malloc(strlen(basetopic) + 1);
		if (!callbacks[index]->topic) {
			os_free(callbacks[index]);
			return -3;
		}
		strcpy(callbacks[index]->topic, basetopic);
	}

	if (!callbacks[index]->subscriptionTopic || strcmp(callbacks[index]->subscriptionTopic, subscriptiontopic)) {
		if (callbacks[index]->subscriptionTopic) {
			os_free(callbacks[index]->subscriptionTopic);
		}
		callbacks[index]->subscriptionTopic = (char*)os_malloc(strlen(subscriptiontopic) + 1);
		callbacks[index]->subscriptionTopic[0] = '\0';
		if (!callbacks[index]->subscriptionTopic) {
			os_free(callbacks[index]->topic);
			os_free(callbacks[index]);
			return -3;
		}

		// find out if this subscription is new.
		for (i = 0; i < numCallbacks; i++) {
			if (callbacks[i]) {
				if (callbacks[i]->subscriptionTopic &&
					!strcmp(callbacks[i]->subscriptionTopic, subscriptiontopic)) {
					break;
				}
			}
		}
		strcpy(callbacks[index]->subscriptionTopic, subscriptiontopic);
		// if this subscription is new, must reconnect
		if (i == numCallbacks) {
			subscribechange++;
		}
	}

	callbacks[index]->callback = callback;
	if (index == numCallbacks) {
		numCallbacks++;
	}

	if (subscribechange) {
		if (mqtt_client) {
			mqtt_reconnect = 8;
		}
	}
	// success
	return 0;
}

int MQTT_RemoveCallback(int ID) {
	int index;

	for (index = 0; index < numCallbacks; index++) {
		if (callbacks[index]) {
			if (callbacks[index]->ID == ID) {
				if (callbacks[index]->topic) {
					os_free(callbacks[index]->topic);
					callbacks[index]->topic = NULL;
				}
				if (callbacks[index]->subscriptionTopic) {
					os_free(callbacks[index]->subscriptionTopic);
					callbacks[index]->subscriptionTopic = NULL;
				}
				os_free(callbacks[index]);
				callbacks[index] = NULL;
				if (mqtt_client) {
					mqtt_reconnect = 8;
				}
				return 1;
			}
		}
	}
	return 0;
}

const char *skipExpected(const char *p, const char *tok) {
	while (1) {
		if (*p == 0)
			return 0;
		if (*p != *tok)
			return 0;
		p++;
		tok++;
		if (*tok == 0) {
			if (*p == '/') {
				p++;
				return p;
			}
			return 0;
		}
	}
	return p;
}
/** From a MQTT topic formatted <client>/<topic>, check if <client> is present
 *  and return <topic>.
 *
 *  @param topic	The topic to parse
 *  @return 		The topic without the client, or NULL if <client>/ wasn't present
 */
const char* MQTT_RemoveClientFromTopic(const char* topic, const char *prefix) {
	const char *p2;
	const char *p = topic;
	if (prefix) {
		p = skipExpected(p, prefix);
		if (p == 0) {
			return 0;
		}
	}
	// it is either group topic or a device topic
	p2 = skipExpected(p, CFG_GetMQTTClientId());
	if (p2 == 0) {
		p2 = skipExpected(p, CFG_GetMQTTGroupTopic());
	}
	return p2;
}
bool stribegins(const char *str, const char *needle) {
	int l = strlen(needle);
	return !wal_strnicmp(str, needle, l);
}
// this accepts obkXXXXXX/<chan>/get to request channel publish
int channelGet(obk_mqtt_request_t* request) {
	//int len = request->receivedLen;
	int channel = 0;
	const char* p;

	// we only support here publishes with emtpy value, otherwise we would get into
	// a loop where we receive a get, and then send get reply with val, and receive our own get
	if (request->receivedLen) {
		return 1;
	}

	addLogAdv(LOG_DEBUG, LOG_FEATURE_MQTT, "channelGet topic %i with arg %s", request->topic, request->received);

	p = MQTT_RemoveClientFromTopic(request->topic,0);

	if (p == NULL) {
		return 0;
	}

	addLogAdv(LOG_INFO, LOG_FEATURE_MQTT, "channelGet part topic %s", p);
#if ENABLE_LED_BASIC
	if (stribegins(p, "led_enableAll")) {
		LED_SendEnableAllState();
		return 1;
	}
	if (stribegins(p, "led_dimmer")) {
		LED_SendDimmerChange();
		return 1;
	}
	if (stribegins(p, "led_temperature")) {
		sendTemperatureChange();
		return 1;
	}
	if (stribegins(p, "led_finalcolor_rgb")) {
		sendFinalColor();
		return 1;
	}
	if (stribegins(p, "led_basecolor_rgb")) {
		sendColorChange();
		return 1;
	}
#endif

	// atoi won't parse any non-decimal chars, so it should skip over the rest of the topic.
	channel = atoi(p);

	//addLogAdv(LOG_INFO, LOG_FEATURE_MQTT, "channelGet channel %i", channel);

	// if channel out of range, stop here.
	if ((channel < 0) || (channel > 32)) {
		return 0;
	}

	MQTT_ChannelPublish(channel, 0);

	// return 1 to stop processing callbacks here.
	// return 0 to allow later callbacks to process this topic.
	return 1;
}
// this accepts obkXXXXXX/<chan>/set to receive data to set channels
int channelSet(obk_mqtt_request_t* request) {
	//int len = request->receivedLen;
	int channel = 0;
	int iValue = 0;
	const char* p;
	const char *argument;

	addLogAdv(LOG_DEBUG, LOG_FEATURE_MQTT, "channelSet topic %i with arg %s", request->topic, request->received);

	p = MQTT_RemoveClientFromTopic(request->topic,0);

	if (p == NULL) {
		return 0;
	}

	//addLogAdv(LOG_INFO, LOG_FEATURE_MQTT, "channelSet part topic %s", p);

	// atoi won't parse any non-decimal chars, so it should skip over the rest of the topic.
	channel = atoi(p);

	//addLogAdv(LOG_INFO, LOG_FEATURE_MQTT, "channelSet channel %i", channel);

	// if channel out of range, stop here.
	if ((channel < 0) || (channel > CHANNEL_MAX)) {
		return 0;
	}

	// make sure the topic ends with '/set'.
	p = strchr(p, '/');

	// if not /set, then stop here
	if (strcmp(p, "/set")) {
		//addLogAdv(LOG_INFO, LOG_FEATURE_MQTT, "channelSet NOT 'set'");
		return 0;
	}

	addLogAdv(LOG_INFO, LOG_FEATURE_MQTT, "MQTT client in mqtt_incoming_data_cb data is %.*s for ch %i\n", MQTT_MAX_DATA_LOG_LENGTH, request->received, channel);

	argument = ((const char*)request->received);

	if (!wal_strnicmp(argument, "toggle", 6)) {
		CHANNEL_Toggle(channel);
	}
	else {
		iValue = atoi(argument);
		CHANNEL_Set(channel, iValue, 0);
	}

	// return 1 to stop processing callbacks here.
	// return 0 to allow later callbacks to process this topic.
	return 1;
}


// this accepts cmnd/<clientId>/<xxx> to execute any supported console command
// Example 1: 
// Topic: cmnd/obk8C112233/power
// Payload: toggle
// this will toggle power
// Example 2:
// Topic: cmnd/obk8C112233/backlog
// Payload: echo Test1; power toggle; echo Test2
// will do echo, toggle power and do ecoh
//


void MQTT_PublishPrinterContentsToStat(obk_mqtt_publishReplyPrinter_t *printer, const char *statName) {
	const char *toUse;
	if (printer->allocated)
		toUse = printer->allocated;
	else
		toUse = printer->stackBuffer;
	MQTT_PublishStat(statName, toUse);
}
void MQTT_PublishPrinterContentsToTele(obk_mqtt_publishReplyPrinter_t *printer, const char *statName) {
	const char *toUse;
	if (printer->allocated)
		toUse = printer->allocated;
	else
		toUse = printer->stackBuffer;
	MQTT_PublishTele(statName, toUse);
}
int mqtt_printf255(obk_mqtt_publishReplyPrinter_t* request, const char* fmt, ...) {
	va_list argList;
	char tmp[256];
	int myLen;

	memset(tmp, 0, sizeof(tmp));
	va_start(argList, fmt);
	vsnprintf(tmp, 255, fmt, argList);
	va_end(argList);

	myLen = strlen(tmp);

	if (request->curLen + (myLen + 2) >= MQTT_STACK_BUFFER_SIZE) {
		if (request->curLen + (myLen + 2) >= MQTT_TOTAL_BUFFER_SIZE) {
			// TODO: realloc
			return 0;
		}
		// init alloced if needed
		if (request->allocated == 0) {
			request->allocated = malloc(MQTT_TOTAL_BUFFER_SIZE);
			strcpy(request->allocated, request->stackBuffer);
		}
		strcat(request->allocated, tmp);
	}
	else {
		strcat(request->stackBuffer, tmp);
	}
	request->curLen += myLen;
	return 0;
}
#if ENABLE_TASMOTA_JSON
void MQTT_ProcessCommandReplyJSON(const char *cmd, const char *args, int flags) {
	obk_mqtt_publishReplyPrinter_t replyBuilder;
	memset(&replyBuilder, 0, sizeof(obk_mqtt_publishReplyPrinter_t));
	JSON_ProcessCommandReply(cmd, args, &replyBuilder, (jsonCb_t)mqtt_printf255, flags);
	if (replyBuilder.allocated != 0) {
		free(replyBuilder.allocated);
	}
}
#endif

int onHassStatus(obk_mqtt_request_t* request) {
	if (!strcmp(request->topic, "homeassistant/status")) {
		const char *args = (const char *)request->received;
		addLogAdv(LOG_INFO, LOG_FEATURE_MQTT, "HA status - %s\n", args);
		if (!strcmp(args, "online")) {
			MQTT_PublishWholeDeviceState_Internal(true);
		}
	}
	return 1;
}
int tasCmnd(obk_mqtt_request_t* request) {
	const char *p, *args;
    //const char *p2;

	p = MQTT_RemoveClientFromTopic(request->topic, "cmnd");
	if (p == 0) {
		p = MQTT_RemoveClientFromTopic(request->topic, "tele");
		if (p == 0) {
			p = MQTT_RemoveClientFromTopic(request->topic, "stat");
			if (p == 0) {

			}
		}
	}
	if (p == 0)
		return 1;

#if 1
	args = (const char *)request->received;
	// I think that our function get_received always ensured that
	// there is a NULL terminating character after payload of MQTT
	// So we can feed it directly as command
	CMD_ExecuteCommandArgs(p, args, COMMAND_FLAG_SOURCE_MQTT);
#if ENABLE_TASMOTA_JSON
	MQTT_ProcessCommandReplyJSON(p, args, COMMAND_FLAG_SOURCE_MQTT);
#endif
#else
	int len = request->receivedLen;
	char copy[64];
	char *allocated;
	// assume a string input here, copy and terminate
	// Try to avoid free/malloc
	if (len > sizeof(copy) - 2) {
		allocated = (char*)malloc(len + 1);
		if (allocated) {
			strncpy(allocated, (char*)request->received, len);
			// strncpy does not terminate??!!!!
			allocated[len] = '\0';
		}
		// use command executor....
		CMD_ExecuteCommandArgs(p, allocated, COMMAND_FLAG_SOURCE_MQTT);
		if (allocated) {
			free(allocated);
		}
	}
	else {
		strncpy(copy, (char*)request->received, len);
		// strncpy does not terminate??!!!!
		copy[len] = '\0';
		// use command executor....
		CMD_ExecuteCommandArgs(p, copy, COMMAND_FLAG_SOURCE_MQTT);
	}
#endif
	// return 1 to stop processing callbacks here.
	// return 0 to allow later callbacks to process this topic.
	return 1;
}

//void MQTT_GetStats(int *outUsed, int *outMax, int *outFreeMem) {
//  mqtt_get_request_stats(mqtt_client,outUsed, outMax,outFreeMem);
//}

// copied here because for some reason renames in sdk?
static void MQTT_disconnect(mqtt_client_t* client)
{
	if (!client)
		return;
	// this is what it was renamed to.  why?
	LOCK_TCPIP_CORE();
	mqtt_disconnect(client);
	UNLOCK_TCPIP_CORE();

}

/* Called when publish is complete either with sucess or failure */
static void mqtt_pub_request_cb(void* arg, err_t result)
{
	if (result != ERR_OK)
	{
		addLogAdv(LOG_INFO, LOG_FEATURE_MQTT, "Publish result: %d(%s)\n", result, get_error_name(result));
		mqtt_publish_errors++;
	}
}

// This publishes value to the specified topic/channel.
static OBK_Publish_Result MQTT_PublishTopicToClient(mqtt_client_t* client, const char* sTopic, const char* sChannel, const char* sVal, int flags, bool appendGet)
{
	err_t err;
	u8_t qos = 1; /* 0 1 or 2, see MQTT specification */
	if (flags & OBK_PUBLISH_FLAG_QOS_ZERO)
	{
		qos = 0;
	}
	u8_t retain = 0; /* No don't retain such crappy payload... */
	size_t sVal_len;
	char* pub_topic;

	if (client == 0)
		return OBK_PUBLISH_WAS_DISCONNECTED;

	if (flags & OBK_PUBLISH_FLAG_MUTEX_SILENT)
	{
		if (MQTT_Mutex_Take(100) == 0)
		{
			return OBK_PUBLISH_MUTEX_FAIL;
		}
	}
	else {
		if (MQTT_Mutex_Take(500) == 0)
		{
			addLogAdv(LOG_ERROR, LOG_FEATURE_MQTT, "MQTT_PublishTopicToClient: mutex failed for %s=%s\r\n", sChannel, sVal);
			return OBK_PUBLISH_MUTEX_FAIL;
		}
	}
	if (flags & OBK_PUBLISH_FLAG_RETAIN)
	{
		retain = 1;
	}
	// global tool
	if (CFG_HasFlag(OBK_FLAG_MQTT_ALWAYSSETRETAIN))
	{
		retain = 1;
	}
	if (flags & OBK_PUBLISH_FLAG_FORCE_REMOVE_GET)
	{
		appendGet = false;
	}
	if (CFG_HasFlag(OBK_FLAG_MQTT_NEVERAPPENDGET))
	{
		appendGet = false;
	}


	LOCK_TCPIP_CORE();
	int res = mqtt_client_is_connected(client);
	UNLOCK_TCPIP_CORE();

	if (res == 0)
	{
		g_my_reconnect_mqtt_after_time = 5;
		MQTT_Mutex_Free();
		return OBK_PUBLISH_WAS_DISCONNECTED;
	}

	g_timeSinceLastMQTTPublish = 0;

	pub_topic = (char*)os_malloc(strlen(sTopic) + 1 + strlen(sChannel) + 5 + 1); //5 for /get
	if ((pub_topic != NULL) && (sVal != NULL))
	{
		sVal_len = strlen(sVal);
		if (flags & OBK_PUBLISH_FLAG_RAW_TOPIC_NAME)
		{
			strcpy(pub_topic, sChannel);
		}
		else 
		{
			sprintf(pub_topic, "%s/%s%s", sTopic, sChannel, (appendGet == true ? "/get" : ""));
		}
		if (sVal_len < 128)
		{
			addLogAdv(LOG_INFO, LOG_FEATURE_MQTT, "Publishing val %s to %s retain=%i\n", sVal, pub_topic, retain);
		}
		else {
			addLogAdv(LOG_INFO, LOG_FEATURE_MQTT, "Publishing val (%d bytes) to %s retain=%i\n", sVal_len, pub_topic, retain);
		}


		LOCK_TCPIP_CORE();
		err = mqtt_publish(client, pub_topic, sVal, strlen(sVal), qos, retain, mqtt_pub_request_cb, 0);
		UNLOCK_TCPIP_CORE();
		os_free(pub_topic);

		if (err != ERR_OK)
		{
			if (err == ERR_CONN)
			{
				addLogAdv(LOG_ERROR, LOG_FEATURE_MQTT, "Publish err: ERR_CONN aka %d\n", err);
			}
			else if (err == ERR_MEM) {
				addLogAdv(LOG_ERROR, LOG_FEATURE_MQTT, "Publish err: ERR_MEM aka %d\n", err);
				g_memoryErrorsThisSession++;
			}
			else {
				addLogAdv(LOG_ERROR, LOG_FEATURE_MQTT, "Publish err: %d\n", err);
			}
			mqtt_publish_errors++;
			MQTT_Mutex_Free();
			return OBK_PUBLISH_MEM_FAIL;
		}
		mqtt_published_events++;
		MQTT_Mutex_Free();
		return OBK_PUBLISH_OK;
	}
	else {
		MQTT_Mutex_Free();
		return OBK_PUBLISH_MEM_FAIL;
	}
}

// This is used to publish channel values in "obk0696FB33/1/get" format with numerical value,
// This is also used to publish custom information with string name,
// for example, "obk0696FB33/voltage/get" is used to publish voltage from the sensor
static OBK_Publish_Result MQTT_PublishMain(mqtt_client_t* client, const char* sChannel, const char* sVal, int flags, bool appendGet)
{
	return MQTT_PublishTopicToClient(mqtt_client, CFG_GetMQTTClientId(), sChannel, sVal, flags, appendGet);
}
OBK_Publish_Result MQTT_PublishTele(const char* teleName, const char* teleValue)
{
	char topic[64];
	snprintf(topic, sizeof(topic), "tele/%s", CFG_GetMQTTClientId());
	return MQTT_PublishTopicToClient(mqtt_client, topic, teleName, teleValue, 0, false);
}
OBK_Publish_Result MQTT_PublishStat(const char* statName, const char* statValue)
{
	char topic[64];
	snprintf(topic,sizeof(topic),"stat/%s", CFG_GetMQTTClientId());
	return MQTT_PublishTopicToClient(mqtt_client, topic, statName, statValue, 0, false);
}
/// @brief Publish a MQTT message immediately.
/// @param sTopic 
/// @param sChannel 
/// @param sVal 
/// @param flags
/// @return 
OBK_Publish_Result MQTT_Publish(const char* sTopic, const char* sChannel, const char* sVal, int flags)
{
	return MQTT_PublishTopicToClient(mqtt_client, sTopic, sChannel, sVal, flags, false);
}

void MQTT_OBK_Printf(char* s) {
	addLogAdv(LOG_INFO, LOG_FEATURE_MQTT, s);
}

////////////////////////////////////////
// called from tcp_thread context.
// we should do callbacks from one of our threads?
static void mqtt_incoming_data_cb(void* arg, const u8_t* data, u16_t len, u8_t flags)
{
	int i;
	// unused - left here as example
	//const struct mqtt_connect_client_info_t* client_info = (const struct mqtt_connect_client_info_t*)arg;

	// if we stored a topic in g_mqtt_request, then we found a matching callback, so use it.
	if (g_mqtt_request.topic[0])
	{
		// note: data is NOT terminated (it may be binary...).
		g_mqtt_request.received = data;
		g_mqtt_request.receivedLen = len;

		//addLogAdv(LOG_INFO, LOG_FEATURE_MQTT, "MQTT in topic %s", g_mqtt_request.topic);
		mqtt_received_events++;

		for (i = 0; i < numCallbacks; i++)
		{
			if (callbacks[i] == 0)
				continue;
			char* cbtopic = callbacks[i]->topic;
			if (!strncmp(g_mqtt_request.topic, cbtopic, strlen(cbtopic)))
			{
				MQTT_Post_Received(g_mqtt_request.topic, strlen(g_mqtt_request.topic), data, len);
				// if ANYONE is interested, store it.
				break;
				// note - callback must return 1 to say it ate the mqtt, else further processing can be performed.
				// i.e. multiple people can get each topic if required.
				//if (callbacks[i]->callback(&g_mqtt_request))
				//{
				//	return;
				//}
			}
		}
		//addLogAdv(LOG_INFO, LOG_FEATURE_MQTT, "MQTT topic not handled: %s", g_mqtt_request.topic);
	}
}


// run from userland (quicktick or wakeable thread)
int MQTT_process_received(){
	char *topic;
	int topiclen;
	unsigned char *data;
	int datalen;
	int found = 0;
	int count = 0;
	do{
		found = get_received(&topic, &topiclen, &data, &datalen);
		if (found){
			count++;
			strncpy(g_mqtt_request_cb.topic, topic, sizeof(g_mqtt_request_cb.topic));
			g_mqtt_request_cb.received = data;
			g_mqtt_request_cb.receivedLen = datalen;
			for (int i = 0; i < numCallbacks; i++)
			{
				char* cbtopic = callbacks[i]->topic;
				if (!strncmp(topic, cbtopic, strlen(cbtopic)))
				{
					// note - callback must return 1 to say it ate the mqtt, else further processing can be performed.
					// i.e. multiple people can get each topic if required.
					if (callbacks[i]->callback(&g_mqtt_request_cb))
					{
						// if no further processing, then break this loop.
						break;
					}
				}
			}
		}
	} while (found);

	return count;
}


////////////////////////////////
// called from tcp_thread context
static void mqtt_incoming_publish_cb(void* arg, const char* topic, u32_t tot_len)
{
	//const char *p;
	int i;
	// unused - left here as example
	//const struct mqtt_connect_client_info_t* client_info = (const struct mqtt_connect_client_info_t*)arg;

	// look for a callback with this URL and method, or HTTP_ANY
	g_mqtt_request.topic[0] = '\0';
	for (i = 0; i < numCallbacks; i++)
	{
		char* cbtopic = callbacks[i]->topic;
		if (strncmp(topic, cbtopic, strlen(cbtopic)))
		{
			strncpy(g_mqtt_request.topic, topic, sizeof(g_mqtt_request.topic) - 1);
			g_mqtt_request.topic[sizeof(g_mqtt_request.topic) - 1] = 0;
			break;
		}
	}
	addLogAdv(LOG_INFO, LOG_FEATURE_MQTT, "MQTT client in mqtt_incoming_publish_cb topic %s\n", topic);
}

static void mqtt_request_cb(void* arg, err_t err)
{
	const struct mqtt_connect_client_info_t* client_info = (const struct mqtt_connect_client_info_t*)arg;
	if (err != 0) {
		addLogAdv(LOG_ERROR, LOG_FEATURE_MQTT, "MQTT client \"%s\" request cb: err %d\n", client_info->client_id, (int)err);
	}
}

/////////////////////////////////////////////
// should be called in tcp_thread context.
static void mqtt_connection_cb(mqtt_client_t* client, void* arg, mqtt_connection_status_t status)
{
	int i;
	char tmp[CGF_MQTT_CLIENT_ID_SIZE + 16];
	const char* clientId;
	err_t err = ERR_OK;
	const struct mqtt_connect_client_info_t* client_info = (const struct mqtt_connect_client_info_t*)arg;
	LWIP_UNUSED_ARG(client);

	//   addLogAdv(LOG_INFO,LOG_FEATURE_MQTT,"MQTT client < removed name > connection cb: status %d\n",  (int)status);
	 //  addLogAdv(LOG_INFO,LOG_FEATURE_MQTT,"MQTT client \"%s\" connection cb: status %d\n", client_info->client_id, (int)status);

	if (status == MQTT_CONNECT_ACCEPTED)
	{
		addLogAdv(LOG_INFO, LOG_FEATURE_MQTT, "mqtt_connection_cb: Successfully connected\n");

#if LWIP_ALTCP_TLS_MBEDTLS
		if (CFG_GetMQTTUseTls() && client && client->conn && client->conn->state) {
			altcp_mbedtls_state_t* state = client->conn->state;
			mbedtls_ssl_context* ssl = &state->ssl_context;
			addLogAdv(LOG_INFO, LOG_FEATURE_MQTT, "MQTT TLS VERSION: %s\n", mbedtls_ssl_get_version(ssl));
			addLogAdv(LOG_INFO, LOG_FEATURE_MQTT, "MQTT TLS CIPHER : %s\n", mbedtls_ssl_get_ciphersuite(ssl));
		}
#endif

		//LOCK_TCPIP_CORE();
		mqtt_set_inpub_callback(mqtt_client,
			mqtt_incoming_publish_cb,
			mqtt_incoming_data_cb,
			LWIP_CONST_CAST(void*, &mqtt_client_info));
		//UNLOCK_TCPIP_CORE();

		// subscribe to all callback subscription topics
		// this makes a BIG assumption that we can subscribe multiple times to the same one?
		// TODO - check that subscribing multiple times to the same topic is not BAD
		for (i = 0; i < numCallbacks; i++) {
			if (callbacks[i]) {
				if (callbacks[i]->subscriptionTopic && callbacks[i]->subscriptionTopic[0]) {
					err = mqtt_sub_unsub(client,
						callbacks[i]->subscriptionTopic, 1,
						mqtt_request_cb, LWIP_CONST_CAST(void*, client_info),
						1);
					if (err != ERR_OK) {
						addLogAdv(LOG_INFO, LOG_FEATURE_MQTT, "mqtt_subscribe to %s return: %d\n", callbacks[i]->subscriptionTopic, err);
					}
					else {
						addLogAdv(LOG_INFO, LOG_FEATURE_MQTT, "mqtt_subscribed to %s\n", callbacks[i]->subscriptionTopic);
					}
				}
			}
		}

		clientId = CFG_GetMQTTClientId();

		snprintf(tmp, sizeof(tmp), "%s/connected", clientId);
		//LOCK_TCPIP_CORE();
		err = mqtt_publish(client, tmp, "online", strlen("online"), 2, true, mqtt_pub_request_cb, 0);
		//UNLOCK_TCPIP_CORE();
		if (err != ERR_OK) {
			addLogAdv(LOG_ERROR, LOG_FEATURE_MQTT, "Publish err: %d\n", err);
			if (err == ERR_CONN) {
				// g_my_reconnect_mqtt_after_time = 5;
			}
		}

		g_just_connected = 1;

		//mqtt_sub_unsub(client,
		//        "topic_qos1", 1,
		//        mqtt_request_cb, LWIP_CONST_CAST(void*, client_info),
		//        1);
		//mqtt_sub_unsub(client,
		//        "topic_qos0", 0,
		//        mqtt_request_cb, LWIP_CONST_CAST(void*, client_info),
		//        1);
	}
	else {
		addLogAdv(LOG_INFO, LOG_FEATURE_MQTT, "mqtt_connection_cb: Disconnected, reason: %d(%s)\n", status, get_callback_error(status));
	}
}

static ip_addr_t mqtt_ip_resolved;
static volatile int dns_in_progress_time;
static volatile bool dns_resolved;
void dnsFound(const char *name, ip_addr_t *ipaddr, void *arg) 
{       

	if (NULL != ipaddr)
	{
		memcpy(&mqtt_ip_resolved, ipaddr, sizeof(mqtt_ip_resolved));
		dns_resolved = true;
		/* Try to reconnect immediately after resolving the host */
		mqtt_loopsWithDisconnected = LOOPS_WITH_DISCONNECTED + 1;
		addLogAdv(LOG_INFO, LOG_FEATURE_MQTT, "mqtt_host %s resolution SUCCESS\r\n", name);
	}
	else
	{
		dns_resolved = false;
		addLogAdv(LOG_INFO, LOG_FEATURE_MQTT, "mqtt_host %s resolution FAILED\r\n", name);
	}
	dns_in_progress_time = 0;
}

static int MQTT_do_connect(mqtt_client_t* client)
{
	const char* mqtt_userName, * mqtt_host, * mqtt_pass, * mqtt_clientID;
	int mqtt_port;
	int res;
	struct hostent* hostEntry;
	char will_topic[CGF_MQTT_CLIENT_ID_SIZE + 16];
	bool mqtt_use_tls, mqtt_verify_tls_cert;

	mqtt_host = CFG_GetMQTTHost();

	if (!mqtt_host[0]) {
		addLogAdv(LOG_INFO, LOG_FEATURE_MQTT, "mqtt_host empty, not starting mqtt\r\n");
		snprintf(mqtt_status_message, sizeof(mqtt_status_message), "mqtt_host empty, not starting mqtt");
		return 0;
	}

	mqtt_userName = CFG_GetMQTTUserName();
	mqtt_pass = CFG_GetMQTTPass();
	mqtt_clientID = CFG_GetMQTTClientId();
	mqtt_port = CFG_GetMQTTPort();
#if MQTT_USE_TLS
	mqtt_use_tls = CFG_GetMQTTUseTls();
	mqtt_verify_tls_cert = CFG_GetMQTTVerifyTlsCert();
#endif

	if (dns_in_progress_time <= 0 && !dns_resolved) {
		addLogAdv(LOG_INFO, LOG_FEATURE_MQTT, "mqtt_userName %s\r\nmqtt_pass %s\r\nmqtt_clientID %s\r\nmqtt_host %s:%d\r\n",
			mqtt_userName,
			/* do not log sensitive data */
			//mqtt_pass,
			"********",
			mqtt_clientID,
			mqtt_host,
			mqtt_port
		);
	}

	// set pointer, there are no buffers to strcpy
	// empty field for us means "no password", etc,
	// but LWIP (without mods) expects a NULL pointer in that case...
	mqtt_client_info.client_id = mqtt_clientID;
	if(mqtt_pass[0] != 0) {
		mqtt_client_info.client_pass = mqtt_pass;
	} else {
		mqtt_client_info.client_pass = 0;
	}
	if(mqtt_userName[0] != 0) {
		mqtt_client_info.client_user = mqtt_userName;
	} else {
		mqtt_client_info.client_user = 0;
	}

	sprintf(will_topic, "%s/connected", mqtt_clientID);
	mqtt_client_info.will_topic = will_topic;
	mqtt_client_info.will_msg = "offline";
	mqtt_client_info.will_retain = true;
	mqtt_client_info.will_qos = 1;

#ifdef WINDOWS
	hostEntry = gethostbyname(mqtt_host);
	// host name/ip
	if (NULL != hostEntry)
	{
#ifndef LINUX
		if (hostEntry->h_addr_list && hostEntry->h_addr_list[0]) {
			int len = hostEntry->h_length;
			if (len > 4) {
				addLogAdv(LOG_INFO, LOG_FEATURE_MQTT, "mqtt_host resolves to addr len > 4\r\n");
				len = 4;
			}
			memcpy(&mqtt_ip, hostEntry->h_addr_list[0], len);
		}
		else 
#endif
		{
			addLogAdv(LOG_INFO, LOG_FEATURE_MQTT, "mqtt_host resolves no addresses?\r\n");
			snprintf(mqtt_status_message, sizeof(mqtt_status_message), "mqtt_host resolves no addresses?");
			return 0;
		}
#else
	if (dns_in_progress_time <= 0 && !dns_resolved)
	{
#ifdef PLATFORM_XR809
		res = dns_gethostbyname(mqtt_host, &mqtt_ip_resolved, dnsFound, NULL);
#else
	    res = dns_gethostbyname_addrtype(mqtt_host, &mqtt_ip_resolved, dnsFound, NULL, LWIP_DNS_ADDRTYPE_IPV4);
#endif
		if (ERR_OK == res)
		{
			dns_in_progress_time = 0;
			dns_resolved = true;
		}
		else if (ERR_INPROGRESS == res)
		{
			dns_in_progress_time = 10;
			dns_resolved = false;
		}
		else
		{
			dns_in_progress_time = 0;
			dns_resolved = false;
		}
	}

		// host name/ip
		//ipaddr_aton(mqtt_host,&mqtt_ip);
	if (dns_in_progress_time <= 0 && dns_resolved)
	{
		dns_resolved = false;
		memcpy(&mqtt_ip, &mqtt_ip_resolved, sizeof(mqtt_ip_resolved));

		/* Includes for MQTT over TLS */
#if MQTT_USE_TLS
		/* Free old configuration */
		if (mqtt_client_info.tls_config) {
			altcp_tls_free_config(mqtt_client_info.tls_config);
			altcp_tls_free_entropy();
			mqtt_client_info.tls_config = NULL;
		}
		if (mqtt_use_tls) {
			addLogAdv(LOG_INFO, LOG_FEATURE_MQTT, "Secure TLS connection enabled");
			size_t ca_len = 0;
			u8_t* ca = NULL;
			if (mqtt_verify_tls_cert) {
				if (strlen(CFG_GetMQTTCertFile()) > 0) {
					addLogAdv(LOG_INFO, LOG_FEATURE_MQTT, "Load certificate %s", CFG_GetMQTTCertFile());
					ca = LFS_ReadFile(CFG_GetMQTTCertFile());
					if (ca) {
						ca_len = strlen((char*)ca)+1;
					}
				}
			}
			else {
				addLogAdv(LOG_INFO, LOG_FEATURE_MQTT, "Verify certificate disabled");
			}
			mqtt_client_info.tls_config = altcp_tls_create_config_client(ca, ca_len);
			if (ca) {
				free(ca);
				ca = NULL;
			}
			if (mqtt_client_info.tls_config) {				
#if ALTCP_MBEDTLS_DEBUG
				mbedtls_ssl_conf_verify(&mqtt_client_info.tls_config->conf, mbedtls_verify_cb, NULL);
#if MBEDTLS_DEBUG_C
				mbedtls_ssl_conf_dbg(&mqtt_client_info.tls_config->conf, mbedtls_debug_cb, NULL);
				mbedtls_debug_set_threshold(1);
#endif
				if (mqtt_client_info.tls_config->ca){
					mbedtls_dump_conf(&mqtt_client_info.tls_config->conf, NULL);
				}
#endif				

				if (mqtt_verify_tls_cert) {
					mbedtls_ssl_conf_authmode(&mqtt_client_info.tls_config->conf, MBEDTLS_SSL_VERIFY_REQUIRED);
				}
				else {
					mbedtls_ssl_conf_authmode(&mqtt_client_info.tls_config->conf, MBEDTLS_SSL_VERIFY_OPTIONAL);
				}
			}
			else {
				addLogAdv(LOG_INFO, LOG_FEATURE_MQTT, "Secure TLS config fail. Try connect anyway.");
			}
		}
#endif /* MQTT_USE_TLS */


#endif /* ELSE WINDOWS*/

		/* Initiate client and connect to server, if this fails immediately an error code is returned
		  otherwise mqtt_connection_cb will be called with connection result after attempting
		  to establish a connection with the server.
		  For now MQTT version 3.1.1 is always used */

		LOCK_TCPIP_CORE();
		res = mqtt_client_connect(mqtt_client,
			&mqtt_ip, mqtt_port,
			mqtt_connection_cb, LWIP_CONST_CAST(void*, &mqtt_client_info),
			&mqtt_client_info);
		UNLOCK_TCPIP_CORE();
		mqtt_connect_result = res;
		if (res != ERR_OK)
		{
			addLogAdv(LOG_INFO, LOG_FEATURE_MQTT, "Connect error in mqtt_client_connect - code: %d (%s)\n", res, get_error_name(res));
			snprintf(mqtt_status_message, sizeof(mqtt_status_message), "mqtt_client_connect connect failed");
			if (res == ERR_ISCONN)
			{
				mqtt_disconnect(mqtt_client);
			}
		}
		else {
			mqtt_status_message[0] = '\0';
		}
		return res;
	}
	else {
		if (dns_in_progress_time > 0)
		{
			addLogAdv(LOG_INFO, LOG_FEATURE_MQTT, "mqtt_host %s is being resolved by gethostbyname\r\n", mqtt_host);
			dns_in_progress_time--;
			/* Discount connection event if host is being resolved */
			mqtt_connect_events--;
		}
		else
		{
			if (!dns_resolved)
			{
				addLogAdv(LOG_INFO, LOG_FEATURE_MQTT, "mqtt_host %s not found by gethostbyname\r\n", mqtt_host);
				snprintf(mqtt_status_message, sizeof(mqtt_status_message), "mqtt_host %s not found by gethostbyname", mqtt_host);
			}
		}
	}
	return 0;
}

OBK_Publish_Result MQTT_PublishMain_StringInt(const char* sChannel, int iv, int flags)
{
	char valueStr[16];

	sprintf(valueStr, "%i", iv);

	return MQTT_PublishMain(mqtt_client, sChannel, valueStr, flags, true);

}
OBK_Publish_Result MQTT_PublishMain_StringFloat(const char* sChannel, float f, int maxDecimalPlaces, int flags)
{
	char valueStr[16];
	if (isnan(f)) {
		f = 0;
	}

	sprintf(valueStr, "%f", f);
	// fix decimal places
	if (maxDecimalPlaces >= 0) {
		stripDecimalPlaces(valueStr, maxDecimalPlaces);
	}

	return MQTT_PublishMain(mqtt_client, sChannel, valueStr, flags, true);

}
OBK_Publish_Result MQTT_PublishMain_StringString(const char* sChannel, const char* valueStr, int flags)
{

	return MQTT_PublishMain(mqtt_client, sChannel, valueStr, flags, true);

}

OBK_Publish_Result MQTT_ChannelPublish(int channel, int flags)
{
	char channelNameStr[8];
	char valueStr[16];

	// allow users to force-hide some channels (those channels are NEVER published)
	if (CHANNEL_HasNeverPublishFlag(channel)) {
		return OBK_PUBLISH_OK;
	}

	if (CFG_HasFlag(OBK_FLAG_PUBLISH_MULTIPLIED_VALUES)) {
		float dVal = CHANNEL_GetFinalValue(channel);
		// Float value
		addLogAdv(LOG_INFO, LOG_FEATURE_MQTT, "Channel has changed! Publishing %f to channel %i \n", dVal, channel);
		sprintf(valueStr, "%f", dVal);
	}
	else {
		int iVal = CHANNEL_Get(channel);
		// Integer value
		addLogAdv(LOG_INFO, LOG_FEATURE_MQTT, "Channel has changed! Publishing %i to channel %i \n", iVal, channel);
		sprintf(valueStr, "%i", iVal);
	}

	MQTT_BroadcastTasmotaTeleSTATE();
	MQTT_BroadcastTasmotaTeleSENSOR();

	// String from channel number
	sprintf(channelNameStr, "%i", channel);

	// This will set RETAIN flag for all channels that are used for RELAY
	if (CFG_HasFlag(OBK_FLAG_MQTT_RETAIN_POWER_CHANNELS)) {
		if (CHANNEL_IsPowerRelayChannel(channel)) {
			flags |= OBK_PUBLISH_FLAG_RETAIN;
		}
	}

	return MQTT_PublishMain(mqtt_client, channelNameStr, valueStr, flags, true);
}
// This console command will trigger a publish of all used variables (channels and extra stuff)
commandResult_t MQTT_PublishAll(const void* context, const char* cmd, const char* args, int cmdFlags) {
	MQTT_PublishWholeDeviceState_Internal(true);
	return CMD_RES_OK;// TODO make return values consistent for all console commands
}
// This console command will trigger a publish of runtime variables
commandResult_t MQTT_PublishChannels(const void* context, const char* cmd, const char* args, int cmdFlags) {
	MQTT_PublishOnlyDeviceChannelsIfPossible();
	return CMD_RES_OK;// TODO make return values consistent for all console commands
}
commandResult_t MQTT_PublishChannel(const void* context, const char* cmd, const char* args, int cmdFlags) {
	int channelIndex;

	Tokenizer_TokenizeString(args, 0);

	if (Tokenizer_GetArgsCount() < 1) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	channelIndex = Tokenizer_GetArgInteger(0);

	MQTT_ChannelPublish(channelIndex,0);

	return CMD_RES_OK;
}
commandResult_t MQTT_PublishCommand(const void* context, const char* cmd, const char* args, int cmdFlags) {
	const char* topic, * value;
	OBK_Publish_Result ret;
	int flags = 0;

	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_ALLOW_ESCAPING_QUOTATIONS | TOKENIZER_EXPAND_EARLY);

	if (Tokenizer_GetArgsCount() < 2) {
		addLogAdv(LOG_INFO, LOG_FEATURE_MQTT, "Publish command requires two arguments (topic and value)");
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	topic = Tokenizer_GetArg(0);
	value = Tokenizer_GetArg(1);
	// optional third argument to remove get, etc
	if (Tokenizer_GetArgIntegerDefault(2, 0) != 0) {
		flags = OBK_PUBLISH_FLAG_RAW_TOPIC_NAME;
	}
	ret = MQTT_PublishMain_StringString(topic, value, flags);

	return CMD_RES_OK;
}
/*
1. Create LittleFS file with any string
2. Use command: PublishFile [TopicName] [FileName] [bOptionalForRemoveGet]
	Like: PublishFile myTopic myFile.txt 1

*/
#if ENABLE_LITTLEFS
commandResult_t MQTT_PublishFile(const void* context, const char* cmd, const char* args, int cmdFlags) {
	const char* topic, *fname;
	OBK_Publish_Result ret;
	int flags = 0;
	byte*data;

	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_ALLOW_ESCAPING_QUOTATIONS | TOKENIZER_EXPAND_EARLY);

	if (Tokenizer_GetArgsCount() < 2) {
		addLogAdv(LOG_INFO, LOG_FEATURE_MQTT, "Publish command requires two arguments (topic and value)");
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	topic = Tokenizer_GetArg(0);
	fname = Tokenizer_GetArg(1);
	// optional third argument to remove get, etc
	if (Tokenizer_GetArgIntegerDefault(2, 0) != 0) {
		flags = OBK_PUBLISH_FLAG_RAW_TOPIC_NAME;
	}
	data = LFS_ReadFileExpanding(fname);
	if (data) {
		ret = MQTT_PublishMain_StringString(topic, (const char*)data, flags);
		free(data);
	}

	return CMD_RES_OK;
}
#endif
// we have a separate command for integer because it can support math expressions
// (so it handled like $CH10*10, etc)
commandResult_t MQTT_PublishCommandInteger(const void* context, const char* cmd, const char* args, int cmdFlags) {
	const char* topic;
	int value;
	OBK_Publish_Result ret;
	int flags = 0;

	Tokenizer_TokenizeString(args, 0);

	if (Tokenizer_GetArgsCount() < 2) {
		addLogAdv(LOG_INFO, LOG_FEATURE_MQTT, "Publish command requires two arguments (topic and value)");
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	topic = Tokenizer_GetArg(0);
	value = Tokenizer_GetArgInteger(1);
	// optional third argument to remove get, etc
	if (Tokenizer_GetArgIntegerDefault(2, 0) != 0) {
		flags = OBK_PUBLISH_FLAG_RAW_TOPIC_NAME;
	}
	ret = MQTT_PublishMain_StringInt(topic, value, flags);

	return CMD_RES_OK;
}

// we have a separate command for float because it can support math expressions
// (so it handled like $CH10*0.01, etc)
commandResult_t MQTT_PublishCommandFloat(const void* context, const char* cmd, const char* args, int cmdFlags) {
	const char* topic;
	float value;
	OBK_Publish_Result ret;
	int flags = 0;
	int decimalPlaces;

	Tokenizer_TokenizeString(args, 0);

	if (Tokenizer_GetArgsCount() < 2) {
		addLogAdv(LOG_INFO, LOG_FEATURE_MQTT, "Publish command requires two arguments (topic and value)");
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	topic = Tokenizer_GetArg(0);
	value = Tokenizer_GetArgFloat(1);

	// optional third argument to remove get, etc
	if (Tokenizer_GetArgIntegerDefault(2, 0) != 0) {
		flags = OBK_PUBLISH_FLAG_RAW_TOPIC_NAME;
	}
	// optional fourth argument to set rounding
	decimalPlaces = Tokenizer_GetArgIntegerDefault(3, -1);
	ret = MQTT_PublishMain_StringFloat(topic, value, decimalPlaces, flags);

	return CMD_RES_OK;
}
commandResult_t MQTT_PublishCommandDriver(const void* context, const char* cmd, const char* args, int cmdFlags) {
	const char* driver;
	OBK_Publish_Result ret;

	Tokenizer_TokenizeString(args, 0);

	driver = Tokenizer_GetArg(0);
	bool bOn = DRV_IsRunning(driver);

	char full[32];
	sprintf(full,"driver/%s", driver);
	ret = MQTT_PublishMain_StringInt(full, bOn, OBK_PUBLISH_FLAG_FORCE_REMOVE_GET);

	return CMD_RES_OK;
}
/****************************************************************************************************
 *
 ****************************************************************************************************/
#define MQTT_TMR_DURATION      50

typedef struct BENCHMARK_TEST_INFO
{
	portTickType TestStartTick;
	portTickType TestStopTick;
	long msg_cnt;
	long msg_num;
	char topic[256];
	char value[256];
	float bench_time;
	float bench_rate;
	bool report_published;
} BENCHMARK_TEST_INFO;

#ifndef portTICK_RATE_MS
#define portTICK_RATE_MS ( ( portTickType ) 1000 / configTICK_RATE_HZ )
#endif

void MQTT_Test_Tick(void* param)
{
	BENCHMARK_TEST_INFO* info = (BENCHMARK_TEST_INFO*)param;
	int block = 1;
	err_t err;
	int qos = 1;
	int retain = 0;

	if (info != NULL)
	{
		while (1)
		{

			LOCK_TCPIP_CORE();
			int res = mqtt_client_is_connected(mqtt_client);
			UNLOCK_TCPIP_CORE();

			if (res == 0)
				break;
			if (info->msg_cnt < info->msg_num)
			{
				sprintf(info->value, "TestMSG: %li/%li Time: %i s, Rate: %i msg/s", info->msg_cnt, info->msg_num,
					(int)info->bench_time, (int)info->bench_rate);
				LOCK_TCPIP_CORE();
				err = mqtt_publish(mqtt_client, info->topic, info->value, strlen(info->value), qos, retain, mqtt_pub_request_cb, 0);
				UNLOCK_TCPIP_CORE();
				if (err == ERR_OK)
				{
					/* MSG published */
					info->msg_cnt++;
					info->TestStopTick = xTaskGetTickCount();
					/* calculate stats */
					info->bench_time = (float)(info->TestStopTick - info->TestStartTick);
					info->bench_time /= (float)(1000 / portTICK_RATE_MS);
					info->bench_rate = (float)info->msg_cnt;
					if (info->bench_time != 0.0)
						info->bench_rate /= info->bench_time;
					block--;
					if (block <= 0)
						break;
				}
				else {
					/* MSG not published, error occured */
					break;
				}
			}
			else {
				/* All messages publiched */
				if (info->report_published == false)
				{
					/* Publish report */
					sprintf(info->value, "Benchmark completed. %li msg published. Total Time: %i s MsgRate: %i msg/s",
						info->msg_cnt, (int)info->bench_time, (int)info->bench_rate);
					LOCK_TCPIP_CORE();
					err = mqtt_publish(mqtt_client, info->topic, info->value, strlen(info->value), qos, retain, mqtt_pub_request_cb, 0);
					UNLOCK_TCPIP_CORE();
					if (err == ERR_OK)
					{
						/* Report published */
						addLogAdv(LOG_INFO, LOG_FEATURE_MQTT, info->value);
						info->report_published = true;
						/* Stop timer */
					}
				}
				break;
			}
		}
	}
}

commandResult_t MQTT_SetTasTeleIntervals(const void* context, const char* cmd, const char* args, int cmdFlags)
{
	Tokenizer_TokenizeString(args, 0);
	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 2)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

	g_teleSensor_interval = Tokenizer_GetArgInteger(0);
	g_teleState_interval = Tokenizer_GetArgInteger(1);

	return CMD_RES_OK;
}
commandResult_t MQTT_SetMaxBroadcastItemsPublishedPerSecond(const void* context, const char* cmd, const char* args, int cmdFlags)
{
	Tokenizer_TokenizeString(args, 0);
	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	g_maxBroadcastItemsPublishedPerSecond = Tokenizer_GetArgInteger(0);

	return CMD_RES_OK;
}
commandResult_t MQTT_SetBroadcastInterval(const void* context, const char* cmd, const char* args, int cmdFlags)
{
	Tokenizer_TokenizeString(args, 0);
	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	g_intervalBetweenMQTTBroadcasts = Tokenizer_GetArgInteger(0);

	return CMD_RES_OK;
}
static BENCHMARK_TEST_INFO* info = NULL;

#if WINDOWS

#elif PLATFORM_BL602 || PLATFORM_W600 || PLATFORM_W800 || PLATFORM_ESPIDF || PLATFORM_TR6260 \
	|| PLATFORM_REALTEK || PLATFORM_ECR6600 || PLATFORM_ESP8266 || PLATFORM_TXW81X || PLATFORM_RDA5981
static void mqtt_timer_thread(void* param)
{
	while (1)
	{
		rtos_delay_milliseconds(MQTT_TMR_DURATION);
		MQTT_Test_Tick(param);
	}
}
#elif PLATFORM_XRADIO || PLATFORM_LN882H
static OS_Timer_t timer;
#else
static beken_timer_t g_mqtt_timer;
#endif

commandResult_t MQTT_StartMQTTTestThread(const void* context, const char* cmd, const char* args, int cmdFlags)
{
	if (info != NULL)
	{
		/* Benchmark test already started */
		/* try to restart */
		info->TestStartTick = xTaskGetTickCount();
		info->msg_cnt = 0;
		info->report_published = false;
		return CMD_RES_OK;
	}

	info = (BENCHMARK_TEST_INFO*)os_malloc(sizeof(BENCHMARK_TEST_INFO));
	if (info == NULL)
	{
		return CMD_RES_ERROR;
	}

	memset(info, 0, sizeof(BENCHMARK_TEST_INFO));
	info->TestStartTick = xTaskGetTickCount();
	info->msg_num = 1000;
	sprintf(info->topic, "%s/benchmark", CFG_GetMQTTClientId());

#if WINDOWS

#elif PLATFORM_BL602 || PLATFORM_W600 || PLATFORM_W800 || PLATFORM_ESPIDF || PLATFORM_TR6260 \
	|| PLATFORM_REALTEK || PLATFORM_ECR6600 || PLATFORM_ESP8266
	xTaskCreate(mqtt_timer_thread, "mqtt", 1024, (void*)info, 15, NULL);
#elif PLATFORM_TXW81X
	os_task_create("mqtt", mqtt_timer_thread, (void*)info, 15, 0, NULL, 1024);
#elif PLATFORM_RDA5981
	rda_thread_new("mqtt", mqtt_timer_thread, NULL, 1024, osPriorityNormal);
#elif PLATFORM_XRADIO || PLATFORM_LN882H
	OS_TimerSetInvalid(&timer);
	if (OS_TimerCreate(&timer, OS_TIMER_PERIODIC, MQTT_Test_Tick, (void*)info, MQTT_TMR_DURATION) != OS_OK)
	{
		printf("PIN_AddCommands timer create failed\n");
		return CMD_RES_ERROR;
	}
	OS_TimerStart(&timer); /* start OS timer to feed watchdog */
#else
	OSStatus result;

	result = rtos_init_timer(&g_mqtt_timer, MQTT_TMR_DURATION, MQTT_Test_Tick, (void*)info);
	ASSERT(kNoErr == result);
	result = rtos_start_timer(&g_mqtt_timer);
	ASSERT(kNoErr == result);
#endif
	return CMD_RES_OK;
}

/****************************************************************************************************
 *
 ****************************************************************************************************/

void MQTT_InitCallbacks() {
	char cbtopicbase[CGF_MQTT_CLIENT_ID_SIZE + 16];
	char cbtopicsub[CGF_MQTT_CLIENT_ID_SIZE + 16];
	const char* clientId;
	const char* groupId;

	MQTT_ClearCallbacks();
	g_mqtt_bBaseTopicDirty = 0;

	clientId = CFG_GetMQTTClientId();
	groupId = CFG_GetMQTTGroupTopic();

	// register the main set channel callback
	snprintf(cbtopicbase, sizeof(cbtopicbase), "%s/", clientId);
	snprintf(cbtopicsub, sizeof(cbtopicsub), "%s/+/set", clientId);
	// note: this may REPLACE an existing entry with the same ID.  ID 1 !!!
	MQTT_RegisterCallback(cbtopicbase, cbtopicsub, 1, channelSet);

	// so-called "Group topic", a secondary topic that can be set on multiple devices 
	// to control them together
	// register the TAS cmnd callback
	if (*groupId) {
		// register the main set channel callback
		snprintf(cbtopicbase, sizeof(cbtopicbase), "%s/", groupId);
		snprintf(cbtopicsub, sizeof(cbtopicsub), "%s/+/set", groupId);
		// note: this may REPLACE an existing entry with the same ID.  ID 2 !!!
		MQTT_RegisterCallback(cbtopicbase, cbtopicsub, 2, channelSet);
	}

	// base topic
	// register the TAS cmnd callback
	snprintf(cbtopicbase, sizeof(cbtopicbase), "cmnd/%s/", clientId);
	snprintf(cbtopicsub, sizeof(cbtopicsub), "cmnd/%s/+", clientId);
	// note: this may REPLACE an existing entry with the same ID.  ID 3 !!!
	MQTT_RegisterCallback(cbtopicbase, cbtopicsub, 3, tasCmnd);

	// so-called "Group topic", a secondary topic that can be set on multiple devices 
	// to control them together
	// register the TAS cmnd callback
	if (*groupId) {
		snprintf(cbtopicbase, sizeof(cbtopicbase), "cmnd/%s/", groupId);
		snprintf(cbtopicsub, sizeof(cbtopicsub), "cmnd/%s/+", groupId);
		// note: this may REPLACE an existing entry with the same ID.  ID 4 !!!
		MQTT_RegisterCallback(cbtopicbase, cbtopicsub, 4, tasCmnd);
	}

	// register the getter callback (send empty message here to get reply)
	snprintf(cbtopicbase, sizeof(cbtopicbase), "%s/", clientId);
	snprintf(cbtopicsub, sizeof(cbtopicsub), "%s/+/get", clientId);
	// note: this may REPLACE an existing entry with the same ID.  ID 5 !!!
	MQTT_RegisterCallback(cbtopicbase, cbtopicsub, 5, channelGet);

	if (CFG_HasFlag(OBK_FLAG_DO_TASMOTA_TELE_PUBLISHES)) {
		// test hack iobroker
		snprintf(cbtopicbase, sizeof(cbtopicbase), "tele/%s/", clientId);
		snprintf(cbtopicsub, sizeof(cbtopicsub), "tele/%s/+", clientId);
		// note: this may REPLACE an existing entry with the same ID.  ID 6 !!!
		MQTT_RegisterCallback(cbtopicbase, cbtopicsub, 6, tasCmnd);

		// test hack iobroker
		snprintf(cbtopicbase, sizeof(cbtopicbase), "stat/%s/", clientId);
		snprintf(cbtopicsub, sizeof(cbtopicsub), "stat/%s/+", clientId);
		// note: this may REPLACE an existing entry with the same ID.  ID 7 !!!
		MQTT_RegisterCallback(cbtopicbase, cbtopicsub, 7, tasCmnd);
	}
	// test hack iobroker
	snprintf(cbtopicbase, sizeof(cbtopicbase), "homeassistant/");
	snprintf(cbtopicsub, sizeof(cbtopicsub), "homeassistant/+");
	MQTT_RegisterCallback(cbtopicbase, cbtopicsub, 8, onHassStatus);
}
 // initialise things MQTT
 // called from user_main
void MQTT_init()
{
	// WINDOWS must support reinit
#ifdef WINDOWS
	mqtt_client = 0;
#endif

	MQTT_InitCallbacks();

	mqtt_initialised = 1;

	//cmddetail:{"name":"publish","args":"[Topic][Value][bOptionalSkipPrefixAndSuffix]",
	//cmddetail:"descr":"Publishes data by MQTT. The final topic will be obk0696FB33/[Topic]/get, but you can also publish under raw topic, by adding third argument - '1'. You can use argument expansion here, so $CH11 will change to value of the channel 11",
	//cmddetail:"fn":"MQTT_PublishCommand","file":"mqtt/new_mqtt.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("publish", MQTT_PublishCommand, NULL);
	//cmddetail:{"name":"publishInt","args":"[Topic][Value][bOptionalSkipPrefixAndSuffix]",
	//cmddetail:"descr":"Publishes data by MQTT. The final topic will be obk0696FB33/[Topic]/get, but you can also publish under raw topic, by adding third argument - '1'.. You can use argument expansion here, so $CH11 will change to value of the channel 11. This version of command publishes an integer, so you can also use math expressions like $CH10*10, etc.",
	//cmddetail:"fn":"MQTT_PublishCommandInteger","file":"mqtt/new_mqtt.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("publishInt", MQTT_PublishCommandInteger, NULL);
	//cmddetail:{"name":"publishFloat","args":"[Topic][Value][bOptionalSkipPrefixAndSuffix]",
	//cmddetail:"descr":"Publishes data by MQTT. The final topic will be obk0696FB33/[Topic]/get, but you can also publish under raw topic, by adding third argument - '1'.. You can use argument expansion here, so $CH11 will change to value of the channel 11. This version of command publishes an float, so you can also use math expressions like $CH10*0.0, etc.",
	//cmddetail:"fn":"MQTT_PublishCommandFloat","file":"mqtt/new_mqtt.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("publishFloat", MQTT_PublishCommandFloat, NULL);
	//cmddetail:{"name":"publishAll","args":"",
	//cmddetail:"descr":"Starts the step by step publish of all available values",
	//cmddetail:"fn":"MQTT_PublishAll","file":"mqtt/new_mqtt.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("publishAll", MQTT_PublishAll, NULL);
	//cmddetail:{"name":"publishChannel","args":"[ChannelIndex]",
	//cmddetail:"descr":"Forces publish of given channel",
	//cmddetail:"fn":"MQTT_PublishChannel","file":"mqtt/new_mqtt.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("publishChannel", MQTT_PublishChannel, NULL);
	//cmddetail:{"name":"publishChannels","args":"",
	//cmddetail:"descr":"Starts the step by step publish of all channel values",
	//cmddetail:"fn":"MQTT_PublishChannels","file":"mqtt/new_mqtt.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("publishChannels", MQTT_PublishChannels, NULL);
	//cmddetail:{"name":"publishBenchmark","args":"",
	//cmddetail:"descr":"",
	//cmddetail:"fn":"MQTT_StartMQTTTestThread","file":"mqtt/new_mqtt.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("publishBenchmark", MQTT_StartMQTTTestThread, NULL);
	//cmddetail:{"name":"mqtt_broadcastInterval","args":"[ValueSeconds]",
	//cmddetail:"descr":"If broadcast self state every 60 seconds/minute is enabled in flags, this value allows you to change the delay, change this 60 seconds to any other value in seconds. This value is not saved, you must use autoexec.bat or short startup command to execute it on every reboot.",
	//cmddetail:"fn":"MQTT_SetBroadcastInterval","file":"mqtt/new_mqtt.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("mqtt_broadcastInterval", MQTT_SetBroadcastInterval, NULL);
	//cmddetail:{"name":"mqtt_broadcastItemsPerSec","args":"[PublishCountPerSecond]",
	//cmddetail:"descr":"If broadcast self state (this option in flags) is started, then gradually device info is published, with a speed of N publishes per second. Do not set too high value, it may overload LWIP MQTT library. This value is not saved, you must use autoexec.bat or short startup command to execute it on every reboot.",
	//cmddetail:"fn":"MQTT_SetMaxBroadcastItemsPublishedPerSecond","file":"mqtt/new_mqtt.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("mqtt_broadcastItemsPerSec", MQTT_SetMaxBroadcastItemsPublishedPerSecond, NULL);
	//cmddetail:{"name":"TasTeleInterval","args":"[SensorInterval][StateInterval]",
	//cmddetail:"descr":"This allows you to configure Tasmota TELE publish intervals, only if you have TELE flag enabled. First argument is interval for sensor publish (energy metering, etc), second is interval for State tele publish.",
	//cmddetail:"fn":"MQTT_SetTasTeleIntervals","file":"mqtt/new_mqtt.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("TasTeleInterval", MQTT_SetTasTeleIntervals, NULL);

#if ENABLE_LITTLEFS
	//cmddetail:{"name":"publishFile","args":"[Topic][Value][bOptionalSkipPrefixAndSuffix]",
	//cmddetail:"descr":"Publishes data read from LFS file by MQTT. The final topic will be obk0696FB33/[Topic]/get, but you can also publish under raw topic, by adding third argument - '1'.",
	//cmddetail:"fn":"MQTT_PublishFile","file":"mqtt/new_mqtt.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("publishFile", MQTT_PublishFile, NULL);
#endif


	//cmddetail:{"name":"publishDriver","args":"TODO",
	//cmddetail:"descr":"",
	//cmddetail:"fn":"MQTT_PublishCommandDriver","file":"mqtt/new_mqtt.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("publishDriver", MQTT_PublishCommandDriver, NULL);
}
static float getInternalTemperature() {
	return g_wifi_temperature;
}

OBK_Publish_Result MQTT_DoItemPublishString(const char* sChannel, const char* valueStr)
{
	return MQTT_PublishMain(mqtt_client, sChannel, valueStr, OBK_PUBLISH_FLAG_MUTEX_SILENT, false);
}

OBK_Publish_Result MQTT_DoItemPublish(int idx)
{
	//int type;
	char dataStr[3 * 6 + 1];  //This is sufficient to hold mac value
	bool bWantsToPublish;

	switch (idx) {
	case PUBLISHITEM_SELF_STATIC_RESERVED_2:
	case PUBLISHITEM_SELF_STATIC_RESERVED_1:
		return OBK_PUBLISH_WAS_NOT_REQUIRED;

	case PUBLISHITEM_QUEUED_VALUES:
		return PublishQueuedItems();

	case PUBLISHITEM_SELF_DYNAMIC_LIGHTSTATE:
	{
#if ENABLE_LED_BASIC
		if (LED_IsLEDRunning()) {
			return LED_SendEnableAllState();
		}
#endif
		return OBK_PUBLISH_WAS_NOT_REQUIRED;
	}
	case PUBLISHITEM_SELF_DYNAMIC_LIGHTMODE:
	{
#if ENABLE_LED_BASIC
		if (LED_IsLEDRunning()) {
			return LED_SendCurrentLightModeParam_TempOrColor();
		}
#endif
		return OBK_PUBLISH_WAS_NOT_REQUIRED;
	}
	case PUBLISHITEM_SELF_DYNAMIC_DIMMER:
	{
#if ENABLE_LED_BASIC
		if (LED_IsLEDRunning()) {
			return LED_SendDimmerChange();
		}
#endif
		return OBK_PUBLISH_WAS_NOT_REQUIRED;
	}

	case PUBLISHITEM_SELF_HOSTNAME:
		return MQTT_DoItemPublishString("host", CFG_GetShortDeviceName());

	case PUBLISHITEM_SELF_BUILD:
		return MQTT_DoItemPublishString("build", BUILD_AND_VERSION_FOR_MQTT);

	case PUBLISHITEM_SELF_MAC:
		return MQTT_DoItemPublishString("mac", HAL_GetMACStr(dataStr));

	case PUBLISHITEM_SELF_SSID:
		// TODO: correct SSID
		return MQTT_DoItemPublishString("ssid", CFG_GetWiFiSSID());


	case PUBLISHITEM_SELF_DATETIME:
// TIME_GetCurrentTime() is allways present
/*		//Drivers are only built on BK7231 chips
#ifndef OBK_DISABLE_ALL_DRIVERS

		if (DRV_IsRunning("NTP")) {
*/
#ifdef PLATFORM_ESP8266
		// while all other platforms will accept uint32_t as long unsigned, ESP8266 needs %u 
		// biuild fails otherwise because of -Werror=format
		// src/mqtt/new_mqtt.c:2036:24: error: format '%ld' expects argument of type 'long int', but argument 3 has type 'uint32_t' {aka 'unsigned int'} [-Werror=format=]
		// al other ESP:
		/// src/mqtt/new_mqtt.c:2036:44: error: format '%d' expects argument of type 'int', but argument 3 has type 'uint32_t' {aka 'long unsigned int'} [-Werror=format=]
		sprintf(dataStr, "%u", TIME_GetCurrentTime());
#else
		sprintf(dataStr, "%lu", TIME_GetCurrentTime());
#endif
		return MQTT_DoItemPublishString("datetime", dataStr);
/*		}
		else {
			return OBK_PUBLISH_WAS_NOT_REQUIRED;
		}
#else
		return OBK_PUBLISH_WAS_NOT_REQUIRED;
#endif
*/
	case PUBLISHITEM_SELF_SOCKETS:
		sprintf(dataStr, "%d", LWIP_GetActiveSockets());
		return MQTT_DoItemPublishString("sockets", dataStr);

#ifndef NO_CHIP_TEMPERATURE
	case PUBLISHITEM_SELF_TEMP:
		sprintf(dataStr, "%.2f", getInternalTemperature());
		return MQTT_DoItemPublishString("temp", dataStr);
#endif

	case PUBLISHITEM_SELF_RSSI:
		sprintf(dataStr, "%d", HAL_GetWifiStrength());
		return MQTT_DoItemPublishString("rssi", dataStr);

	case PUBLISHITEM_SELF_UPTIME:
		sprintf(dataStr, "%d", g_secondsElapsed);
		return MQTT_DoItemPublishString("uptime", dataStr);

	case PUBLISHITEM_SELF_FREEHEAP:
		sprintf(dataStr, "%d", xPortGetFreeHeapSize());
		return MQTT_DoItemPublishString("freeheap", dataStr);

	case PUBLISHITEM_SELF_IP:
		//g_firstFullBroadcast = false; //We published the last status item, disable full broadcast
		return MQTT_DoItemPublishString("ip", HAL_GetMyIPString());

	default:
		break;
	}

	// Do not publish raw channel value for channels like PWM values, RGBCW has 5 raw channels.
	// We do not need raw values for RGBCW lights (or RGB, etc)
	// because we are using led_basecolor_rgb, led_dimmer, led_enableAll, etc
	// NOTE: negative indexes are not channels - they are special values
	bWantsToPublish = false;
	if (CHANNEL_ShouldBePublished(idx)) {
		bWantsToPublish = true;
	}
	// TODO
	//type = CHANNEL_GetType(idx);
	if (bWantsToPublish) {
		return MQTT_ChannelPublish(g_publishItemIndex, OBK_PUBLISH_FLAG_MUTEX_SILENT);
	}

	return OBK_PUBLISH_WAS_NOT_REQUIRED; // didnt publish
}

// from 5ms quicktick
int MQTT_RunQuickTick(){
#ifndef PLATFORM_BEKEN
	// on Beken, we use a one-shot timer for this.
	MQTT_process_received();
#endif
	return 0;
}

int g_wantTasmotaTeleSend = 0;
void MQTT_BroadcastTasmotaTeleSENSOR() {
#if ENABLE_TASMOTA_JSON
	if (CFG_HasFlag(OBK_FLAG_DO_TASMOTA_TELE_PUBLISHES) == false) {
		return;
	}
	bool bHasAnySensor = false;
#ifndef OBK_DISABLE_ALL_DRIVERS
	if (DRV_IsMeasuringPower()) {
		bHasAnySensor = true;
	}
#endif
	if (bHasAnySensor) {
		MQTT_ProcessCommandReplyJSON("SENSOR", "", COMMAND_FLAG_SOURCE_TELESENDER);
	}
#endif
}
void MQTT_BroadcastTasmotaTeleSTATE() {
#if ENABLE_TASMOTA_JSON
	if (CFG_HasFlag(OBK_FLAG_DO_TASMOTA_TELE_PUBLISHES) == false) {
		return;
	}
	MQTT_ProcessCommandReplyJSON("STATE", "", COMMAND_FLAG_SOURCE_TELESENDER);
#endif
}
// called from user timer.
int MQTT_RunEverySecondUpdate()
{
	if (!mqtt_initialised)
		return 0;

	if (Main_HasWiFiConnected() == 0)
	{
		mqtt_reconnect = 0;
		if (Main_HasFastConnect()) {
			mqtt_loopsWithDisconnected = LOOPS_WITH_DISCONNECTED + 1;
		}
		else {
			mqtt_loopsWithDisconnected = LOOPS_WITH_DISCONNECTED - 2;
		}
		return 0;
	}

	// take mutex for connect and disconnect operations
	if (MQTT_Mutex_Take(100) == 0)
	{
		return 0;
	}
	if (g_mqtt_bBaseTopicDirty) {
		addLogAdv(LOG_INFO, LOG_FEATURE_MQTT, "MQTT base topic is dirty, will reinit callbacks and reconnect\n");
		MQTT_InitCallbacks();
		mqtt_reconnect = 5;
	}

	// reconnect if went into MQTT library ERR_MEM forever loop
	if (g_memoryErrorsThisSession >= 5)
	{
		addLogAdv(LOG_INFO, LOG_FEATURE_MQTT, "MQTT will reconnect soon to fix ERR_MEM errors\n");
		g_memoryErrorsThisSession = 0;
		mqtt_reconnect = 5;
	}

	int res = 0;
	if (mqtt_client){
		LOCK_TCPIP_CORE();
		res = mqtt_client_is_connected(mqtt_client);
		UNLOCK_TCPIP_CORE();
	}
	// if asked to reconnect (e.g. change of topic(s))
	if (mqtt_reconnect > 0)
	{
		mqtt_reconnect--;
		addLogAdv(LOG_INFO, LOG_FEATURE_MQTT, "MQTT has pending reconnect in %i\n", mqtt_reconnect);
		if (mqtt_reconnect == 0)
		{
			// then if connected, disconnect, and then it will reconnect automatically in 2s
			if (mqtt_client && res)
			{
				addLogAdv(LOG_INFO, LOG_FEATURE_MQTT, "MQTT will now do a forced reconnect\n");
				MQTT_disconnect(mqtt_client);
				mqtt_loopsWithDisconnected = LOOPS_WITH_DISCONNECTED - 2;
			}
		}
	}

	if (mqtt_client == 0 || res == 0)
	{
		//addLogAdv(LOG_INFO,LOG_FEATURE_MAIN, "Timer discovers disconnected mqtt %i\n",mqtt_loopsWithDisconnected);
		if (OTA_GetProgress() == -1)
		{
			mqtt_loopsWithDisconnected++;
			if (mqtt_loopsWithDisconnected > LOOPS_WITH_DISCONNECTED)
			{
				if (mqtt_client == 0)
				{
					LOCK_TCPIP_CORE();
					mqtt_client = mqtt_client_new();
					UNLOCK_TCPIP_CORE();
				}
				else
				{
					LOCK_TCPIP_CORE();
					mqtt_disconnect(mqtt_client);
#if defined(MQTT_CLIENT_CLEANUP)
					mqtt_client_cleanup(mqtt_client);
#endif
					UNLOCK_TCPIP_CORE();
				}
				if (MQTT_do_connect(mqtt_client) == ERR_RTE) {
					// silently allow retry next frame
				}
				else {
					mqtt_loopsWithDisconnected = 0;
				}
				mqtt_connect_events++;
			}
		}
		MQTT_Mutex_Free();
		return 0;
	}
	else {
		// things to do in our threads on connection accepted.
		if (g_just_connected){
			g_just_connected = 0;
			// publish all values on state
			if (CFG_HasFlag(OBK_FLAG_MQTT_BROADCASTSELFSTATEONCONNECT)) {
				g_wantTasmotaTeleSend = 1;
				MQTT_PublishWholeDeviceState();
			}
			else {
				//MQTT_PublishOnlyDeviceChannelsIfPossible();
			}
		}

		MQTT_Mutex_Free();
		// below mutex is not required any more

		// it is connected publish TELE
		if (g_wantTasmotaTeleSend) {
			MQTT_BroadcastTasmotaTeleSTATE();
			MQTT_BroadcastTasmotaTeleSENSOR();
			g_wantTasmotaTeleSend = 0;
		}
		g_timeSinceLastMQTTPublish++;
		if (OTA_GetProgress() != -1)
		{
			addLogAdv(LOG_INFO, LOG_FEATURE_MQTT, "OTA started MQTT will be closed\n");
			LOCK_TCPIP_CORE();
			mqtt_disconnect(mqtt_client);
			UNLOCK_TCPIP_CORE();
			return 1;
		}

		if (CFG_HasFlag(OBK_FLAG_DO_TASMOTA_TELE_PUBLISHES)) {
			static int g_mqtt_tasmotaTeleCounter_sensor = 0;
			g_mqtt_tasmotaTeleCounter_sensor++;
			if (g_mqtt_tasmotaTeleCounter_sensor >= g_teleSensor_interval) {
				g_mqtt_tasmotaTeleCounter_sensor = 0;
				MQTT_BroadcastTasmotaTeleSENSOR();
			}
			static int g_mqtt_tasmotaTeleCounter_state = 0;
			g_mqtt_tasmotaTeleCounter_state++;
			if (g_mqtt_tasmotaTeleCounter_state >= g_teleState_interval) {
				g_mqtt_tasmotaTeleCounter_state = 0;
				MQTT_BroadcastTasmotaTeleSTATE();
			}
		}

		// do we want to broadcast full state?
		// Do it slowly in order not to overload the buffers
		// The item indexes start at negative values for special items
		// and then covers Channel indexes up to CHANNEL_MAX
		//Handle only queued items. Don't need to do this separately if entire state is being published.
		if ((g_MqttPublishItemsQueued > 0) && !g_bPublishAllStatesNow)
		{
			PublishQueuedItems();
			return 1;
		}
		else if (g_bPublishAllStatesNow)
		{
			// Doing step by a step a full publish state
			//if (g_timeSinceLastMQTTPublish > 2)
			{
				OBK_Publish_Result publishRes;
				int g_sent_thisFrame = 0;

				while (g_publishItemIndex < CHANNEL_MAX)
				{
					publishRes = MQTT_DoItemPublish(g_publishItemIndex);
					if (publishRes != OBK_PUBLISH_WAS_NOT_REQUIRED)
					{
						if (false) {
							addLogAdv(LOG_INFO, LOG_FEATURE_MQTT, "[g_bPublishAllStatesNow] item %i result %i\n", g_publishItemIndex, publishRes);
						}
					}
					// There are several things that can happen now
					// OBK_PUBLISH_OK - it was required and was published
					if (publishRes == OBK_PUBLISH_OK)
					{
						g_sent_thisFrame++;
						if (g_sent_thisFrame >= g_maxBroadcastItemsPublishedPerSecond)
						{
							g_publishItemIndex++;
							break;
						}
					}
					// OBK_PUBLISH_MUTEX_FAIL - MQTT is busy
					if (publishRes == OBK_PUBLISH_MUTEX_FAIL
						|| publishRes == OBK_PUBLISH_WAS_DISCONNECTED)
					{
						// retry the same later
						break;
					}
					// OBK_PUBLISH_WAS_NOT_REQUIRED
					// The item is not used for this device
					g_publishItemIndex++;
				}

				if (g_publishItemIndex >= CHANNEL_MAX)
				{
					// done
					g_bPublishAllStatesNow = 0;
				}
			}
		}
		else {
			// not doing anything
			if (CFG_HasFlag(OBK_FLAG_MQTT_BROADCASTSELFSTATEPERMINUTE))
			{
				// this is called every second
				g_secondsBeforeNextFullBroadcast--;
				if (g_secondsBeforeNextFullBroadcast <= 0)
				{
					g_secondsBeforeNextFullBroadcast = g_intervalBetweenMQTTBroadcasts;
					MQTT_PublishWholeDeviceState();
				}
			}
		}
	}
	return 1;
}

MqttPublishItem_t* get_queue_tail(MqttPublishItem_t* head) {
	if (head == NULL) { return NULL; }

	while (head->next != NULL) {
		head = head->next;
	}
	return head;
}

MqttPublishItem_t* find_queue_reusable_item(MqttPublishItem_t* head) {
	while (head != NULL) {
		if (MQTT_QUEUE_ITEM_IS_REUSABLE(head)) {
			return head;
		}
		head = head->next;
	}
	return head;
}

/// @brief Queue an entry for publish and execute a command after the publish.
/// @param topic 
/// @param channel 
/// @param value 
/// @param flags
/// @param command Command to execute after the publish
void MQTT_QueuePublishWithCommand(const char* topic, const char* channel, const char* value, int flags, PostPublishCommands command) {
	MqttPublishItem_t* newItem;
	if (g_MqttPublishItemsQueued >= MQTT_MAX_QUEUE_SIZE) {
		addLogAdv(LOG_ERROR, LOG_FEATURE_MQTT, "Unable to queue! %i items already present\r\n", g_MqttPublishItemsQueued);
		return;
	}

	if ((strlen(topic) > MQTT_PUBLISH_ITEM_TOPIC_LENGTH) ||
		(strlen(channel) > MQTT_PUBLISH_ITEM_CHANNEL_LENGTH) ||
		(strlen(value) > MQTT_PUBLISH_ITEM_VALUE_LENGTH)) {
		addLogAdv(LOG_ERROR, LOG_FEATURE_MQTT, "Unable to queue! Topic (%i), channel (%i) or value (%i) exceeds size limit\r\n",
			strlen(topic), strlen(channel), strlen(value));
		return;
	}

	//Queue data for publish. This might be a new item in the queue or an existing item. This is done to prevent
	//memory fragmentation. The total queue length is limited to MQTT_MAX_QUEUE_SIZE.

	if (g_MqttPublishQueueHead == NULL) {
		g_MqttPublishQueueHead = newItem = os_malloc(sizeof(MqttPublishItem_t));
		newItem->next = NULL;
	}
	else {
		newItem = find_queue_reusable_item(g_MqttPublishQueueHead);

		if (newItem == NULL) {
			newItem = os_malloc(sizeof(MqttPublishItem_t));
			newItem->next = NULL;
			get_queue_tail(g_MqttPublishQueueHead)->next = newItem; //Append new item
		}
	}

	//strcpy does copy ending null character.
	strcpy(newItem->topic, topic);
	strcpy(newItem->channel, channel);
	strcpy(newItem->value, value);
	newItem->command = command;
	newItem->flags = flags;

	g_MqttPublishItemsQueued++;
	addLogAdv(LOG_INFO, LOG_FEATURE_MQTT, "Queued topic=%s/%s, %i items in queue", newItem->topic, newItem->channel, g_MqttPublishItemsQueued);
}

/// @brief Add the specified command to the last entry in the queue.
/// @param command 
void MQTT_InvokeCommandAtEnd(PostPublishCommands command) {
	MqttPublishItem_t* tail = get_queue_tail(g_MqttPublishQueueHead);
	if (tail == NULL){
		addLogAdv(LOG_ERROR, LOG_FEATURE_MQTT, "InvokeCommandAtEnd invoked but queue is empty");
	}
	else {
		tail->command = command;
	}
}

/// @brief Queue an entry for publish.
/// @param topic 
/// @param channel 
/// @param value 
/// @param flags
void MQTT_QueuePublish(const char* topic, const char* channel, const char* value, int flags) {
	MQTT_QueuePublishWithCommand(topic, channel, value, flags, None);
}


/// @brief Publish MQTT_QUEUED_ITEMS_PUBLISHED_AT_ONCE queued items.
/// @return 
OBK_Publish_Result PublishQueuedItems() {
	OBK_Publish_Result result = OBK_PUBLISH_WAS_NOT_REQUIRED;

	int count = 0;
	MqttPublishItem_t* head = g_MqttPublishQueueHead;

	//The next actionable item might not be at the front. The queue size is limited to MQTT_QUEUED_ITEMS_PUBLISHED_AT_ONCE
	//so this traversal is fast.
	//addLogAdv(LOG_INFO,LOG_FEATURE_MQTT,"PublishQueuedItems g_MqttPublishItemsQueued=%i",g_MqttPublishItemsQueued );
	while ((head != NULL) && (count < MQTT_QUEUED_ITEMS_PUBLISHED_AT_ONCE) && (g_MqttPublishItemsQueued > 0)) {
		if (!MQTT_QUEUE_ITEM_IS_REUSABLE(head)) {  //Skip reusable entries
			count++;
			result = MQTT_PublishTopicToClient(mqtt_client, head->topic, head->channel, head->value, head->flags, false);
			MQTT_QUEUE_ITEM_SET_REUSABLE(head); //Flag item as reusable
			g_MqttPublishItemsQueued--;   //decrement queued count

			//Stop if last publish failed
			if (result != OBK_PUBLISH_OK) break;

			switch (head->command) {
			case None:
				break;
			case PublishAll:
				MQTT_PublishWholeDeviceState_Internal(true);
				break;
			case PublishChannels:
				MQTT_PublishOnlyDeviceChannelsIfPossible();
				break;
			}
		}
		else {
			//addLogAdv(LOG_INFO,LOG_FEATURE_MQTT,"PublishQueuedItems item skipped reusable");
		}

		head = head->next;
	}

	return result;
}


/// @brief Is MQTT sub system ready and connected?
/// @return 
bool MQTT_IsReady() {
	int res = 0;
	if (mqtt_client){
		LOCK_TCPIP_CORE();
		res = mqtt_client_is_connected(mqtt_client);
		UNLOCK_TCPIP_CORE();
	}
	return mqtt_client && res;
}

#endif // ENABLE_MQTT
#if MQTT_USE_TLS
#ifdef MBEDTLS_ENTROPY_HARDWARE_ALT
#include "fake_clock_pub.h"
#include "mbedtls/error.h"
int mbedtls_hardware_poll(void* data, unsigned char* output, size_t len, size_t* olen) {
	int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
	((void)data);
	*olen = 0;

	if (len < sizeof(unsigned char)) {
		return 0;
	}

	if (output) {
		srand(fclk_get_second());
		for (size_t n = 0; n < len; n++) {
			output[n] = rand() % 255;
		}
		*olen = len;
		ret = 0;
	}
	return ret;
}
int mbedtls_hardclock_poll(void* data, unsigned char* output, size_t len, size_t* olen) {
	return mbedtls_hardware_poll(data, output, len, olen);
}
#endif  //MBEDTLS_ENTROPY_HARDWARE_ALT

#ifdef MBEDTLS_PLATFORM_GMTIME_R_ALT
static bool log_gmtime_alt = true;
struct tm* cvt_date(char const* date, char const* time, struct tm* t);
struct tm* cvt_date(char const* date, char const* time, struct tm* t)
{
	char s_month[5];
	int year;
	static const char month_names[] = "JanFebMarAprMayJunJulAugSepOctNovDec";
	sscanf(date, "%s %d %d", s_month, &t->tm_mday, &year);
	sscanf(time, "%2d %*c %2d %*c %2d", &t->tm_hour, &t->tm_min, &t->tm_sec);
	// Find where is s_month in month_names. Deduce month value.
	t->tm_mon = (strstr(month_names, s_month) - month_names) / 3;
	t->tm_year = year - 1900;
	return t;
}
struct tm* mbedtls_platform_gmtime_r(const mbedtls_time_t* tt, struct tm* tm_buf) {
	// If NTP time not synced return compile time
	struct tm* ltm;
	if (!NTP_IsTimeSynced()) {	
		ltm = cvt_date(__DATE__, __TIME__, tm_buf);
		if (log_gmtime_alt) {
			addLogAdv(LOG_INFO, LOG_FEATURE_NTP, "MBEDTLS: NTP not synchronized. Using compile time: %04d/%02d/%02d %02d:%02d:%02d",
				ltm->tm_year + 1900, ltm->tm_mon + 1, ltm->tm_mday, ltm->tm_hour, ltm->tm_min, ltm->tm_sec);
			log_gmtime_alt = false; 
		}			
		return ltm;
	}
	time_t ntpTime;
	ntpTime=(time_t)TIME_GetCurrentTime();
	return gmtime_r((time_t*)&ntpTime, tm_buf);
}
#endif  //MBEDTLS_PLATFORM_GMTIME_R_ALT


#if ALTCP_MBEDTLS_DEBUG
static int mbedtls_verify_cb(void* data, mbedtls_x509_crt* crt, int depth, uint32_t* flags)
{
	((void)data);
	char buf[1024];

	addLogAdv(LOG_DEBUG, LOG_FEATURE_MQTT, "Verify requested for (Depth% d) : \n", depth);
	mbedtls_x509_crt_info(buf, sizeof(buf) - 1, "", crt);
	addLogAdv(LOG_DEBUG, LOG_FEATURE_MQTT, "\n%s", buf);

	if ((*flags) == 0) {
		addLogAdv(LOG_DEBUG, LOG_FEATURE_MQTT, "  This certificate has no flags\n");
	}
	else {
		mbedtls_x509_crt_verify_info(buf, sizeof(buf), "  ! ", *flags);
		addLogAdv(LOG_DEBUG, LOG_FEATURE_MQTT, "%s\n", buf);
	}
	return 0;
}

static void mbedtls_debug_cb(void* ctx, int level, const char* file, int line, const char* str)
{
	const char* p, * basename;
	(void)ctx;

	if (level == 2)
		return;

	/* Extract basename from file */
	for (p = basename = file; *p != '\0'; p++) {
		if (*p == '/' || *p == '\\') {
			basename = p + 1;
		}
	}

	addLogAdv(LOG_ERROR, LOG_FEATURE_MQTT, "%s:%04d: |%d| %s", basename, line, level, str);
}

void mbedtls_dump_conf(mbedtls_ssl_config* conf, mbedtls_ssl_context* ssl) {
	if (ssl && ssl->handshake) {
		addLogAdv(LOG_DEBUG, LOG_FEATURE_MQTT, "HANDSHAKE CIPHER SUITE: %s", ssl->handshake->ciphersuite_info->name);
		switch (ssl->handshake->ciphersuite_info->key_exchange)
		{
			case MBEDTLS_KEY_EXCHANGE_NONE: 
				addLogAdv(LOG_DEBUG, LOG_FEATURE_MQTT, "HANDSHAKE KEY EXCHANGE: MBEDTLS_KEY_EXCHANGE_NONE");
				break;
			case MBEDTLS_KEY_EXCHANGE_RSA: 
				addLogAdv(LOG_DEBUG, LOG_FEATURE_MQTT, "HANDSHAKE KEY EXCHANGE: MBEDTLS_KEY_EXCHANGE_RSA");
				break;
			case MBEDTLS_KEY_EXCHANGE_DHE_RSA: 
				addLogAdv(LOG_DEBUG, LOG_FEATURE_MQTT, "HANDSHAKE KEY EXCHANGE: MBEDTLS_KEY_EXCHANGE_DHE_RSA");
				break;
			case MBEDTLS_KEY_EXCHANGE_ECDHE_RSA: 
				addLogAdv(LOG_DEBUG, LOG_FEATURE_MQTT, "HANDSHAKE KEY EXCHANGE: MBEDTLS_KEY_EXCHANGE_ECDHE_RSA");
				break;
			case MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA: 
				addLogAdv(LOG_DEBUG, LOG_FEATURE_MQTT, "HANDSHAKE KEY EXCHANGE: MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA");
				break;
			case MBEDTLS_KEY_EXCHANGE_PSK: 
				addLogAdv(LOG_DEBUG, LOG_FEATURE_MQTT, "HANDSHAKE KEY EXCHANGE: MBEDTLS_KEY_EXCHANGE_PSK");
				break;
			case MBEDTLS_KEY_EXCHANGE_DHE_PSK: 
				addLogAdv(LOG_DEBUG, LOG_FEATURE_MQTT, "HANDSHAKE KEY EXCHANGE: MBEDTLS_KEY_EXCHANGE_DHE_PSK");
				break;
			case MBEDTLS_KEY_EXCHANGE_RSA_PSK: 
				addLogAdv(LOG_DEBUG, LOG_FEATURE_MQTT, "HANDSHAKE KEY EXCHANGE: MBEDTLS_KEY_EXCHANGE_RSA_PSK");
				break;
			case MBEDTLS_KEY_EXCHANGE_ECDHE_PSK: 
				addLogAdv(LOG_DEBUG, LOG_FEATURE_MQTT, "HANDSHAKE KEY EXCHANGE: MBEDTLS_KEY_EXCHANGE_ECDHE_PSK");
				break;
			case MBEDTLS_KEY_EXCHANGE_ECDH_RSA: 
				addLogAdv(LOG_DEBUG, LOG_FEATURE_MQTT, "HANDSHAKE KEY EXCHANGE: MBEDTLS_KEY_EXCHANGE_ECDH_RSA");
				break;
			case MBEDTLS_KEY_EXCHANGE_ECDH_ECDSA: 
				addLogAdv(LOG_DEBUG, LOG_FEATURE_MQTT, "HANDSHAKE KEY EXCHANGE: MBEDTLS_KEY_EXCHANGE_ECDH_ECDSA");
				break;
			case MBEDTLS_KEY_EXCHANGE_ECJPAKE: 
				addLogAdv(LOG_DEBUG, LOG_FEATURE_MQTT, "HANDSHAKE KEY EXCHANGE: MBEDTLS_KEY_EXCHANGE_ECJPAKE");
				break;
		}
	}
	
	if (conf) {
		addLogAdv(LOG_DEBUG, LOG_FEATURE_MQTT, "AVAILABLE CIPHERS:");
		int len = sizeof(conf->ciphersuite_list) / (sizeof(conf->ciphersuite_list[0]));
		for (int s = 0; s < len; s++) {
			addLogAdv(LOG_DEBUG, LOG_FEATURE_MQTT, "     %s",
				mbedtls_ssl_get_ciphersuite_name(*conf->ciphersuite_list[s]));
		}
		addLogAdv(LOG_DEBUG, LOG_FEATURE_MQTT, "AVAILABLE CURVES:");
		len = sizeof(conf->curve_list) / (sizeof(mbedtls_ecp_group_id));
		const mbedtls_ecp_group_id* c = conf->curve_list;
		for (; *c; c++) {
			switch (*c)
			{
			case MBEDTLS_ECP_DP_NONE: 
				addLogAdv(LOG_DEBUG, LOG_FEATURE_MQTT, "     MBEDTLS_ECP_DP_NONE");
				break;
			case MBEDTLS_ECP_DP_SECP192R1: 
				addLogAdv(LOG_DEBUG, LOG_FEATURE_MQTT, "     MBEDTLS_ECP_DP_SECP192R1");
				break;
			case MBEDTLS_ECP_DP_SECP224R1: 
				addLogAdv(LOG_DEBUG, LOG_FEATURE_MQTT, "     MBEDTLS_ECP_DP_SECP224R1");
				break;
			case MBEDTLS_ECP_DP_SECP256R1: 
				addLogAdv(LOG_DEBUG, LOG_FEATURE_MQTT, "     MBEDTLS_ECP_DP_SECP256R1");
				break;
			case MBEDTLS_ECP_DP_SECP384R1: 
				addLogAdv(LOG_DEBUG, LOG_FEATURE_MQTT, "     MBEDTLS_ECP_DP_SECP384R1");
				break;
			case MBEDTLS_ECP_DP_SECP521R1: 
				addLogAdv(LOG_DEBUG, LOG_FEATURE_MQTT, "     MBEDTLS_ECP_DP_SECP521R1");
				break;
			case MBEDTLS_ECP_DP_BP256R1: 
				addLogAdv(LOG_DEBUG, LOG_FEATURE_MQTT, "     MBEDTLS_ECP_DP_BP256R1");
				break;
			case MBEDTLS_ECP_DP_BP384R1: 
				addLogAdv(LOG_DEBUG, LOG_FEATURE_MQTT, "     MBEDTLS_ECP_DP_BP384R1");
				break;
			case MBEDTLS_ECP_DP_BP512R1: 
				addLogAdv(LOG_DEBUG, LOG_FEATURE_MQTT, "     MBEDTLS_ECP_DP_BP512R1");
				break;
			case MBEDTLS_ECP_DP_CURVE25519: 
				addLogAdv(LOG_DEBUG, LOG_FEATURE_MQTT, "     MBEDTLS_ECP_DP_CURVE25519");
				break;
			case MBEDTLS_ECP_DP_SECP192K1: 
				addLogAdv(LOG_DEBUG, LOG_FEATURE_MQTT, "     MBEDTLS_ECP_DP_SECP192K1");
				break;
			case MBEDTLS_ECP_DP_SECP224K1: 
				addLogAdv(LOG_DEBUG, LOG_FEATURE_MQTT, "     MBEDTLS_ECP_DP_SECP224K1");
				break;
			case MBEDTLS_ECP_DP_SECP256K1: 
				addLogAdv(LOG_DEBUG, LOG_FEATURE_MQTT, "     MBEDTLS_ECP_DP_SECP256K1");
				break;
			case MBEDTLS_ECP_DP_CURVE448: 
				addLogAdv(LOG_DEBUG, LOG_FEATURE_MQTT, "     MBEDTLS_ECP_DP_CURVE448");
				break;
			}
		}
	}
}
#endif  //ALTCP_MBEDTLS_DEBUG
#endif  //MQTT_USE_TLS
