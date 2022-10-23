
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

#define DEDUPER_MAX_STRING_LEN 32

// TODO: diferriate between string and int types, do not malloc buffer for int type
typedef struct mqtt_dedup_slot_s {
	char value[DEDUPER_MAX_STRING_LEN];
	//int iValue;
	int timeSinceLastSend;
} mqtt_dedup_slot_t;

static mqtt_dedup_slot_t *mqtt_dedups[DEDUP_MAX];

static int stat_deduper_send = 0;
static int stat_deduper_culled = 0;

void MQTT_Dedup_Tick() {
	int i;

	for(i = 0; i < DEDUP_MAX; i++) {
		if(mqtt_dedups[i]) {
			mqtt_dedups[i]->timeSinceLastSend++;
		}
	}
	
    ADDLOG_INFO(LOG_FEATURE_MQTT, "MQTT deduper sent %i, culled %i",stat_deduper_send,stat_deduper_culled);

}
OBK_Publish_Result MQTT_PublishMain_StringInt_DeDuped(int slotCode, int expireTime, const char* sChannel, int val, int flags) {
	char buffer[16];
	sprintf(buffer,"%i",val);
	return MQTT_PublishMain_StringString_DeDuped(slotCode,expireTime,sChannel,buffer,flags);
}
OBK_Publish_Result MQTT_PublishMain_StringString_DeDuped(int slotCode, int expireTime, const char* sChannel, const char* valueStr, int flags) {
	mqtt_dedup_slot_t *slot;
	OBK_Publish_Result res;

	// alloc only when it's required
	if(mqtt_dedups[slotCode] == 0) {
		mqtt_dedups[slotCode] = malloc(sizeof(mqtt_dedup_slot_t));
		memset(mqtt_dedups[slotCode],0,sizeof(mqtt_dedup_slot_t));
	}

	slot = mqtt_dedups[slotCode];

	// is value the same?
	if(!strcmp(slot->value,valueStr)) {
		// has minimal time to republish passed?
		if(expireTime > slot->timeSinceLastSend) {
			stat_deduper_culled++;
			return OBK_PUBLISH_OK; // do not resend if just few seconds passed
		}
	}
	// send futher
	res = MQTT_PublishMain_StringString(sChannel,valueStr,flags);
	if(res == OBK_PUBLISH_OK) {
		// mark as sent
		slot->timeSinceLastSend = 0;
		// save previous value
		strcpy_safe(slot->value,valueStr, DEDUPER_MAX_STRING_LEN);
	}
	stat_deduper_send++;
	return res;
}
