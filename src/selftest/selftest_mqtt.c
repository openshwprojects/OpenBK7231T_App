#ifdef WINDOWS

#include "selftest_local.h"
#include "../hal/hal_wifi.h"

void Test_MQTT_Channels() {
	SIM_ClearOBK();
	PIN_SetPinRoleForPinIndex(9, IOR_Relay);
	PIN_SetPinChannelForPinIndex(9, 1);
	SIM_ClearMQTTHistory();

	Main_OnWiFiStatusChange(WIFI_STA_CONNECTED);
	CFG_SetMQTTClientId("myTestDevice");
	MQTT_init();
	for (int i = 0; i < 20; i++) {
		MQTT_RunEverySecondUpdate();
	}

	// This should trigger MQTT publish
	CMD_ExecuteCommand("setChannel 1 1", 0);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("myTestDevice/1/get", "1", false);

	// This should trigger MQTT publish
	CMD_ExecuteCommand("setChannel 1 0", 0);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("myTestDevice/1/get", "0", false);

	// This should trigger MQTT publish
	CMD_ExecuteCommand("publish myTestVal 123", 0);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("myTestDevice/myTestVal/get", "123", false);
}
void Test_MQTT(){
	Test_MQTT_Channels();
}

#endif
