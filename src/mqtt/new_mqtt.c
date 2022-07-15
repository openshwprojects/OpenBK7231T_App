
#include "new_mqtt.h"
#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
#include "../logging/logging.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../hal/hal_wifi.h"



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

// from mqtt.c
extern void mqtt_disconnect(mqtt_client_t *client);

static int g_my_reconnect_mqtt_after_time = -1;
ip_addr_t mqtt_ip LWIP_MQTT_EXAMPLE_IPADDR_INIT;
mqtt_client_t* mqtt_client;

static int mqtt_initialised = 0;

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
#define PUBLISHITEM_INDEX_FIRST -1
#define PUBLISHITEM_SELF_IP -1
int g_publishItemIndex = PUBLISHITEM_INDEX_FIRST;

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
static void MQTT_Mutex_Free() {
	xSemaphoreGive( g_mutex );
}

void MQTT_PublishWholeDeviceState() {
	g_bPublishAllStatesNow = 1;
	g_publishItemIndex = PUBLISHITEM_INDEX_FIRST;
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


// this accepts cmnd/<basename>/<xxx> to receive data to set channels
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
  if (!client) return;
  // this is what it was renamed to.  why?
  mqtt_disconnect(client);
}

/* Called when publish is complete either with sucess or failure */
static void mqtt_pub_request_cb(void *arg, err_t result)
{
  if(result != ERR_OK) {
  addLogAdv(LOG_INFO,LOG_FEATURE_MQTT,"Publish result: %d\n", result);
  }
}

// This is used to publish channel values in "obk0696FB33/1/get" format with numerical value,
// This is also used to publish custom information with string name,
// for example, "obk0696FB33/voltage/get" is used to publish voltage from the sensor
static int MQTT_PublishMain(mqtt_client_t *client, const char *sChannel, const char *sVal)
{
	char pub_topic[32];
//  const char *pub_payload= "{\"temperature\": \"45.5\"}";
  err_t err;
  //int myValue;
  u8_t qos = 2; /* 0 1 or 2, see MQTT specification */
  u8_t retain = 0; /* No don't retain such crappy payload... */
	const char *baseName;



  if(client==0)
	  return 1;

  
	if(MQTT_Mutex_Take(100)==0) {
		addLogAdv(LOG_ERROR,LOG_FEATURE_MQTT,"MQTT_PublishMain: mutex failed for %s=%s\r\n", sChannel, sVal);
		return 1;
	}


  if(mqtt_client_is_connected(client)==0) {
		 g_my_reconnect_mqtt_after_time = 5;
		MQTT_Mutex_Free();
		return 1;
  }

  addLogAdv(LOG_INFO,LOG_FEATURE_MQTT,"Publishing %s = %s \n",sChannel,sVal);

	baseName = CFG_GetShortDeviceName();

	sprintf(pub_topic,"%s/%s/get",baseName,sChannel);
  err = mqtt_publish(client, pub_topic, sVal, strlen(sVal), qos, retain, mqtt_pub_request_cb, 0);
  if(err != ERR_OK) {
	 if(err == ERR_CONN) {
		addLogAdv(LOG_INFO,LOG_FEATURE_MQTT,"Publish err: ERR_CONN aka %d\n", err);
	 } else if(err == ERR_MEM) {
		addLogAdv(LOG_INFO,LOG_FEATURE_MQTT,"Publish err: ERR_MEM aka %d\n", err);
	 } else {
		addLogAdv(LOG_INFO,LOG_FEATURE_MQTT,"Publish err: %d\n", err);
	 }
  }
	MQTT_Mutex_Free();
	return 0;
}


static void mqtt_incoming_data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags)
{
  int i;
  // unused - left here as example
  //const struct mqtt_connect_client_info_t* client_info = (const struct mqtt_connect_client_info_t*)arg;

  // if we stored a topic in g_mqtt_request, then we found a matching callback, so use it.
  if (g_mqtt_request.topic[0]) {
    // note: data is NOT terminated (it may be binary...).
    g_mqtt_request.received = data;
    g_mqtt_request.receivedLen = len;

  addLogAdv(LOG_INFO,LOG_FEATURE_MQTT,"MQTT in topic %s", g_mqtt_request.topic);

    for (i = 0; i < numCallbacks; i++){
      char *cbtopic = callbacks[i]->topic;
      if (!strncmp(g_mqtt_request.topic, cbtopic, strlen(cbtopic))){
        // note - callback must return 1 to say it ate the mqtt, else further processing can be performed.
        // i.e. multiple people can get each topic if required.
        if (callbacks[i]->callback(&g_mqtt_request)){
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
	for (i = 0; i < numCallbacks; i++){
		char *cbtopic = callbacks[i]->topic;
		if (strncmp(topic, cbtopic, strlen(cbtopic))){
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
	char tmp[64];
	const char *baseName;
  err_t err = ERR_OK;
  const struct mqtt_connect_client_info_t* client_info = (const struct mqtt_connect_client_info_t*)arg;
  LWIP_UNUSED_ARG(client);

//   addLogAdv(LOG_INFO,LOG_FEATURE_MQTT,"MQTT client < removed name > connection cb: status %d\n",  (int)status);
 //  addLogAdv(LOG_INFO,LOG_FEATURE_MQTT,"MQTT client \"%s\" connection cb: status %d\n", client_info->client_id, (int)status);

  if (status == MQTT_CONNECT_ACCEPTED) {
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

  	baseName = CFG_GetShortDeviceName();

    sprintf(tmp,"%s/connected",baseName);
    err = mqtt_publish(client, tmp, "online", strlen("online"), 2, true, mqtt_pub_request_cb, 0);
    if(err != ERR_OK) {
      addLogAdv(LOG_INFO,LOG_FEATURE_MQTT,"Publish err: %d\n", err);
      if(err == ERR_CONN) {
      // g_my_reconnect_mqtt_after_time = 5;
      }
    }

	// publish all values on state
	MQTT_PublishWholeDeviceState();

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

  mqtt_userName = CFG_GetMQTTUserName();
  mqtt_pass = CFG_GetMQTTPass();
  //mqtt_clientID = CFG_GetMQTTBrokerName();
  mqtt_clientID = CFG_GetShortDeviceName();
  mqtt_host = CFG_GetMQTTHost();
	mqtt_port = CFG_GetMQTTPort();

  addLogAdv(LOG_INFO,LOG_FEATURE_MQTT,"mqtt_userName %s\r\nmqtt_pass %s\r\nmqtt_clientID %s\r\nmqtt_host %s:%d\r\n",
    mqtt_userName,
    mqtt_pass,
    mqtt_clientID,
    mqtt_host,
    mqtt_port
  );


  if (!mqtt_host[0]){
    addLogAdv(LOG_INFO,LOG_FEATURE_MQTT,"mqtt_host empty, not starting mqtt\r\n");
    return;
  }

  // set pointer, there are no buffers to strcpy
  mqtt_client_info.client_id = mqtt_clientID;
  mqtt_client_info.client_pass = mqtt_pass;
  mqtt_client_info.client_user = mqtt_userName;

	sprintf(will_topic,"%s/connected",CFG_GetShortDeviceName());
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
   if(res != ERR_OK) {
      addLogAdv(LOG_INFO,LOG_FEATURE_MQTT,"Connect error in mqtt_client_connect - code: %d\n", res);

    }

  } else {
   addLogAdv(LOG_INFO,LOG_FEATURE_MQTT,"mqtt_host %s not found by gethostbyname\r\n", mqtt_host);
  }
}
int MQTT_PublishMain_StringInt(const char *sChannel, int iv)
{
	char valueStr[16];

	sprintf(valueStr,"%i",iv);

	return MQTT_PublishMain(mqtt_client,sChannel,valueStr);

}
int MQTT_PublishMain_StringFloat(const char *sChannel, float f)
{
	char valueStr[16];

	sprintf(valueStr,"%f",f);

	return MQTT_PublishMain(mqtt_client,sChannel,valueStr);

}
int MQTT_PublishMain_StringString(const char *sChannel, const char *valueStr)
{

	return MQTT_PublishMain(mqtt_client,sChannel,valueStr);

}
int MQTT_Publish_SelfIP() {
	const char *ip;

	ip = HAL_GetMyIPString();

	return MQTT_PublishMain_StringString("IP",ip);
}
int MQTT_ChannelChangeCallback(int channel, int iVal)
{
	char channelNameStr[8];
	char valueStr[16];

   addLogAdv(LOG_INFO,LOG_FEATURE_MAIN, "Channel has changed! Publishing change %i with %i \n",channel,iVal);

	sprintf(channelNameStr,"%i",channel);
	sprintf(valueStr,"%i",iVal);

	return MQTT_PublishMain(mqtt_client,channelNameStr,valueStr);
}
int  MQTT_ChannelPublish(int channel)
{
	char channelNameStr[8];
	char valueStr[16];
	int iValue;
	
	iValue = CHANNEL_Get(channel);

	addLogAdv(LOG_INFO,LOG_FEATURE_MAIN, "Forced channel publish! Publishing val %i with %i \n",channel,iValue);

	sprintf(channelNameStr,"%i",channel);
	sprintf(valueStr,"%i",iValue);

	return MQTT_PublishMain(mqtt_client,channelNameStr,valueStr);
}
int MQTT_PublishCommand(const void *context, const char *cmd, const char *args, int cmdFlags) {
	const char *topic, *value;

	Tokenizer_TokenizeString(args);

	if(Tokenizer_GetArgsCount() < 2){
		addLogAdv(LOG_INFO,LOG_FEATURE_MQTT,"Publish command requires two arguments (topic and value)");
		return 0;
	}
	topic = Tokenizer_GetArg(0);
	value = Tokenizer_GetArg(1);

	MQTT_PublishMain_StringString(topic,value);

	return 1;
}
// initialise things MQTT
// called from user_main
void MQTT_init(){
	char cbtopicbase[64];
	char cbtopicsub[64];
  const char *baseName;
	baseName = CFG_GetShortDeviceName();

  // register the main set channel callback
	sprintf(cbtopicbase,"%s/",baseName);
  sprintf(cbtopicsub,"%s/+/set",baseName);
  // note: this may REPLACE an existing entry with the same ID.  ID 1 !!!
  MQTT_RegisterCallback( cbtopicbase, cbtopicsub, 1, channelSet);

  // register the TAS cmnd callback
	sprintf(cbtopicbase,"cmnd/%s/",baseName);
  sprintf(cbtopicsub,"cmnd/%s/+",baseName);
  // note: this may REPLACE an existing entry with the same ID.  ID 2 !!!
  MQTT_RegisterCallback( cbtopicbase, cbtopicsub, 2, tasCmnd);

  mqtt_initialised = 1;

	CMD_RegisterCommand("publish","",MQTT_PublishCommand, "Sqqq", NULL);

}

bool MQTT_DoItemPublish(int idx) {
	if(idx == PUBLISHITEM_SELF_IP) {
		MQTT_Publish_SelfIP();
		return true; //  published
	}
	if(CHANNEL_IsInUse(idx)) {
		MQTT_ChannelPublish(g_publishItemIndex);
		return true; //  published
	}
	return false; // didnt publish
}
static int g_secondsBeforeNextFullBroadcast = 30;

// called from user timer.
int MQTT_RunEverySecondUpdate() {

	if (!mqtt_initialised)
		return 0;

	if(MQTT_Mutex_Take(100)==0) {
		return 0;
	}

	// if asked to reconnect (e.g. change of topic(s))
	if (mqtt_reconnect > 0){
		mqtt_reconnect --;
		if (mqtt_reconnect == 0){
			// then if connected, disconnect, and then it will reconnect automatically in 2s
			if (mqtt_client && mqtt_client_is_connected(mqtt_client)) {
				MQTT_disconnect(mqtt_client);
				loopsWithDisconnected = 8;
			}
		}
	}

	if(mqtt_client == 0 || mqtt_client_is_connected(mqtt_client) == 0) {
		//addLogAdv(LOG_INFO,LOG_FEATURE_MAIN, "Timer discovers disconnected mqtt %i\n",loopsWithDisconnected);
		loopsWithDisconnected++;
		if(loopsWithDisconnected > 10)
		{
			if(mqtt_client == 0)
			{
				mqtt_client = mqtt_client_new();
			}
			MQTT_do_connect(mqtt_client);
			loopsWithDisconnected = 0;
		}
		MQTT_Mutex_Free();
		return 0;
	} else {
		// it is connected

		// do we want to broadcast full state?
		// Do it slowly in order not to overload the buffers
		// The item indexes start at negative values for special items
		// and then covers Channel indexes up to CHANNEL_MAX
		if(g_bPublishAllStatesNow) {
			// Doing step by a step a full publish state
			int g_sent_thisFrame = 0;

			while(g_publishItemIndex < CHANNEL_MAX) {
				if(MQTT_DoItemPublish(g_publishItemIndex)) {
					g_sent_thisFrame++;
					if(g_sent_thisFrame>=2){
						g_publishItemIndex++;
						break;
					}
				}
				g_publishItemIndex++;
			}
			if(g_publishItemIndex >= CHANNEL_MAX) {
				// done
				g_bPublishAllStatesNow = 0;
			}	
		} else {
			// not doing anything
			if(CFG_HasFlag(OBK_FLAG_MQTT_BROADCASTSELFSTATEPERMINUTE)) {
				// this is called every second
				g_secondsBeforeNextFullBroadcast--;
				if(g_secondsBeforeNextFullBroadcast <= 0) {
					g_secondsBeforeNextFullBroadcast = 60;
					MQTT_PublishWholeDeviceState();
				}
			}			
		}

	}
	MQTT_Mutex_Free();
	return 1;
}

