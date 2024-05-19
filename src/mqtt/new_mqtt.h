#ifndef __NEW_MQTT_H__
#define __NEW_MQTT_H__

#include "../new_common.h"

#include "lwip/sockets.h"
#include "lwip/ip_addr.h"
#include "lwip/inet.h"
#include "lwip/netdb.h"
#if PLATFORM_XR809
#include "my_lwip2_mqtt_replacement.h"
#else
#include "lwip/apps/mqtt.h"
#endif

extern ip_addr_t mqtt_ip;
extern mqtt_client_t* mqtt_client;

void MQTT_init();
int MQTT_RunQuickTick();
int MQTT_RunEverySecondUpdate();
void MQTT_BroadcastTasmotaTeleSTATE();
void MQTT_BroadcastTasmotaTeleSENSOR();


#define PUBLISHITEM_ALL_INDEX_FIRST   -17

//These 3 values are pretty much static
#define PUBLISHITEM_SELF_STATIC_RESERVED_2      -17
#define PUBLISHITEM_SELF_STATIC_RESERVED_1      -16
#define PUBLISHITEM_SELF_HOSTNAME               -15  //Device name
#define PUBLISHITEM_SELF_BUILD                  -14  //Build
#define PUBLISHITEM_SELF_MAC                    -13  //Device mac

#define PUBLISHITEM_DYNAMIC_INDEX_FIRST         -12

#define PUBLISHITEM_QUEUED_VALUES               -12  //Publish queued items

//These values are dynamic
#define PUBLISHITEM_SELF_TEMP		            -11  // Internal temp
#define PUBLISHITEM_SELF_SSID		            -10  // SSID
#define PUBLISHITEM_SELF_DATETIME               -9  //Current unix datetime
#define PUBLISHITEM_SELF_SOCKETS                -8  //Active sockets
#define PUBLISHITEM_SELF_RSSI                   -7  //Link strength
#define PUBLISHITEM_SELF_UPTIME                 -6  //Uptime
#define PUBLISHITEM_SELF_FREEHEAP               -5  //Free heap
#define PUBLISHITEM_SELF_IP                     -4  //ip address

#define PUBLISHITEM_SELF_DYNAMIC_LIGHTSTATE     -3
#define PUBLISHITEM_SELF_DYNAMIC_LIGHTMODE      -2
#define PUBLISHITEM_SELF_DYNAMIC_DIMMER         -1


enum OBK_Publish_Result_e {
	OBK_PUBLISH_OK,
	OBK_PUBLISH_MUTEX_FAIL,
	OBK_PUBLISH_WAS_DISCONNECTED,
	OBK_PUBLISH_WAS_NOT_REQUIRED,
	OBK_PUBLISH_MEM_FAIL,
};

#define OBK_PUBLISH_FLAG_MUTEX_SILENT			1
#define OBK_PUBLISH_FLAG_RETAIN					2
#define OBK_PUBLISH_FLAG_FORCE_REMOVE_GET		4
// do not add anything to given topic
#define OBK_PUBLISH_FLAG_RAW_TOPIC_NAME			8


#include "new_mqtt_deduper.h"


// ability to register callbacks for MQTT data
typedef struct obk_mqtt_request_tag {
	const unsigned char* received; // note: NOT terminated, may be binary
	int receivedLen;
	char topic[128];
} obk_mqtt_request_t;

#define MQTT_PUBLISH_ITEM_TOPIC_LENGTH    64
#define MQTT_PUBLISH_ITEM_CHANNEL_LENGTH  128
#define MQTT_PUBLISH_ITEM_VALUE_LENGTH    1023

typedef enum PostPublishCommands_e {
	None,
	PublishAll,
	PublishChannels
} PostPublishCommands;


/// @brief Publish queue item
typedef struct MqttPublishItem
{
	char topic[MQTT_PUBLISH_ITEM_TOPIC_LENGTH];
	char channel[MQTT_PUBLISH_ITEM_CHANNEL_LENGTH];
	char value[MQTT_PUBLISH_ITEM_VALUE_LENGTH];
	int flags;
	struct MqttPublishItem* next;
	PostPublishCommands command;
} MqttPublishItem_t;


// Maximum length to log data parameters
#define MQTT_MAX_DATA_LOG_LENGTH					12

// Count of queued items published at once.
#define MQTT_QUEUED_ITEMS_PUBLISHED_AT_ONCE	3
// When using Hass discovery, when we have, for example,
// 16 relays, every relay will be a separate publish,
// so I bumped MAX to 32
#define MQTT_MAX_QUEUE_SIZE	                32

// callback function for mqtt.
// return 0 to allow the incoming topic/data to be processed by others/channel set.
// return 1 to 'eat the packet and terminate further processing.
typedef int (*mqtt_callback_fn)(obk_mqtt_request_t* request);

// topics must be unique (i.e. you can't have /about and /aboutme or /about/me)
// ALL topics currently must start with main device topic root.
// ID is unique and non-zero - so that callbacks can be replaced....
int MQTT_GetConnectEvents(void);
const char* get_error_name(int err);
int MQTT_GetConnectResult(void);
char* MQTT_GetStatusMessage(void);
int MQTT_GetPublishEventCounter(void);
int MQTT_GetPublishErrorCounter(void);
int MQTT_GetReceivedEventCounter(void);

OBK_Publish_Result PublishQueuedItems();
OBK_Publish_Result MQTT_ChannelPublish(int channel, int flags);
void MQTT_ClearCallbacks();
int MQTT_RegisterCallback(const char* basetopic, const char* subscriptiontopic, int ID, mqtt_callback_fn callback);
int MQTT_RemoveCallback(int ID);

// this is called from tcp_thread context to queue received mqtt,
// and then we'll retrieve them from our own thread for processing.
//
// NOTE: this function is now public, but only because my unit tests
// system can use it to spoof MQTT packets to check if MQTT commands
// are working...
int MQTT_Post_Received(const char *topic, int topiclen, const unsigned char *data, int datalen);
int MQTT_Post_Received_Str(const char *topic, const char *data);

void MQTT_GetStats(int* outUsed, int* outMax, int* outFreeMem);

OBK_Publish_Result MQTT_DoItemPublish(int idx);
OBK_Publish_Result MQTT_PublishMain_StringFloat(const char* sChannel, float f, 
	int maxDecimalPlaces, int flags);
OBK_Publish_Result MQTT_PublishMain_StringInt(const char* sChannel, int val, int flags);
OBK_Publish_Result MQTT_PublishMain_StringString(const char* sChannel, const char* valueStr, int flags);
void MQTT_PublishOnlyDeviceChannelsIfPossible();
void MQTT_QueuePublish(const char* topic, const char* channel, const char* value, int flags);
void MQTT_QueuePublishWithCommand(const char* topic, const char* channel, const char* value, int flags, PostPublishCommands command);
OBK_Publish_Result MQTT_Publish(const char* sTopic, const char* sChannel, const char* value, int flags);
OBK_Publish_Result MQTT_PublishStat(const char* statName, const char* statValue);
OBK_Publish_Result MQTT_PublishTele(const char* teleName, const char* teleValue);
void MQTT_InvokeCommandAtEnd(PostPublishCommands command);
bool MQTT_IsReady();
extern int g_mqtt_bBaseTopicDirty;
extern int mqtt_reconnect;
extern int mqtt_loopsWithDisconnected;

// TODO: hide it, internal usage only
#define MQTT_STACK_BUFFER_SIZE 32
#define MQTT_TOTAL_BUFFER_SIZE 4096
typedef struct obk_mqtt_publishReplyPrinter_s {
	char *allocated;
	char stackBuffer[MQTT_STACK_BUFFER_SIZE];
	int curLen;
} obk_mqtt_publishReplyPrinter_t;

void MQTT_PublishPrinterContentsToStat(obk_mqtt_publishReplyPrinter_t *printer, const char *statName);
void MQTT_PublishPrinterContentsToTele(obk_mqtt_publishReplyPrinter_t *printer, const char *statName);


#endif // __NEW_MQTT_H__
