#ifdef WINDOWS

#include "selftest_local.h".

void Test_Commands_Generic() {
	// reset whole device
	SIM_ClearOBK(0);

	CMD_ExecuteCommand("MqttUser Tester", 0);
	SELFTEST_ASSERT_STRING(CFG_GetMQTTUserName(), "Tester");

	CMD_ExecuteCommand("MqttUser ", 0);
	SELFTEST_ASSERT_STRING(CFG_GetMQTTUserName(), "Tester");

	CMD_ExecuteCommand("MqttPassword Secret1", 0);
	SELFTEST_ASSERT_STRING(CFG_GetMQTTPass(), "Secret1");

	CMD_ExecuteCommand("SSID1 TPLink123", 0);
	SELFTEST_ASSERT_STRING(CFG_GetWiFiSSID(), "TPLink123");
}


#endif
