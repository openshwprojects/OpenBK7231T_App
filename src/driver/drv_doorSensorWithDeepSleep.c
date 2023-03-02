// Power-saving door sensor driver with deep sleep
// Device reboots and waits for MQTT connection, while monitoring door sensor status.
// After a certain amount of time with MQTT connection, to make sure that door state is 
// already reported, and with no futher door state changes, device goes to deep sleep 
// and waits for wakeup from door sensor input state change.

#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../mqtt/new_mqtt.h"
#include "../logging/logging.h"
#include "drv_local.h"
#include "drv_uart.h"
#include "../httpserver/new_http.h"
#include "../hal/hal_pins.h"
#include "../hal/hal_adc.h"

static int setting_timeRequiredUntilDeepSleep = 60;
static int g_noChangeTimePassed = 0;
static int g_emergencyTimeWithNoConnection = 0;

#define EMERGENCY_TIME_TO_SLEEP_WITHOUT_MQTT 60 * 5

void DoorDeepSleep_Init() {
	// 0 seconds since last change
	g_noChangeTimePassed = 0;
}

void DoorDeepSleep_OnEverySecond() {
	int i;

#if PLATFORM_BK7231N || PLATFORM_BK7231T
	if (ota_progress() >= 0) {
#else
	if (false) {
#endif
		// update active
		g_noChangeTimePassed = 0;
		g_emergencyTimeWithNoConnection = 0;
	} else if (Main_HasMQTTConnected() && Main_HasWiFiConnected()) {
		//if (g_noChangeTimePassed < 4) {
			for (i = 0; i < PLATFORM_GPIO_MAX; i++) {
				if (g_cfg.pins.roles[i] == IOR_DoorSensorWithDeepSleep ||
					g_cfg.pins.roles[i] == IOR_DoorSensorWithDeepSleep_NoPup) {
					MQTT_ChannelPublish(g_cfg.pins.channels[i], 0);
				}
			}
		//}
		g_noChangeTimePassed++;
		if (g_noChangeTimePassed >= setting_timeRequiredUntilDeepSleep) {
			// start deep sleep in the next loop
			// The deep sleep start will check for role: IOR_DoorSensorWithDeepSleep
			// and for those pins, it will wake up on the pin change to opposite state
			// (so if door sensor is low, it will wake up on rising edge,
			// if door sensor is high, it will wake up on falling)
			g_bWantPinDeepSleep = true;
		}
	}
	else {
		g_emergencyTimeWithNoConnection++;
		if (g_emergencyTimeWithNoConnection >= EMERGENCY_TIME_TO_SLEEP_WITHOUT_MQTT) {
			g_bWantPinDeepSleep = true;
		}
	}
}


void DoorDeepSleep_StopDriver() {

}
void DoorDeepSleep_AppendInformationToHTTPIndexPage(http_request_t* request)
{
	int untilSleep;

	if (Main_HasMQTTConnected()) {
		untilSleep = setting_timeRequiredUntilDeepSleep - g_noChangeTimePassed;
		hprintf255(request, "<h2>Door: time until deep sleep: %i</h2>", untilSleep);
	}
	else {
		untilSleep = EMERGENCY_TIME_TO_SLEEP_WITHOUT_MQTT - g_emergencyTimeWithNoConnection;
		hprintf255(request, "<h2>Door: waiting for MQTT connection (but will emergency sleep in %i)</h2>", untilSleep);
	}
}

void DoorDeepSleep_OnChannelChanged(int ch, int value) {
	// detect door state change
	// (only sleep when there are no changes for certain time)
	if (CHANNEL_HasChannelPinWithRoleOrRole(ch, IOR_DoorSensorWithDeepSleep, IOR_DoorSensorWithDeepSleep_NoPup)) {
		// 0 seconds since last change
		g_noChangeTimePassed = 0;
		g_emergencyTimeWithNoConnection = 0;
	}
}






