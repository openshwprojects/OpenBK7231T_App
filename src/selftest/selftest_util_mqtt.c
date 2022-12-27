#ifdef WINDOWS

#include "selftest_local.h".

void SIM_SendFakeMQTTAndRunSimFrame_CMND(const char *command, const char *arguments) {

	const char *myName = CFG_GetMQTTClientId();
	char buffer[4096];
	sprintf(buffer, "cmnd/%s/%s", myName, command);
	MQTT_Post_Received_Str(buffer, arguments);
	Sim_RunFrames(1, false);

}
void SIM_SendFakeMQTTRawChannelSet(int channelIndex, const char *arguments) {
	const char *myName = CFG_GetMQTTClientId();
	char buffer[4096];
	sprintf(buffer, "%s/%i/set", myName, channelIndex);
	MQTT_Post_Received_Str(buffer, arguments);
	Sim_RunFrames(1, false);
}

#define MAX_MQTT_HISTORY 64
typedef struct mqttHistoryEntry_s {
	char topic[256];
	char value[4096];
	int qos;
	bool bRetain;
} mqttHistoryEntry_t;

mqttHistoryEntry_t mqtt_history[MAX_MQTT_HISTORY];
int history_head = 0;
int history_tail = 0;

void SIM_ClearMQTTHistory() {
	history_head = history_tail = 0;
}
bool SIM_CheckMQTTHistoryForString(const char *topic, const char *value, bool bRetain) {
	mqttHistoryEntry_t *ne;
	int cur = history_tail;
	while (cur != history_head) {
		ne = &mqtt_history[cur];
		if (!strcmp(ne->topic, topic) && !strcmp(ne->value, value) && ne->bRetain == bRetain) {
			return true;
		}
		cur++;
		cur %= MAX_MQTT_HISTORY;
	}
	return false;
}
void SIM_OnMQTTPublish(const char *topic, const char *value, int len, int qos, bool bRetain) {
	mqttHistoryEntry_t *ne;

	ne = &mqtt_history[history_head];

	history_head++;
	history_head %= MAX_MQTT_HISTORY;
	if (history_tail == history_head) {
		history_tail++;
		history_tail %= MAX_MQTT_HISTORY;
	}

	strcpy_safe(ne->topic, topic, sizeof(ne->topic));
	strcpy_safe(ne->value, value, sizeof(ne->value));
	ne->bRetain = bRetain;
	ne->qos = qos;



}

#endif
