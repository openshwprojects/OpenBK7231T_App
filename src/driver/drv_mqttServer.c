#include "../new_common.h"
#include "../logging/logging.h"
#include "../obk_config.h"

#if ENABLE_DRIVER_MQTTSERVER

static int g_mqttServer_secondsElapsed = 0;

void DRV_MQTTServer_Init() {
	g_mqttServer_secondsElapsed = 0;
	ADDLOG_INFO(LOG_FEATURE_GENERAL, "DRV_MQTTServer_Init: MQTT Server driver started");
}
void DRV_MQTTServer_RunEverySecond() {
	g_mqttServer_secondsElapsed++;
}
void DRV_MQTTServer_Stop() {
	ADDLOG_INFO(LOG_FEATURE_GENERAL, "DRV_MQTTServer_Stop: MQTT Server driver stopped");
}

#endif // ENABLE_DRIVER_MQTTSERVER
