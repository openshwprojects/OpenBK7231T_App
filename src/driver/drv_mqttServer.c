#include "../logging/logging.h"
#include "../new_common.h"
#include "../obk_config.h"


#if ENABLE_DRIVER_MQTTSERVER

static int g_mqttServer_secondsElapsed = 0;

// single user only
static char g_password[128];
static char g_user[128];

typedef struct mqttVariable_s {
	char *name;
	char *value;
	int valueLen;
	struct mqttVariable_s *next;
} mqttVariable_t;

typedef struct mqttClient_s {
	char *name;
	int socket;
	mqttVariable_t *vars;
} mqttClient_t;

static mqttClient_t *g_clients = 0;

void DRV_MQTTServer_Init() {
  strcpy(g_password, "ma1oovoo0pooTie7koa8Eiwae9vohth1vool8ekaej8Voohi7beif5uMuph9Diex");
  strcpy(g_user, "homeassistant");

  g_mqttServer_secondsElapsed = 0;
  ADDLOG_INFO(LOG_FEATURE_GENERAL,
              "DRV_MQTTServer_Init: MQTT Server driver started");
}
void DRV_MQTTServer_RunEverySecond() {
  g_mqttServer_secondsElapsed++;
  ADDLOG_INFO(LOG_FEATURE_GENERAL, "DRV_MQTTServer_RunEverySecond");
}
void DRV_MQTTServer_Stop() {
  ADDLOG_INFO(LOG_FEATURE_GENERAL,
              "DRV_MQTTServer_Stop: MQTT Server driver stopped");
}

#endif // ENABLE_DRIVER_MQTTSERVER
