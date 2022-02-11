
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

static int g_my_reconnect_mqtt_after_time = -1;
ip_addr_t mqtt_ip LWIP_MQTT_EXAMPLE_IPADDR_INIT;
mqtt_client_t* mqtt_client;

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




/* Called when publish is complete either with sucess or failure */
static void mqtt_pub_request_cb(void *arg, err_t result)
{
  if(result != ERR_OK) {
    PR_NOTICE("Publish result: %d\n", result);
  }
}
static void
mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status);

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

int g_incoming_channel_mqtt = 0;
static void mqtt_incoming_data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags)
{
	int iValue;
  // unused - left here as example
  //const struct mqtt_connect_client_info_t* client_info = (const struct mqtt_connect_client_info_t*)arg;

  //PR_NOTICE("MQTT client in mqtt_incoming_data_cb\n");
  PR_NOTICE("MQTT client in mqtt_incoming_data_cb data is %s for ch %i\n",data,g_incoming_channel_mqtt);

  iValue = atoi((char *)data);
  CHANNEL_Set(g_incoming_channel_mqtt,iValue,0);

 // PR_NOTICE(("MQTT client \"%s\" data cb: len %d, flags %d\n", client_info->client_id, (int)len, (int)flags));
}

static void mqtt_incoming_publish_cb(void *arg, const char *topic, u32_t tot_len)
{
	const char *p;
  // unused - left here as example
  //const struct mqtt_connect_client_info_t* client_info = (const struct mqtt_connect_client_info_t*)arg;

  //PR_NOTICE("MQTT client in mqtt_incoming_publish_cb\n");
  PR_NOTICE("MQTT client in mqtt_incoming_publish_cb topic %s\n",topic);
// TODO: better 
//  g_incoming_channel_mqtt = topic[5] - '0';
  p = topic;
  while(*p != '/') {
	  if(*p == 0)
		  return;
		p++;
  }
  p++;
  g_incoming_channel_mqtt = *p - '0';

 // PR_NOTICE(("MQTT client \"%s\" publish cb: topic %s, len %d\n", client_info->client_id, topic, (int)tot_len));
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
void example_do_connect(mqtt_client_t *client);
static void mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status)
{
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

	 /* Subscribe to a topic named "subtopic" with QoS level 1, call mqtt_sub_request_cb with result */

	baseName = CFG_GetShortDeviceName();
  // + is a MQTT wildcard
	sprintf(tmp,"%s/+/set",baseName);
    err = mqtt_sub_unsub(client,
          //  "wb2s/+/set", 1,
		  tmp, 1,
            mqtt_request_cb, LWIP_CONST_CAST(void*, client_info),
            1);
    if(err != ERR_OK) {
      PR_NOTICE("mqtt_subscribe return: %d\n", err);
    }

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
   // example_do_connect(client);

  }
}

void example_do_connect(mqtt_client_t *client)
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


