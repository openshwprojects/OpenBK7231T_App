
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

// Maximum lenght of both string value and publish name in MQTT deduper
#define DEDUPER_MAX_STRING_LEN 32
// put 1 to enable deduper of fast changing values
// This is used to avoid sending, let's say, 20 packets per second for a led_dimmer that
// is increased by one 20 times per second
#define DEDUPER_ENABLE_DELAY_SEND_OF_FAST_CHANGING_VALUES 1
// If option above is enabled,
// do not send the same publish (even with differnt value) more often that this:
#define MIN_INTERVAL_BETWEEN_SENDS 1

// TODO: diferriate between string and int types, do not malloc buffer for int type
typedef struct mqtt_dedup_slot_s {
	int flags;
	char name[DEDUPER_MAX_STRING_LEN];
	char value[DEDUPER_MAX_STRING_LEN];
	// if dirty, then it needs to be resend manually
	bool bValueDirty;
	int timeSinceLastSend;
} mqtt_dedup_slot_t;

static mqtt_dedup_slot_t *mqtt_dedups[DEDUP_MAX];

static int stat_deduper_send = 0;
static int stat_deduper_culled_duplicates = 0;
static int stat_deduper_culled_tooFast = 0;

static SemaphoreHandle_t g_mutex = 0;


static bool DD_Mutex_Take(int del) {
    int taken;

    if (g_mutex == 0)
    {
        g_mutex = xSemaphoreCreateMutex();
    }
    taken = xSemaphoreTake(g_mutex, del);
    if (taken == pdTRUE) {
        return true;
    }
    return false;
}

static void DD_Mutex_Free()
{
    xSemaphoreGive(g_mutex);
}

void MQTT_Dedup_Tick() {
	int i;

	//if(DD_Mutex_Take(10)) {
	//	return;
	///}
	for(i = 0; i < DEDUP_MAX; i++) {
		if(mqtt_dedups[i]) {
			mqtt_dedups[i]->timeSinceLastSend++;
#if DEDUPER_ENABLE_DELAY_SEND_OF_FAST_CHANGING_VALUES
			if(mqtt_dedups[i]->timeSinceLastSend > MIN_INTERVAL_BETWEEN_SENDS && mqtt_dedups[i]->bValueDirty) {
				// Some values of this publish were not published, because we had too many publish requests in one second or so.
				// Now the cooldown has passed, so we can send the LATEST, most up-to-date value of this publish.
				MQTT_PublishMain_StringString(mqtt_dedups[i]->name,mqtt_dedups[i]->value,mqtt_dedups[i]->flags);
				mqtt_dedups[i]->bValueDirty = false;
				mqtt_dedups[i]->timeSinceLastSend = 0;
			}
#endif
		}
	}
//	DD_Mutex_Free();
	if (CFG_HasLoggerFlag(LOGGER_FLAG_MQTT_DEDUPER)) {
		ADDLOG_DEBUG(LOG_FEATURE_MQTT, "MQTT deduper sent %i, culled duplicates %i, culled too fast %i",
			stat_deduper_send, stat_deduper_culled_duplicates, stat_deduper_culled_tooFast);
	}

}
OBK_Publish_Result MQTT_PublishMain_StringInt_DeDuped(int slotCode, int expireTime, const char* sChannel, int val, int flags) {
	char buffer[16];
	sprintf(buffer,"%i",val);
	return MQTT_PublishMain_StringString_DeDuped(slotCode,expireTime,sChannel,buffer,flags);
}
OBK_Publish_Result MQTT_PublishMain_StringString_DeDuped(int slotCode, int expireTime, const char* sChannel, const char* valueStr, int flags) {
	mqtt_dedup_slot_t *slot;
	OBK_Publish_Result res;

	// for simulator, we don't currently need dups removal
#ifdef WINDOWS
	return MQTT_PublishMain_StringString(sChannel, valueStr, flags);
#endif

	// alloc only when it's required
	if(mqtt_dedups[slotCode] == 0) {
		mqtt_dedups[slotCode] = malloc(sizeof(mqtt_dedup_slot_t));
		// just in case malloc fails..
		if (mqtt_dedups[slotCode] == 0) {
			 return MQTT_PublishMain_StringString(sChannel, valueStr, flags);
		}
		memset(mqtt_dedups[slotCode],0,sizeof(mqtt_dedup_slot_t));
		mqtt_dedups[slotCode]->timeSinceLastSend = 999;
	}

	slot = mqtt_dedups[slotCode];

	// is value the same?
	if(!strcmp(slot->value,valueStr)) {
		// has minimal time to republish passed?
		if(expireTime > slot->timeSinceLastSend) {
			stat_deduper_culled_duplicates++;
			return OBK_PUBLISH_OK; // do not resend if just few seconds passed
		}
	}
#if DEDUPER_ENABLE_DELAY_SEND_OF_FAST_CHANGING_VALUES
	// has minimal time to republish passed?
	// 'slot->timeSinceLastSend' is increased ONCE per second
	// So we check if it was just sent this second or previous second
	if(MIN_INTERVAL_BETWEEN_SENDS >= slot->timeSinceLastSend) {
		// It was sent in last second, don't resend just again

		// Just save values for later

		// save previous value
		strcpy_safe(slot->value,valueStr, DEDUPER_MAX_STRING_LEN);
		strcpy_safe(slot->name,sChannel, DEDUPER_MAX_STRING_LEN);
		// mark as 'have to republish later'
		slot->bValueDirty = true;
		slot->flags = flags;
		stat_deduper_culled_tooFast++;
		return OBK_PUBLISH_OK; // do not resend if just few seconds passed
	}
#endif
	// send futher
	res = MQTT_PublishMain_StringString(sChannel,valueStr,flags);
	if(res == OBK_PUBLISH_OK) {
		slot->bValueDirty = false;
		// mark as sent
		slot->timeSinceLastSend = 0;
		// save previous value
		strcpy_safe(slot->value,valueStr, DEDUPER_MAX_STRING_LEN);
	}
	stat_deduper_send++;
	return res;
}
