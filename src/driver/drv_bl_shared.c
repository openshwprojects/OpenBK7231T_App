#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../mqtt/new_mqtt.h"
#include "../logging/logging.h"
#include "drv_public.h"
#include "drv_local.h"
#include "drv_uart.h"
#include "../httpserver/new_http.h"

int stat_updatesSkipped = 0;
int stat_updatesSent = 0;

// Current values
float lastReadings[OBK_NUM_MEASUREMENTS];
//
// Variables below are for optimization
// We can't send a full MQTT update every second.
// It's too much for Beken, and it's too much for LWIP 2 MQTT library,
// especially when actively browsing site and using JS app Log Viewer.
// It even fails to publish with -1 error (can't alloc next packet)
// So we publish when value changes from certain threshold or when a certain time passes.
//
// what are the last values we sent over the MQTT?
float lastSentValues[OBK_NUM_MEASUREMENTS];
// how much update frames has passed without sending MQTT update of read values?
int noChangeFrames[OBK_NUM_MEASUREMENTS];
// how much of value have to change in order to be send over MQTT again?
int changeSendThresholds[OBK_NUM_MEASUREMENTS] = {
	0.25f, // voltage - OBK_VOLTAGE
	0.002f, // current - OBK_CURRENT
	0.25f, // power - OBK_POWER
};
// how are they called in MQTT
const char *mqttNames[OBK_NUM_MEASUREMENTS] = {
	"voltage",
	"current",
	"power"
};

int changeSendAlwaysFrames = 60;

void BL09XX_AppendInformationToHTTPIndexPage(http_request_t *request) {
	char tmp[128];
	const char *mode;

	if(DRV_IsRunning("BL0937")) {
		mode = "BL0937";
	} else if(DRV_IsRunning("BL0942")) {
		mode = "BL0942";
	} else {
		mode = "PWR";
	}
	sprintf(tmp, "<h2>%s Voltage=%f, Current=%f, Power=%f (changes sent %i, skipped %i)</h2>",
		mode, lastReadings[OBK_VOLTAGE],lastReadings[OBK_CURRENT], lastReadings[OBK_POWER],
		stat_updatesSent, stat_updatesSkipped);
    hprintf128(request,tmp);

}
void BL_ProcessUpdate(float voltage, float current, float power) {
	int i;

// those are final values, like 230V
	lastReadings[OBK_POWER] = power;
	lastReadings[OBK_VOLTAGE] = voltage;
	lastReadings[OBK_CURRENT] = current;

	for(i = 0; i < OBK_NUM_MEASUREMENTS; i++) {
		// send update only if there was a big change or if certain time has passed
		if(
			(abs(lastSentValues[i]-lastReadings[i]) > changeSendThresholds[i])
			||
			noChangeFrames[i] > changeSendAlwaysFrames
			){
			noChangeFrames[i] = 0;
			if(i == OBK_CURRENT) {
				int prev_mA, now_mA;
				prev_mA = lastSentValues[i] * 1000;
				now_mA = lastReadings[i] * 1000;
				EventHandlers_ProcessVariableChange_Integer(CMD_EVENT_CHANGE_CURRENT, prev_mA,now_mA);
			} else {
				EventHandlers_ProcessVariableChange_Integer(CMD_EVENT_CHANGE_VOLTAGE+i, lastSentValues[i], lastReadings[i]);
			}
			lastSentValues[i] = lastReadings[i];
			MQTT_PublishMain_StringFloat(mqttNames[i],lastReadings[i]);
			stat_updatesSent++;
		} else {
			// no change frame
			noChangeFrames[i]++;
			stat_updatesSkipped++;
		}
	}
}
void BL_Shared_Init() {

	int i;

	for(i = 0; i < OBK_NUM_MEASUREMENTS; i++) {
		noChangeFrames[i] = 0;
		lastReadings[i] = 0;
	}
}
// OBK_POWER etc
float DRV_GetReading(int type) {
	return lastReadings[type];
}




