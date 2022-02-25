
#include "new_mqtt.h"
#include "../new_pins.h"
#include "../new_cfg.h"
#include "../logging/logging.h"


#undef os_printf
#undef PR_DEBUG
#undef PR_NOTICE
#undef Malloc
#undef Free
#define os_printf addLog
#define PR_DEBUG addLog
#define PR_NOTICE addLog
#define Malloc os_malloc
#define Free os_free


#ifndef LWIP_MQTT_EXAMPLE_IPADDR_INIT
#if LWIP_IPV4
#define LWIP_MQTT_EXAMPLE_IPADDR_INIT = IPADDR4_INIT(PP_HTONL(IPADDR_LOOPBACK))
#else
#define LWIP_MQTT_EXAMPLE_IPADDR_INIT
#endif
#endif

// from mqtt.c
extern void mqtt_disconnect_my2(mqtt_client_t *client);

static int g_my_reconnect_mqtt_after_time = -1;
ip_addr_t mqtt_ip LWIP_MQTT_EXAMPLE_IPADDR_INIT;
mqtt_client_t* mqtt_client;

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

  PR_NOTICE("channelSet topic %i", request->topic);

  // TODO: better 
  while(*p != '/') {
    if(*p == 0)
      return 0;
    p++;
  }
  p++;
  PR_NOTICE("channelSet part topic %s", p);

  if ((*p - '0' >= 0) && (*p - '0' <= 9)){
    channel = atoi(p);
  } else {
    channel = -1;
  }

  PR_NOTICE("channelSet channel %i", channel);

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
    PR_NOTICE("channelSet NOT 'set'");
    return 0;
  }

  if(len > sizeof(copy)-1) {
    len = sizeof(copy)-1;
  }

  strncpy(copy, (char *)request->received, len);
  // strncpy does not terminate??!!!!
  copy[len] = '\0';

  //PR_NOTICE("MQTT client in mqtt_incoming_data_cb\n");
  PR_NOTICE("MQTT client in mqtt_incoming_data_cb data is %s for ch %i\n", copy, channel);

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
  char *p = request->topic;
  int channel = 0;
  int iValue = 0;

  // assume a string input here, copy and terminate
  if(len > sizeof(copy)-1) {
    len = sizeof(copy)-1;
  }
  strncpy(copy, (char *)request->received, len);
  // strncpy does not terminate??!!!!
  copy[len] = '\0';

  PR_NOTICE("tas? data is %s for ch %s\n", copy, request->topic);

  // TODO: better 
  // skip to after second forward slash
  while(*p != '/') { if(*p == 0) return 0; p++; }
  p++;
  while(*p != '/') { if(*p == 0) return 0; p++; }
  p++;

  do{
    if (!strncmp(p, "POWER", 5)){
      p += 5;
      if ((*p - '0' >= 0) && (*p - '0' <= 9)){
        channel = atoi(p);
      } else {
        channel = -1;
      }
      // if channel out of range, stop here.
      if ((channel < 0) || (channel > 32))  return 0;

      //PR_NOTICE("MQTT client in mqtt_incoming_data_cb\n");
      PR_NOTICE("MQTT client in tasCmnd data is %s for ch %i\n", copy, channel);
      iValue = atoi((char *)copy);
      CHANNEL_Set(channel,iValue,0);
      break;
    }

    PR_NOTICE("MQTT client unprocessed in tasCmnd data is %s for topic \n", copy, request->topic);
    break;
  } while (0);

  // return 1 to stop processing callbacks here.
  // return 0 to allow later callbacks to process this topic.
  return 1;
}


// copied here because for some reason renames in sdk?
static void MQTT_disconnect(mqtt_client_t *client)
{
  if (!client) return;
  // this is what it was renamed to.  why?
  mqtt_disconnect_my2(client);
}

/* Called when publish is complete either with sucess or failure */
static void mqtt_pub_request_cb(void *arg, err_t result)
{
  if(result != ERR_OK) {
    PR_NOTICE("Publish result: %d\n", result);
  }
}

void example_publish(mqtt_client_t *client, int channel, int iVal)
{
	char pub_topic[32];
	char pub_payload[128];
//  const char *pub_payload= "{\"temperature\": \"45.5\"}";
  err_t err;
  //int myValue;
  u8_t qos = 2; /* 0 1 or 2, see MQTT specification */
  u8_t retain = 0; /* No don't retain such crappy payload... */
	const char *baseName;
  
  if(client==0)
	  return;
  if(mqtt_client_is_connected(client)==0) {
		 g_my_reconnect_mqtt_after_time = 5;
		return;
  }

  //myValue = CHANNEL_Check(channel);
   sprintf(pub_payload,"%i",iVal);
   
    PR_NOTICE("calling pub: \n");

	baseName = CFG_GetShortDeviceName();

	//sprintf(pub_topic,"wb2s/%i/get",channel);
	sprintf(pub_topic,"%s/%i/get",baseName,channel);
  err = mqtt_publish(client, pub_topic, pub_payload, strlen(pub_payload), qos, retain, mqtt_pub_request_cb, 0);
  if(err != ERR_OK) {
    PR_NOTICE("Publish err: %d\n", err);
	 if(err == ERR_CONN) {
		 
		// g_my_reconnect_mqtt_after_time = 5;


	 }
  }
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

    PR_NOTICE("MQTT in topic %s", g_mqtt_request.topic);    

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

  //PR_NOTICE("MQTT client in mqtt_incoming_publish_cb\n");
  PR_NOTICE("MQTT client in mqtt_incoming_publish_cb topic %s\n",topic);
}

static void
mqtt_request_cb(void *arg, err_t err)
{
  const struct mqtt_connect_client_info_t* client_info = (const struct mqtt_connect_client_info_t*)arg;

  PR_NOTICE("MQTT client \"%s\" request cb: err %d\n", client_info->client_id, (int)err);
}
static void mqtt_sub_request_cb(void *arg, err_t result)
{
  /* Just print the result code here for simplicity,
     normal behaviour would be to take some action if subscribe fails like
     notifying user, retry subscribe or disconnect from server */
  PR_NOTICE("Subscribe result: %i\n", result);
}

static void mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status)
{
  int i;
	char tmp[64];
	const char *baseName;
  err_t err = ERR_OK;
  const struct mqtt_connect_client_info_t* client_info = (const struct mqtt_connect_client_info_t*)arg;
  LWIP_UNUSED_ARG(client);

//  PR_NOTICE(("MQTT client < removed name > connection cb: status %d\n",  (int)status));
 // PR_NOTICE(("MQTT client \"%s\" connection cb: status %d\n", client_info->client_id, (int)status));

  if (status == MQTT_CONNECT_ACCEPTED) {
    PR_NOTICE("mqtt_connection_cb: Successfully connected\n");

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
            PR_NOTICE("mqtt_subscribe to %s return: %d\n", callbacks[i]->subscriptionTopic, err);
          } else {
            PR_NOTICE("mqtt_subscribed to %s\n", callbacks[i]->subscriptionTopic);
          }
        }
      }
    }

  	baseName = CFG_GetShortDeviceName();

    sprintf(tmp,"%s/connected",baseName);
    err = mqtt_publish(client, tmp, "online", strlen("online"), 2, true, mqtt_pub_request_cb, 0);
    if(err != ERR_OK) {
      PR_NOTICE("Publish err: %d\n", err);
      if(err == ERR_CONN) {
      // g_my_reconnect_mqtt_after_time = 5;
      }
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
    PR_NOTICE("mqtt_connection_cb: Disconnected, reason: %d\n", status);
  }
}

static void MQTT_do_connect(mqtt_client_t *client)
{
  const char *mqtt_userName, *mqtt_host, *mqtt_pass, *mqtt_clientID;
  int mqtt_port;
  struct hostent* hostEntry;
  char will_topic[32];

  mqtt_userName = CFG_GetMQTTUserName();
  mqtt_pass = CFG_GetMQTTPass();
  //mqtt_clientID = CFG_GetMQTTBrokerName();
  mqtt_clientID = CFG_GetShortDeviceName();
  mqtt_host = CFG_GetMQTTHost();
	mqtt_port = CFG_GetMQTTPort();

  PR_NOTICE("mqtt_userName %s\r\nmqtt_pass %s\r\nmqtt_clientID %s\r\nmqtt_host %s:%d\r\n",
    mqtt_userName,
    mqtt_pass,
    mqtt_clientID,
    mqtt_host,
    mqtt_port
  );


  if (!mqtt_host[0]){
    PR_NOTICE("mqtt_host empty, not starting mqtt\r\n");
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
        PR_NOTICE("mqtt_host resolves to addr len > 4\r\n");
        len = 4;
      }
      memcpy(&mqtt_ip, hostEntry->h_addr_list[0], len);
    } else {
      PR_NOTICE("mqtt_host resolves no addresses?\r\n");
      return;
    }

    // host name/ip
    //ipaddr_aton(mqtt_host,&mqtt_ip);

    /* Initiate client and connect to server, if this fails immediately an error code is returned
      otherwise mqtt_connection_cb will be called with connection result after attempting
      to establish a connection with the server.
      For now MQTT version 3.1.1 is always used */

    mqtt_client_connect(mqtt_client,
            &mqtt_ip, mqtt_port,
            mqtt_connection_cb, LWIP_CONST_CAST(void*, &mqtt_client_info),
            &mqtt_client_info);

  } else {
    PR_NOTICE("mqtt_host %s not found by gethostbyname\r\n", mqtt_host);
  }
}

static void app_my_channel_toggle_callback(int channel, int iVal)
{
  ADDLOG_INFO(LOG_FEATURE_MAIN, "Channel has changed! Publishing change %i with %i \n",channel,iVal);
	example_publish(mqtt_client,channel,iVal);
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

}


// called from user timer.
void MQTT_RunEverySecondUpdate() {

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
		ADDLOG_INFO(LOG_FEATURE_MAIN, "Timer discovers disconnected mqtt %i\n",loopsWithDisconnected);
		loopsWithDisconnected++;
		if(loopsWithDisconnected > 10)
		{ 
			if(mqtt_client == 0)
			{
			    mqtt_client = mqtt_client_new();
				CHANNEL_SetChangeCallback(app_my_channel_toggle_callback);
      }
      MQTT_do_connect(mqtt_client);
			loopsWithDisconnected = 0;
		}
	}
}

