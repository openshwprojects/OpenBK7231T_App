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
#include "../ota/ota.h"

static int setting_timeRequiredUntilDeepSleep = 60;
static int g_noChangeTimePassed = 0;
static int g_initialStateSent = 0;
static int g_emergencyTimeWithNoConnection = 0;

#define EMERGENCY_TIME_TO_SLEEP_WITHOUT_MQTT 60 * 5


// addEventHandler OnClick 8 DSTime + 100
commandResult_t DoorDeepSleep_SetTime(const void* context, const char* cmd, const char* args, int cmdFlags) {

	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES | TOKENIZER_DONT_EXPAND);
	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1))
	{
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	if (Tokenizer_GetArg(0)[0] == '+') {
		g_noChangeTimePassed -= Tokenizer_GetArgInteger(0);
	}
	else {
		setting_timeRequiredUntilDeepSleep = Tokenizer_GetArgInteger(0);
	}


	return CMD_RES_OK;
}
void DoorDeepSleep_Init() {
	// 0 seconds since last change
	g_noChangeTimePassed = 0;

	//cmddetail:{"name":"DSTime","args":"[timeSeconds]",
	//cmddetail:"descr":"DoorSensor driver configuration command. Time to keep device running before next sleep after last door sensor change. In future we may add also an option to automatically sleep after MQTT confirms door state receival",
	//cmddetail:"fn":"DoorDeepSleep_SetTime","file":"drv/drv_doorSensorWithDeepSleep.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("DSTime", DoorDeepSleep_SetTime, NULL);
}

void DoorDeepSleep_OnEverySecond() {
	int i, bValue;
	char tmp[8];

#if PLATFORM_BK7231N || PLATFORM_BK7231T
	if (ota_progress() >= 0) {
#else
	if (false) {
#endif
		// update active
		g_noChangeTimePassed = 0;
		g_emergencyTimeWithNoConnection = 0;
	} else if (Main_HasMQTTConnected() && Main_HasWiFiConnected()) {
		if (g_initialStateSent < 3) {
			for (i = 0; i < PLATFORM_GPIO_MAX; i++) {
				if (g_cfg.pins.roles[i] == IOR_DoorSensorWithDeepSleep ||
					g_cfg.pins.roles[i] == IOR_DoorSensorWithDeepSleep_NoPup ||
					g_cfg.pins.roles[i] == IOR_DoorSensorWithDeepSleep_pd) {
					sprintf(tmp, "%i", g_cfg.pins.channels[i]);
					bValue = BIT_CHECK(g_initialPinStates, i);
					MQTT_PublishMain_StringInt(tmp,bValue, 0);
				}
			}
			g_initialStateSent++;
		} else {
			for (i = 0; i < PLATFORM_GPIO_MAX; i++) {
				if (g_cfg.pins.roles[i] == IOR_DoorSensorWithDeepSleep ||
					g_cfg.pins.roles[i] == IOR_DoorSensorWithDeepSleep_NoPup ||
					g_cfg.pins.roles[i] == IOR_DoorSensorWithDeepSleep_pd) {
					MQTT_ChannelPublish(g_cfg.pins.channels[i], 0);
				}
			}
		}
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
		hprintf255(request, "<h2>Door (initial: %i): time until deep sleep: %i</h2>", g_initialPinStates, untilSleep);
	}
	else {
		untilSleep = EMERGENCY_TIME_TO_SLEEP_WITHOUT_MQTT - g_emergencyTimeWithNoConnection;
		hprintf255(request, "<h2>Door (initial: %i): waiting for MQTT connection (but will emergency sleep in %i)</h2>", g_initialPinStates, untilSleep);
	}
}

void DoorDeepSleep_OnChannelChanged(int ch, int value) {
	// detect door state change
	// (only sleep when there are no changes for certain time)
	if (CHANNEL_HasChannelPinWithRoleOrRole(ch, IOR_DoorSensorWithDeepSleep, IOR_DoorSensorWithDeepSleep_NoPup)
		|| CHANNEL_HasChannelPinWithRoleOrRole(ch, IOR_DoorSensorWithDeepSleep, IOR_DoorSensorWithDeepSleep_pd)) {
		// 0 seconds since last change
		g_noChangeTimePassed = 0;
		g_emergencyTimeWithNoConnection = 0;
	}
}






