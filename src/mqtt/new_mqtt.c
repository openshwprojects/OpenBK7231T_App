
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

#ifndef LWIP_MQTT_EXAMPLE_IPADDR_INIT
#if LWIP_IPV4
#define LWIP_MQTT_EXAMPLE_IPADDR_INIT = IPADDR4_INIT(PP_HTONL(IPADDR_LOOPBACK))
#else
#define LWIP_MQTT_EXAMPLE_IPADDR_INIT
#endif
#endif

int wal_stricmp(const char *a, const char *b) {
  int ca, cb;
  do {
     ca = (unsigned char) *a++;
     cb = (unsigned char) *b++;
     ca = tolower(toupper(ca));
     cb = tolower(toupper(cb));
   } while ((ca == cb) && (ca != '\0'));
   return ca - cb;
}
int wal_strnicmp(const char *a, const char *b, int count) {
  int ca, cb;
  do {
     ca = (unsigned char) *a++;
     cb = (unsigned char) *b++;
     ca = tolower(toupper(ca));
     cb = tolower(toupper(cb));
     count--;
   } while ((ca == cb) && (ca != '\0') && (count > 0));
   return ca - cb;
}

#define MQTT_QUEUE_ITEM_IS_REUSABLE(x)  (x->topic[0] == 0)
#define MQTT_QUEUE_ITEM_SET_REUSABLE(x) (x->topic[0] = 0)

MqttPublishItem_t *g_MqttPublishQueueHead = NULL;
int g_MqttPublishItemsQueued = 0;   //Items in the queue waiting to be published. This is not the queue length.
OBK_Publish_Result PublishQueuedItems();

// from mqtt.c
extern void mqtt_disconnect(mqtt_client_t *client);

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

typedef struct mqtt_callback_tag {
    char *topic;
    char *subscriptionTopic;
    int ID;
    mqtt_callback_fn callback;
} mqtt_callback_t;

#define MAX_MQTT_CALLBACKS 32
static mqtt_callback_t *callbacks[MAX_MQTT_CALLBACKS];
static int numCallbacks = 0;
// note: only one incomming can be processed at a time.
static mqtt_request_t g_mqtt_request;

int loopsWithDisconnected = 0;
int mqtt_reconnect = 0;
// set for the device to broadcast self state on start
int g_bPublishAllStatesNow = 0;

#define PUBLISHITEM_ALL_INDEX_FIRST   -15

//These 3 values are pretty much static
#define PUBLISHITEM_SELF_STATIC_RESERVED_2		-15
#define PUBLISHITEM_SELF_STATIC_RESERVED_1		-14
#define PUBLISHITEM_SELF_HOSTNAME				-13  //Device name
#define PUBLISHITEM_SELF_BUILD					-12  //Build
#define PUBLISHITEM_SELF_MAC					  -11  //Device mac

#define PUBLISHITEM_DYNAMIC_INDEX_FIRST			-10

#define PUBLISHITEM_QUEUED_VALUES        		-10  //Publish queued items

//These values are dynamic
#define PUBLISHITEM_SELF_DYNAMIC_LIGHTSTATE		-9
#define PUBLISHITEM_SELF_DYNAMIC_LIGHTMODE		-8
#define PUBLISHITEM_SELF_DYNAMIC_DIMMER			-7
#define PUBLISHITEM_SELF_DATETIME				-6  //Current unix datetime
#define PUBLISHITEM_SELF_SOCKETS				-5  //Active sockets
#define PUBLISHITEM_SELF_RSSI					-4  //Link strength
#define PUBLISHITEM_SELF_UPTIME					-3  //Uptime
#define PUBLISHITEM_SELF_FREEHEAP				-2  //Free heap
#define PUBLISHITEM_SELF_IP						-1  //ip address

int g_publishItemIndex = PUBLISHITEM_ALL_INDEX_FIRST;
static bool g_firstFullBroadcast = true;  //Flag indicating that we need to do a full broadcast

int g_memoryErrorsThisSession = 0;
static SemaphoreHandle_t g_mutex = 0;

static bool MQTT_Mutex_Take(int del) {
	int taken;

	if(g_mutex == 0)
	{
		g_mutex = xSemaphoreCreateMutex( );
	}
    taken = xSemaphoreTake( g_mutex, del );
    if (taken == pdTRUE) {
		return true;
	}
	return false;
}
static void MQTT_Mutex_Free() 
{
	xSemaphoreGive( g_mutex );
}

void MQTT_PublishWholeDeviceState_Internal(bool bAll) 
{
  g_bPublishAllStatesNow = 1;
  if(bAll) {
	g_publishItemIndex = PUBLISHITEM_ALL_INDEX_FIRST;
  } else {
	g_publishItemIndex = PUBLISHITEM_DYNAMIC_INDEX_FIRST;
  }
}

void MQTT_PublishWholeDeviceState() 
{
  //Publish all status items once. Publish only dynamic items after that.
	MQTT_PublishWholeDeviceState_Internal(g_firstFullBroadcast);
}

void MQTT_PublishOnlyDeviceChannelsIfPossible() 
{
  if(g_bPublishAllStatesNow == 1)
    return;
  g_bPublishAllStatesNow = 1;
  //Start with channels
  g_publishItemIndex = 0;
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
  0,    /* will_qos */
  0     /* will_retain */
#if LWIP_ALTCP && LWIP_ALTCP_TLS
  , NULL
#endif
};

// channel set callback
int channelSet(mqtt_request_t* request);
static void MQTT_do_connect(mqtt_client_t *client);
static void mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status);

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

const char *get_error_name(int err)
{
    switch(err)
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

char *MQTT_GetStatusMessage(void)
{
    return mqtt_status_message;
}

// this can REPLACE callbacks, since we MAY wish to change the root topic....
// in which case we would re-resigster all callbacks?
int MQTT_RegisterCallback( const char *basetopic, const char *subscriptiontopic, int ID, mqtt_callback_fn callback){
  int index;
  int i;
  int subscribechange = 0;
	if (!basetopic || !subscriptiontopic || !callback){
		return -1;
	}
  addLogAdv(LOG_INFO,LOG_FEATURE_MQTT,"MQTT_RegisterCallback called for bT %s subT %s", basetopic, subscriptiontopic);

  // find existing to replace
  for (index = 0; index < numCallbacks; index++){
    if (callbacks[index]){
      if (callbacks[index]->ID == ID){
        break;
      }
    }
  }

  // find empty if any (empty by MQTT_RemoveCallback)
  if (index == numCallbacks){
    for (index = 0; index < numCallbacks; index++){
      if (!callbacks[index]){
        break;
      }
    }
  }

	if (index >= MAX_MQTT_CALLBACKS){
		return -4;
	}
  if (!callbacks[index]){
	  callbacks[index] = (mqtt_callback_t*)os_malloc(sizeof(mqtt_callback_t));
	  if(callbacks[index]!=0) {
		memset(callbacks[index],0,sizeof(mqtt_callback_t));
	  }
  }
	if (!callbacks[index]){
		return -2;
	}
  if (!callbacks[index]->topic || strcmp(callbacks[index]->topic, basetopic)){
    if (callbacks[index]->topic){
      os_free(callbacks[index]->topic);
    }
    callbacks[index]->topic = (char *)os_malloc(strlen(basetopic)+1);
    if (!callbacks[index]->topic){
      os_free(callbacks[index]);
      return -3;
    }
    strcpy(callbacks[index]->topic, basetopic);
  }

  if (!callbacks[index]->subscriptionTopic || strcmp(callbacks[index]->subscriptionTopic, subscriptiontopic)){
    if (callbacks[index]->subscriptionTopic){
      os_free(callbacks[index]->subscriptionTopic);
    }
    callbacks[index]->subscriptionTopic = (char *)os_malloc(strlen(subscriptiontopic)+1);
    callbacks[index]->subscriptionTopic[0] = '\0';
    if (!callbacks[index]->subscriptionTopic){
      os_free(callbacks[index]->topic);
      os_free(callbacks[index]);
      return -3;
    }

    // find out if this subscription is new.
    for (i = 0; i < numCallbacks; i++){
      if (callbacks[i]){
        if (callbacks[i]->subscriptionTopic &&
            !strcmp(callbacks[i]->subscriptionTopic, subscriptiontopic)){
          break;
        }
      }
    }
    strcpy(callbacks[index]->subscriptionTopic, subscriptiontopic);
    // if this subscription is new, must reconnect
    if (i == numCallbacks){
      subscribechange++;
    }
  }

	callbacks[index]->callback = callback;
  if (index == numCallbacks){
	  numCallbacks++;
  }

  if (subscribechange){
    mqtt_reconnect = 8;
  }
	// success
	return 0;
}

int MQTT_RemoveCallback(int ID){
  int index;

  for (index = 0; index < numCallbacks; index++){
    if (callbacks[index]){
      if (callbacks[index]->ID == ID){
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
        mqtt_reconnect = 8;
        return 1;
      }
    }
  }
  return 0;
}

// this accepts obkXXXXXX/<chan>/set to receive data to set channels
int channelSet(mqtt_request_t* request){
  // we only need a few bytes to receive a decimal number 0-100
  char copy[12];
  int len = request->receivedLen;
  char *p = request->topic;
  int channel = 0;
  int iValue = 0;

  addLogAdv(LOG_INFO,LOG_FEATURE_MQTT,"channelSet topic %i", request->topic);

  // TODO: better
  while(*p != '/') {
    if(*p == 0)
      return 0;
    p++;
  }
  p++;
  addLogAdv(LOG_INFO,LOG_FEATURE_MQTT,"channelSet part topic %s", p);

  if ((*p - '0' >= 0) && (*p - '0' <= 9)){
    channel = atoi(p);
  } else {
    channel = -1;
  }

  addLogAdv(LOG_INFO,LOG_FEATURE_MQTT,"channelSet channel %i", channel);

  // if channel out of range, stop here.
  if ((channel < 0) || (channel > 32)){
    return 0;
  }

  // find something after channel - should be <base>/<chan>/set
  while(*p != '/') {
    if(*p == 0)
      return 0;
    p++;
  }
  p++;

  // if not /set, then stop here
  if (strcmp(p, "set")){
    addLogAdv(LOG_INFO,LOG_FEATURE_MQTT,"channelSet NOT 'set'");
    return 0;
  }

  if(len > sizeof(copy)-1) {
    len = sizeof(copy)-1;
  }

  strncpy(copy, (char *)request->received, len);
  // strncpy does not terminate??!!!!
  copy[len] = '\0';

  addLogAdv(LOG_INFO,LOG_FEATURE_MQTT,"MQTT client in mqtt_incoming_data_cb data is %s for ch %i\n", copy, channel);

  iValue = atoi((char *)copy);
  CHANNEL_Set(channel,iValue,0);

  // return 1 to stop processing callbacks here.
  // return 0 to allow later callbacks to process this topic.
  return 1;
}


// this accepts cmnd/<clientId>/<xxx> to receive data to set channels
int tasCmnd(mqtt_request_t* request){
  // we only need a few bytes to receive a decimal number 0-100
  char copy[64];
  int len = request->receivedLen;
  const char *p = request->topic;

  // assume a string input here, copy and terminate
  if(len > sizeof(copy)-1) {
    len = sizeof(copy)-1;
  }
  strncpy(copy, (char *)request->received, len);
  // strncpy does not terminate??!!!!
  copy[len] = '\0';

  // TODO: better
  // skip to after second forward slash
  while(*p != '/') { if(*p == 0) return 0; p++; }
  p++;
  while(*p != '/') { if(*p == 0) return 0; p++; }
  p++;

  // use command executor....
  CMD_ExecuteCommandArgs(p, copy, COMMAND_FLAG_SOURCE_MQTT);

  // return 1 to stop processing callbacks here.
  // return 0 to allow later callbacks to process this topic.
  return 1;
}

//void MQTT_GetStats(int *outUsed, int *outMax, int *outFreeMem) {
//	mqtt_get_request_stats(mqtt_client,outUsed, outMax,outFreeMem);
//}

// copied here because for some reason renames in sdk?
static void MQTT_disconnect(mqtt_client_t *client)
{
  if (!client)
	  return;
  // this is what it was renamed to.  why?
  mqtt_disconnect(client);
}

/* Called when publish is complete either with sucess or failure */
static void mqtt_pub_request_cb(void *arg, err_t result)
{
  if(result != ERR_OK) 
  {
    addLogAdv(LOG_INFO,LOG_FEATURE_MQTT,"Publish result: %d(%s)\n", result, get_error_name(result));
    mqtt_publish_errors++;
  }
}

// This publishes value to the specified topic/channel.
static OBK_Publish_Result MQTT_PublishTopicToClient(mqtt_client_t *client, const char *sTopic, const char *sChannel, const char *sVal, int flags, bool appendGet)
{
  err_t err;
  u8_t qos = 2; /* 0 1 or 2, see MQTT specification */
  u8_t retain = 0; /* No don't retain such crappy payload... */

  if(client==0)
    return OBK_PUBLISH_WAS_DISCONNECTED;

  
  if(flags & OBK_PUBLISH_FLAG_MUTEX_SILENT) 
  {
	if(MQTT_Mutex_Take(100)==0) 
    {
	  return OBK_PUBLISH_MUTEX_FAIL;
	}
  } else {
	if(MQTT_Mutex_Take(500)==0) 
    {
      addLogAdv(LOG_ERROR,LOG_FEATURE_MQTT,"MQTT_PublishTopicToClient: mutex failed for %s=%s\r\n", sChannel, sVal);
	  return OBK_PUBLISH_MUTEX_FAIL;
	}
  }
  if(flags & OBK_PUBLISH_FLAG_RETAIN) 
  {
	retain = 1;
  }
  // global tool
  if(CFG_HasFlag(OBK_FLAG_MQTT_ALWAYSSETRETAIN)) 
  {
    retain = 1;
  }

  if(mqtt_client_is_connected(client)==0) 
  {
    g_my_reconnect_mqtt_after_time = 5;
    MQTT_Mutex_Free();
	return OBK_PUBLISH_WAS_DISCONNECTED;
  }

  g_timeSinceLastMQTTPublish = 0;
  
  char *pub_topic = (char *)os_malloc(strlen(sTopic) + 1 + strlen(sChannel) + 5 + 1); //5 for /get
  if (pub_topic != NULL)
  {
    sprintf(pub_topic, "%s/%s%s", sTopic, sChannel, (appendGet == true ? "/get" : ""));
    addLogAdv(LOG_INFO,LOG_FEATURE_MQTT,"Publishing val %s to %s retain=%i\n",sVal,pub_topic, retain);

    err = mqtt_publish(client, pub_topic, sVal, strlen(sVal), qos, retain, mqtt_pub_request_cb, 0);
    os_free(pub_topic);

    if(err != ERR_OK) 
    {
	  if(err == ERR_CONN) 
      {
		addLogAdv(LOG_INFO,LOG_FEATURE_MQTT,"Publish err: ERR_CONN aka %d\n", err);
	  } else if(err == ERR_MEM) {
		addLogAdv(LOG_INFO,LOG_FEATURE_MQTT,"Publish err: ERR_MEM aka %d\n", err);
		g_memoryErrorsThisSession ++;
	  } else {
		addLogAdv(LOG_INFO,LOG_FEATURE_MQTT,"Publish err: %d\n", err);
	  }
    }
    mqtt_published_events++;
	MQTT_Mutex_Free();
	return OBK_PUBLISH_OK;
  } else {
    MQTT_Mutex_Free();
    return OBK_PUBLISH_MEM_FAIL;
  }
}

// This is used to publish channel values in "obk0696FB33/1/get" format with numerical value,
// This is also used to publish custom information with string name,
// for example, "obk0696FB33/voltage/get" is used to publish voltage from the sensor
static OBK_Publish_Result MQTT_PublishMain(mqtt_client_t *client, const char *sChannel, const char *sVal, int flags, bool appendGet)
{
  return MQTT_PublishTopicToClient(mqtt_client, CFG_GetMQTTClientId(), sChannel, sVal, flags, appendGet);
}

/// @brief Publish a MQTT message immediately.
/// @param sTopic 
/// @param sChannel 
/// @param sVal 
/// @param flags
/// @return 
OBK_Publish_Result MQTT_Publish(char *sTopic, char *sChannel, char *sVal, int flags)
{
  return MQTT_PublishTopicToClient(mqtt_client, sTopic, sChannel, sVal, flags, false);
}

void MQTT_OBK_Printf( char *s) {
		addLogAdv(LOG_INFO,LOG_FEATURE_MQTT,s);
}

static void mqtt_incoming_data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags)
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

    addLogAdv(LOG_INFO,LOG_FEATURE_MQTT,"MQTT in topic %s", g_mqtt_request.topic);
    mqtt_received_events++;

    for (i = 0; i < numCallbacks; i++)
    {
      char *cbtopic = callbacks[i]->topic;
      if (!strncmp(g_mqtt_request.topic, cbtopic, strlen(cbtopic)))
      {
        // note - callback must return 1 to say it ate the mqtt, else further processing can be performed.
        // i.e. multiple people can get each topic if required.
        if (callbacks[i]->callback(&g_mqtt_request))
        {
          return;
        }
      }
    }
  }
}

static void mqtt_incoming_publish_cb(void *arg, const char *topic, u32_t tot_len)
{
  //const char *p;
  int i;
  // unused - left here as example
  //const struct mqtt_connect_client_info_t* client_info = (const struct mqtt_connect_client_info_t*)arg;

  // look for a callback with this URL and method, or HTTP_ANY
  g_mqtt_request.topic[0] = '\0';
  for (i = 0; i < numCallbacks; i++)
  {
    char *cbtopic = callbacks[i]->topic;
    if (strncmp(topic, cbtopic, strlen(cbtopic)))
    {
      strncpy(g_mqtt_request.topic, topic, sizeof(g_mqtt_request.topic) - 1);
      g_mqtt_request.topic[sizeof(g_mqtt_request.topic) - 1] = 0;
      break;
    }
  }
  addLogAdv(LOG_INFO,LOG_FEATURE_MQTT,"MQTT client in mqtt_incoming_publish_cb topic %s\n",topic);
}

static void
mqtt_request_cb(void *arg, err_t err)
{
  const struct mqtt_connect_client_info_t* client_info = (const struct mqtt_connect_client_info_t*)arg;

  addLogAdv(LOG_INFO,LOG_FEATURE_MQTT,"MQTT client \"%s\" request cb: err %d\n", client_info->client_id, (int)err);
}

static void mqtt_sub_request_cb(void *arg, err_t result)
{
  /* Just print the result code here for simplicity,
     normal behaviour would be to take some action if subscribe fails like
     notifying user, retry subscribe or disconnect from server */
    addLogAdv(LOG_INFO,LOG_FEATURE_MQTT,"Subscribe result: %i\n", result);
}

static void mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status)
{
  int i;
	char tmp[CGF_MQTT_CLIENT_ID_SIZE + 16];
	const char *clientId;
  err_t err = ERR_OK;
  const struct mqtt_connect_client_info_t* client_info = (const struct mqtt_connect_client_info_t*)arg;
  LWIP_UNUSED_ARG(client);

//   addLogAdv(LOG_INFO,LOG_FEATURE_MQTT,"MQTT client < removed name > connection cb: status %d\n",  (int)status);
 //  addLogAdv(LOG_INFO,LOG_FEATURE_MQTT,"MQTT client \"%s\" connection cb: status %d\n", client_info->client_id, (int)status);

  if (status == MQTT_CONNECT_ACCEPTED) 
  {
    addLogAdv(LOG_INFO,LOG_FEATURE_MQTT,"mqtt_connection_cb: Successfully connected\n");

    mqtt_set_inpub_callback(mqtt_client,
          mqtt_incoming_publish_cb,
          mqtt_incoming_data_cb,
          LWIP_CONST_CAST(void*, &mqtt_client_info));

    // subscribe to all callback subscription topics
    // this makes a BIG assumption that we can subscribe multiple times to the same one?
    // TODO - check that subscribing multiple times to the same topic is not BAD
    for (i = 0; i < numCallbacks; i++){
      if (callbacks[i]){
        if (callbacks[i]->subscriptionTopic && callbacks[i]->subscriptionTopic[0]){
          err = mqtt_sub_unsub(client,
            callbacks[i]->subscriptionTopic, 1,
            mqtt_request_cb, LWIP_CONST_CAST(void*, client_info),
            1);
          if(err != ERR_OK) {
            addLogAdv(LOG_INFO,LOG_FEATURE_MQTT,"mqtt_subscribe to %s return: %d\n", callbacks[i]->subscriptionTopic, err);
          } else {
            addLogAdv(LOG_INFO,LOG_FEATURE_MQTT,"mqtt_subscribed to %s\n", callbacks[i]->subscriptionTopic);
          }
        }
      }
    }

  	clientId = CFG_GetMQTTClientId();

    sprintf(tmp,"%s/connected",clientId);
    err = mqtt_publish(client, tmp, "online", strlen("online"), 2, true, mqtt_pub_request_cb, 0);
    if(err != ERR_OK) {
      addLogAdv(LOG_INFO,LOG_FEATURE_MQTT,"Publish err: %d\n", err);
      if(err == ERR_CONN) {
      // g_my_reconnect_mqtt_after_time = 5;
      }
    }

	// publish all values on state
	if(CFG_HasFlag(OBK_FLAG_MQTT_BROADCASTSELFSTATEONCONNECT)) {
		MQTT_PublishWholeDeviceState();
	} else {
		//MQTT_PublishOnlyDeviceChannelsIfPossible();
	}

    //mqtt_sub_unsub(client,
    //        "topic_qos1", 1,
    //        mqtt_request_cb, LWIP_CONST_CAST(void*, client_info),
    //        1);
    //mqtt_sub_unsub(client,
    //        "topic_qos0", 0,
    //        mqtt_request_cb, LWIP_CONST_CAST(void*, client_info),
    //        1);
  } else {
    addLogAdv(LOG_INFO,LOG_FEATURE_MQTT,"mqtt_connection_cb: Disconnected, reason: %d\n", status);
  }
}

static void MQTT_do_connect(mqtt_client_t *client)
{
  const char *mqtt_userName, *mqtt_host, *mqtt_pass, *mqtt_clientID;
  int mqtt_port;
  int res;
  struct hostent* hostEntry;
  char will_topic[32];

  mqtt_host = CFG_GetMQTTHost();

  if (!mqtt_host[0]){
    addLogAdv(LOG_INFO,LOG_FEATURE_MQTT,"mqtt_host empty, not starting mqtt\r\n");
    sprintf(mqtt_status_message, "mqtt_host empty, not starting mqtt");
    return;
  }

  mqtt_userName = CFG_GetMQTTUserName();
  mqtt_pass = CFG_GetMQTTPass();
  mqtt_clientID = CFG_GetMQTTClientId();
	mqtt_port = CFG_GetMQTTPort();

  addLogAdv(LOG_INFO,LOG_FEATURE_MQTT,"mqtt_userName %s\r\nmqtt_pass %s\r\nmqtt_clientID %s\r\nmqtt_host %s:%d\r\n",
    mqtt_userName,
    mqtt_pass,
    mqtt_clientID,
    mqtt_host,
    mqtt_port
  );

  // set pointer, there are no buffers to strcpy
  mqtt_client_info.client_id = mqtt_clientID;
  mqtt_client_info.client_pass = mqtt_pass;
  mqtt_client_info.client_user = mqtt_userName;

	sprintf(will_topic,"%s/connected",CFG_GetMQTTClientId());
  mqtt_client_info.will_topic = will_topic;
  mqtt_client_info.will_msg = "offline";
  mqtt_client_info.will_retain = true,
  mqtt_client_info.will_qos = 2,

  hostEntry = gethostbyname(mqtt_host);
  if (NULL != hostEntry){
    if (hostEntry->h_addr_list && hostEntry->h_addr_list[0]){
      int len = hostEntry->h_length;
      if (len > 4){
        addLogAdv(LOG_INFO,LOG_FEATURE_MQTT,"mqtt_host resolves to addr len > 4\r\n");
        len = 4;
      }
      memcpy(&mqtt_ip, hostEntry->h_addr_list[0], len);
    } else {
      addLogAdv(LOG_INFO,LOG_FEATURE_MQTT,"mqtt_host resolves no addresses?\r\n");
      sprintf(mqtt_status_message, "mqtt_host resolves no addresses?");
      return;
    }

    // host name/ip
    //ipaddr_aton(mqtt_host,&mqtt_ip);

    /* Initiate client and connect to server, if this fails immediately an error code is returned
      otherwise mqtt_connection_cb will be called with connection result after attempting
      to establish a connection with the server.
      For now MQTT version 3.1.1 is always used */

    res = mqtt_client_connect(mqtt_client,
            &mqtt_ip, mqtt_port,
            mqtt_connection_cb, LWIP_CONST_CAST(void*, &mqtt_client_info),
            &mqtt_client_info);
    mqtt_connect_result = res;
    if(res != ERR_OK) 
    {
      addLogAdv(LOG_INFO,LOG_FEATURE_MQTT,"Connect error in mqtt_client_connect - code: %d (%s)\n", res, get_error_name(res));
      sprintf(mqtt_status_message, "mqtt_client_connect connect failed");
      if (res == ERR_ISCONN)
      {
        mqtt_disconnect(mqtt_client);
      }
    } else {
      mqtt_status_message[0] = '\0';
    }
  } else {
    addLogAdv(LOG_INFO,LOG_FEATURE_MQTT,"mqtt_host %s not found by gethostbyname\r\n", mqtt_host);
    sprintf(mqtt_status_message, "mqtt_host %s not found by gethostbyname", mqtt_host);
  }
}

OBK_Publish_Result MQTT_PublishMain_StringInt(const char *sChannel, int iv)
{
	char valueStr[16];

	sprintf(valueStr,"%i",iv);

	return MQTT_PublishMain(mqtt_client,sChannel,valueStr, 0, true);

}
OBK_Publish_Result MQTT_PublishMain_StringFloat(const char *sChannel, float f)
{
	char valueStr[16];

	sprintf(valueStr,"%f",f);

	return MQTT_PublishMain(mqtt_client,sChannel,valueStr, 0, true);

}
OBK_Publish_Result MQTT_PublishMain_StringString(const char *sChannel, const char *valueStr, int flags)
{

	return MQTT_PublishMain(mqtt_client,sChannel,valueStr, flags, true);

}
OBK_Publish_Result MQTT_ChannelChangeCallback(int channel, int iVal)
{
	char channelNameStr[8];
	char valueStr[16];

   addLogAdv(LOG_INFO,LOG_FEATURE_MAIN, "Channel has changed! Publishing change %i with %i \n",channel,iVal);

	sprintf(channelNameStr,"%i",channel);
	sprintf(valueStr,"%i",iVal);

	return MQTT_PublishMain(mqtt_client,channelNameStr,valueStr, 0, true);
}
OBK_Publish_Result MQTT_ChannelPublish(int channel, int flags)
{
	char channelNameStr[8];
	char valueStr[16];
	int iValue;
	
	iValue = CHANNEL_Get(channel);

	addLogAdv(LOG_INFO,LOG_FEATURE_MAIN, "Forced channel publish! Publishing val %i with %i \n",channel,iValue);

	sprintf(channelNameStr,"%i",channel);
	sprintf(valueStr,"%i",iValue);

	return MQTT_PublishMain(mqtt_client,channelNameStr,valueStr, flags, true);
}
// This console command will trigger a publish of all used variables (channels and extra stuff)
OBK_Publish_Result MQTT_PublishAll(const void *context, const char *cmd, const char *args, int cmdFlags) {
	
	MQTT_PublishWholeDeviceState_Internal(false);

	return 1;// TODO make return values consistent for all console commands
}
// This console command will trigger a publish of runtime variables
OBK_Publish_Result MQTT_PublishChannels(const void *context, const char *cmd, const char *args, int cmdFlags) {
	
	MQTT_PublishWholeDeviceState_Internal(true);

	return 1;// TODO make return values consistent for all console commands
}
OBK_Publish_Result MQTT_PublishCommand(const void *context, const char *cmd, const char *args, int cmdFlags) {
	const char *topic, *value;
	OBK_Publish_Result ret;

	Tokenizer_TokenizeString(args);

	if(Tokenizer_GetArgsCount() < 2){
		addLogAdv(LOG_INFO,LOG_FEATURE_MQTT,"Publish command requires two arguments (topic and value)");
		return 0;
	}
	topic = Tokenizer_GetArg(0);
	value = Tokenizer_GetArg(1);

	ret = MQTT_PublishMain_StringString(topic,value, 0);

	return ret;
}
// initialise things MQTT
// called from user_main
void MQTT_init()
{
  char cbtopicbase[64];
  char cbtopicsub[64];
  const char *clientId;

  clientId = CFG_GetMQTTClientId();

  // register the main set channel callback
  sprintf(cbtopicbase,"%s/",clientId);
  sprintf(cbtopicsub,"%s/+/set",clientId);
  // note: this may REPLACE an existing entry with the same ID.  ID 1 !!!
  MQTT_RegisterCallback( cbtopicbase, cbtopicsub, 1, channelSet);

  // register the TAS cmnd callback
  sprintf(cbtopicbase,"cmnd/%s/",clientId);
  sprintf(cbtopicsub,"cmnd/%s/+",clientId);
  // note: this may REPLACE an existing entry with the same ID.  ID 2 !!!
  MQTT_RegisterCallback( cbtopicbase, cbtopicsub, 2, tasCmnd);

  mqtt_initialised = 1;

  CMD_RegisterCommand("publish","",MQTT_PublishCommand, "Sqqq", NULL);
  CMD_RegisterCommand("publishAll","",MQTT_PublishAll, "Sqqq", NULL);
  CMD_RegisterCommand("publishChannels","",MQTT_PublishChannels, "Sqqq", NULL);
}

OBK_Publish_Result MQTT_DoItemPublishString(const char *sChannel, const char *valueStr) 
{
  return MQTT_PublishMain(mqtt_client, sChannel, valueStr, OBK_PUBLISH_FLAG_MUTEX_SILENT, false);
}

OBK_Publish_Result MQTT_DoItemPublish(int idx) 
{
  char dataStr[3*6+1];  //This is sufficient to hold mac value

	switch(idx) {
    case PUBLISHITEM_SELF_STATIC_RESERVED_2:
    case PUBLISHITEM_SELF_STATIC_RESERVED_1:
      return OBK_PUBLISH_WAS_NOT_REQUIRED;

    case PUBLISHITEM_QUEUED_VALUES:
      return PublishQueuedItems();

	  case PUBLISHITEM_SELF_DYNAMIC_LIGHTSTATE:
      return LED_IsRunningDriver() ? LED_SendEnableAllState() : OBK_PUBLISH_WAS_NOT_REQUIRED;
    case PUBLISHITEM_SELF_DYNAMIC_LIGHTMODE:
      return LED_IsRunningDriver() ? LED_SendCurrentLightMode() : OBK_PUBLISH_WAS_NOT_REQUIRED;
    case PUBLISHITEM_SELF_DYNAMIC_DIMMER:
      return LED_IsRunningDriver() ? LED_SendDimmerChange() : OBK_PUBLISH_WAS_NOT_REQUIRED;

    case PUBLISHITEM_SELF_HOSTNAME:
      return MQTT_DoItemPublishString("host", CFG_GetShortDeviceName());

    case PUBLISHITEM_SELF_BUILD:
      return MQTT_DoItemPublishString("build", g_build_str);
      
    case PUBLISHITEM_SELF_MAC:
      return MQTT_DoItemPublishString("mac", HAL_GetMACStr(dataStr));
    
    case PUBLISHITEM_SELF_DATETIME:
      //Drivers are only built on BK7231 chips
      #ifndef OBK_DISABLE_ALL_DRIVERS
        if (DRV_IsRunning("NTP")) {
          sprintf(dataStr,"%d",NTP_GetCurrentTime());
          return MQTT_DoItemPublishString("datetime", dataStr);
        }
        else{
          return OBK_PUBLISH_WAS_NOT_REQUIRED;
        }
      #else
        return OBK_PUBLISH_WAS_NOT_REQUIRED;
      #endif

    case PUBLISHITEM_SELF_SOCKETS:
      sprintf(dataStr,"%d",LWIP_GetActiveSockets());
      return MQTT_DoItemPublishString("sockets", dataStr);

    case PUBLISHITEM_SELF_RSSI:
      sprintf(dataStr,"%d",HAL_GetWifiStrength());
      return MQTT_DoItemPublishString("rssi", dataStr);

    case PUBLISHITEM_SELF_UPTIME:
      sprintf(dataStr,"%d",Time_getUpTimeSeconds());
      return MQTT_DoItemPublishString("uptime", dataStr);

    case PUBLISHITEM_SELF_FREEHEAP:
      sprintf(dataStr,"%d",xPortGetFreeHeapSize());
      return MQTT_DoItemPublishString("freeheap", dataStr);

    case PUBLISHITEM_SELF_IP:
      g_firstFullBroadcast = false; //We published the last status item, disable full broadcast
      return MQTT_DoItemPublishString("ip", HAL_GetMyIPString());

    default:
      break;
  }
  
	// if LED driver is active, do not publish raw channel values
	if(LED_IsRunningDriver() == false && idx >= 0) {
		// This is because raw channels are like PWM values, RGBCW has 5 raw channels
		// (unless it has I2C LED driver)
		// We do not need raw values for RGBCW lights (or RGB, etc)
		// because we are using led_basecolor_rgb, led_dimmer, led_enableAll, etc
		// NOTE: negative indexes are not channels - they are special values
		if(CHANNEL_IsInUse(idx)) {
			 MQTT_ChannelPublish(g_publishItemIndex, OBK_PUBLISH_FLAG_MUTEX_SILENT);
		}
	}
	return OBK_PUBLISH_WAS_NOT_REQUIRED; // didnt publish
}
static int g_secondsBeforeNextFullBroadcast = 30;

// called from user timer.
int MQTT_RunEverySecondUpdate() 
{
	if (!mqtt_initialised)
		return 0;

    if (Main_HasWiFiConnected() == 0)
        return 0;

	// take mutex for connect and disconnect operations
	if(MQTT_Mutex_Take(100)==0) 
    {
		return 0;
	}

        // reconnect if went into MQTT library ERR_MEM forever loop
	if(g_memoryErrorsThisSession >= 5) 
    {
		addLogAdv(LOG_INFO,LOG_FEATURE_MQTT, "MQTT will reconnect soon to fix ERR_MEM errors\n");
		g_memoryErrorsThisSession = 0;
		mqtt_reconnect = 5;
	}

	// if asked to reconnect (e.g. change of topic(s))
	if (mqtt_reconnect > 0)
    {
		mqtt_reconnect --;
		if (mqtt_reconnect == 0)
        {
			// then if connected, disconnect, and then it will reconnect automatically in 2s
			if (mqtt_client && mqtt_client_is_connected(mqtt_client)) 
            {
				MQTT_disconnect(mqtt_client);
				loopsWithDisconnected = 8;
			}
		}
	}

	if(mqtt_client == 0 || mqtt_client_is_connected(mqtt_client) == 0) 
    {
		//addLogAdv(LOG_INFO,LOG_FEATURE_MAIN, "Timer discovers disconnected mqtt %i\n",loopsWithDisconnected);
		loopsWithDisconnected++;
		if(loopsWithDisconnected > 10)
		{
			if(mqtt_client == 0)
			{
				mqtt_client = mqtt_client_new();
			} else {
				#if defined(MQTT_CLIENT_CLEANUP)
				mqtt_client_cleanup(mqtt_client);
				#endif
			}
			MQTT_do_connect(mqtt_client);
			mqtt_connect_events++;
			loopsWithDisconnected = 0;
		}
		MQTT_Mutex_Free();
		return 0;
	} else {
		MQTT_Mutex_Free();
		// below mutex is not required any more

		// it is connected
		g_timeSinceLastMQTTPublish++;

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
	    else if(g_bPublishAllStatesNow) 
        {
			// Doing step by a step a full publish state
			if(g_timeSinceLastMQTTPublish > 2) 
            {
				OBK_Publish_Result publishRes;
				int g_sent_thisFrame = 0;

				while(g_publishItemIndex < CHANNEL_MAX) 
                {
					publishRes = MQTT_DoItemPublish(g_publishItemIndex);
					if(publishRes != OBK_PUBLISH_WAS_NOT_REQUIRED)
					{
						addLogAdv(LOG_INFO,LOG_FEATURE_MQTT, "[g_bPublishAllStatesNow] item %i result %i\n",g_publishItemIndex,publishRes);
					}
					// There are several things that can happen now
					// OBK_PUBLISH_OK - it was required and was published
					if(publishRes == OBK_PUBLISH_OK) 
                    {
						g_sent_thisFrame++;
						if(g_sent_thisFrame>=1)
                        {
							g_publishItemIndex++;
							break;
						}
					}
					// OBK_PUBLISH_MUTEX_FAIL - MQTT is busy
					if(publishRes == OBK_PUBLISH_MUTEX_FAIL
						|| publishRes == OBK_PUBLISH_WAS_DISCONNECTED) 
                    {
						// retry the same later
						break;
					}
					// OBK_PUBLISH_WAS_NOT_REQUIRED
					// The item is not used for this device
					g_publishItemIndex++;
				}

				if(g_publishItemIndex >= CHANNEL_MAX) 
                {
					// done
					g_bPublishAllStatesNow = 0;
				}	
			}
		} else {
			// not doing anything
			if(CFG_HasFlag(OBK_FLAG_MQTT_BROADCASTSELFSTATEPERMINUTE)) 
            {
				// this is called every second
				g_secondsBeforeNextFullBroadcast--;
				if(g_secondsBeforeNextFullBroadcast <= 0) 
                {
					g_secondsBeforeNextFullBroadcast = 60;
					MQTT_PublishWholeDeviceState();
				}
			}
		}
	}
	return 1;
}

MqttPublishItem_t *get_queue_tail(MqttPublishItem_t *head){
  if (head == NULL){ return NULL; }

  while (head->next != NULL){
    head = head->next; 
  }
  return head;
}

MqttPublishItem_t *find_queue_reusable_item(MqttPublishItem_t *head){
  while (head != NULL){
    if (MQTT_QUEUE_ITEM_IS_REUSABLE(head)){
      return head;
    }
    head = head->next;
  }
  return head;
}

/// @brief Queue an entry for publish.
/// @param topic 
/// @param channel 
/// @param value 
/// @param flags
void MQTT_QueuePublish(char *topic, char *channel, char *value, int flags){
  if (g_MqttPublishItemsQueued >= MQTT_MAX_QUEUE_SIZE){
    addLogAdv(LOG_ERROR,LOG_FEATURE_MQTT,"Unable to queue! %i items already present\r\n", g_MqttPublishItemsQueued);
    return;
  }

  if ((strlen(topic) > MQTT_PUBLISH_ITEM_TOPIC_LENGTH) ||
    (strlen(channel) > MQTT_PUBLISH_ITEM_CHANNEL_LENGTH) ||
    (strlen(value) > MQTT_PUBLISH_ITEM_VALUE_LENGTH)){
    addLogAdv(LOG_ERROR,LOG_FEATURE_MQTT,"Unable to queue! Topic (%i), channel (%i) or value (%i) exceeds size limit\r\n", 
      strlen(topic), strlen(channel), strlen(value));
    return;
  }

  //Queue data for publish. This might be a new item in the queue or an existing item. This is done to prevent
  //memory fragmentation. The total queue length is limited to MQTT_MAX_QUEUE_SIZE.
  MqttPublishItem_t *newItem;

  if (g_MqttPublishQueueHead == NULL){
    g_MqttPublishQueueHead = newItem = os_malloc(sizeof(MqttPublishItem_t));
    newItem->next=NULL;
  }
  else{
    newItem = find_queue_reusable_item(g_MqttPublishQueueHead);

    if (newItem == NULL){
      newItem = os_malloc(sizeof(MqttPublishItem_t));
      newItem->next=NULL;
      get_queue_tail(g_MqttPublishQueueHead)->next = newItem; //Append new item
    }
  }
  
  //os_strcpy does copy ending null character.
  os_strcpy(newItem->topic, topic); 
  os_strcpy(newItem->channel, channel);
  os_strcpy(newItem->value, value);
  newItem->flags = flags;
  
  g_MqttPublishItemsQueued++;
  addLogAdv(LOG_INFO,LOG_FEATURE_MQTT,"Queued topic=%s/%s %i, items queued", newItem->topic, newItem->channel, g_MqttPublishItemsQueued);
}

/// @brief Publish MQTT_QUEUED_ITEMS_PUBLISHED_AT_ONCE queued items.
/// @return 
OBK_Publish_Result PublishQueuedItems(){
  OBK_Publish_Result result = OBK_PUBLISH_WAS_NOT_REQUIRED;

  int count = 0;
  MqttPublishItem_t *head = g_MqttPublishQueueHead;
  
  //The next actionable item might not be at the front. The queue size is limited to MQTT_QUEUED_ITEMS_PUBLISHED_AT_ONCE
  //so this traversal is fast.
  //addLogAdv(LOG_INFO,LOG_FEATURE_MQTT,"PublishQueuedItems g_MqttPublishItemsQueued=%i",g_MqttPublishItemsQueued );
  while ((head != NULL) && (count < MQTT_QUEUED_ITEMS_PUBLISHED_AT_ONCE) && (g_MqttPublishItemsQueued > 0)){
    if (!MQTT_QUEUE_ITEM_IS_REUSABLE(head)){  //Skip reusable entries
      count++;
      result = MQTT_PublishTopicToClient(mqtt_client, head->topic, head->channel, head->value, head->flags, false);
      MQTT_QUEUE_ITEM_SET_REUSABLE(head); //Flag item as reusable
      g_MqttPublishItemsQueued--;   //decrement queued count

      //Stop if last publish failed
      if (result != OBK_PUBLISH_OK) break;
    }
    else{
      //addLogAdv(LOG_INFO,LOG_FEATURE_MQTT,"PublishQueuedItems item skipped reusable");
    }

    head = head->next;
  }

  return result;
}
