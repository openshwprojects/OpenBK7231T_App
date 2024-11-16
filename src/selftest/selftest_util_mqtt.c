#ifdef WINDOWS

#include "selftest_local.h"
#include "../cJSON/cJSON.h"

void SIM_SendFakeMQTT(const char *text, const char *arguments) {
	MQTT_Post_Received_Str(text, arguments);
	Sim_RunFrames(1, false);
}
void SIM_SendFakeMQTTAndRunSimFrame_CMND_Generic(const char *myName , const char *command, const char *arguments) {

	char buffer[4096];
	snprintf(buffer,sizeof(buffer), "cmnd/%s/%s", myName, command);

	SIM_SendFakeMQTT(buffer, arguments);

}
void SIM_SendFakeMQTTAndRunSimFrame_CMND_ViaGroupTopic(const char *command, const char *arguments) {
	const char *myName = CFG_GetMQTTGroupTopic();
	SIM_SendFakeMQTTAndRunSimFrame_CMND_Generic(myName, command, arguments);
}
void SIM_SendFakeMQTTAndRunSimFrame_CMND(const char *command, const char *arguments) {
	const char *myName = CFG_GetMQTTClientId();
	SIM_SendFakeMQTTAndRunSimFrame_CMND_Generic(myName, command, arguments);
}
void SIM_SendFakeMQTTRawChannelSet_Generic(const char *myName, int channelIndex, const char *arguments) {
	char buffer[4096];
	snprintf(buffer, sizeof(buffer), "%s/%i/set", myName, channelIndex);
	SIM_SendFakeMQTT(buffer, arguments);
}
void SIM_SendFakeMQTTRawChannelSet(int channelIndex, const char *arguments) {
	const char *myName = CFG_GetMQTTClientId();
	SIM_SendFakeMQTTRawChannelSet_Generic(myName, channelIndex, arguments);
}
void SIM_SendFakeMQTTRawChannelSet_ViaGroupTopic(int channelIndex, const char *arguments) {
	const char *myName = CFG_GetMQTTGroupTopic();
	SIM_SendFakeMQTTRawChannelSet_Generic(myName, channelIndex, arguments);
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
bool ST_IsIntegerString(const char *s) {
	if (s == 0)
		return false;
	if (*s == 0)
		return false;
	while (*s) {
		if (isdigit(*s) == false)
			return false;
		s++;
	}
	return true;
}
bool SIM_HasMQTTHistoryStringWithJSONPayload(const char *topic, bool bPrefixMode, const char *object1, const char *object2, const char *key, const char *value) {
	mqttHistoryEntry_t *ne;
	int cur = history_tail;
	while (cur != history_head) {
		bool bMatch = false;
		ne = &mqtt_history[cur];
		if (bPrefixMode) {
			if (!strncmp(ne->topic, topic, strlen(topic))) {
				bMatch = true;
			}
		}
		else {
			if (!strcmp(ne->topic, topic)) {
				bMatch = true;
			}
		}
		if (bMatch) {
			cJSON *json = cJSON_Parse(ne->value);
			if (json) {
				cJSON *tmp;
				tmp = json;
				if (object1) {
					tmp = cJSON_GetObjectItemCaseSensitive(tmp, object1);
				}
				if (tmp) {
					if (object2) {
						tmp = cJSON_GetObjectItemCaseSensitive(tmp, object2);
					}
					if (tmp) {
						tmp = cJSON_GetObjectItemCaseSensitive(tmp, key);
						if (tmp) {
							if (tmp->valuestring) {
								const char *ret = tmp->valuestring;
								if (!strcmp(ret, value)) {
									return true;
								}
							}
							else {
								if (ST_IsIntegerString(value)) {
									if (atoi(value) == tmp->valueint) {
										return true;
									}
								}
								else {
									printf("TODO: float compare selftest");
								}
							}
						}
					}

				}
				cJSON_Delete(json);
			}
		}
		cur++;
		cur %= MAX_MQTT_HISTORY;
	}

	return false;
}
const char *SIM_GetMQTTHistoryString(const char *topic, bool bPrefixMode) {
	mqttHistoryEntry_t *ne;
	int cur = history_tail;
	while (cur != history_head) {
		ne = &mqtt_history[cur];
		if (bPrefixMode) {
			if (!strncmp(ne->topic, topic,strlen(topic))) {
				return ne->value;
			}
		}
		else {
			if (!strcmp(ne->topic, topic)) {
				return ne->value;
			}
		}
		cur++;
		cur %= MAX_MQTT_HISTORY;
	}
	return 0;
}
bool SIM_CheckMQTTHistoryForFloat(const char *topic, float value, bool bRetain) {
	mqttHistoryEntry_t *ne;
	int cur = history_tail;
	while (cur != history_head) {
		ne = &mqtt_history[cur];
		float neVal = atof(ne->value);
		if (!strcmp(ne->topic, topic) && Float_Equals(neVal, value) && ne->bRetain == bRetain) {
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

#if 1
	{
		FILE *f;
		f = fopen("sim_lastPublish.txt", "wb");
		if (f != 0) {
			fprintf(f, "Topic: %s", topic);
			fprintf(f, "\n");
			fprintf(f, "Payload: %s", value);
			fclose(f);
		}
		if (strlen(value) > 32) {
			f = fopen("sim_lastPublish_long.txt", "wb");
			if (f != 0) {
				fprintf(f, "Topic: %s", topic);
				fprintf(f, "\n");
				fprintf(f, "Payload: %s", value);
				fclose(f);
			}
		}
		f = fopen("sim_lastPublishes.txt", "a");
		if (f != 0) {
			fprintf(f, "\n");
			fprintf(f, "Topic: %s", topic);
			fprintf(f, "\n");
			fprintf(f, "Payload: %s", value);
			fclose(f);
		}
	}
#endif



}

#endif
