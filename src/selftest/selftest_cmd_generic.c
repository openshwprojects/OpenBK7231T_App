#ifdef WINDOWS

#include "selftest_local.h"

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


	// chooses a command from a list by 0-based index
	CMD_ExecuteCommand("SetChannel 10 2",0);
	CMD_ExecuteCommand("Choice $CH10 \"SetChannel 12 222\" \"SetChannel 12 333\" \"SetChannel 12 444\" \"SetChannel 12 555\" \"SetChannel 12 666\"", 0);
	SELFTEST_ASSERT_CHANNEL(12, 444);
	CMD_ExecuteCommand("SetChannel 10 1", 0);
	CMD_ExecuteCommand("Choice $CH10 \"SetChannel 12 222\" \"SetChannel 12 333\" \"SetChannel 12 444\" \"SetChannel 12 555\" \"SetChannel 12 666\"", 0);
	SELFTEST_ASSERT_CHANNEL(12, 333);
	CMD_ExecuteCommand("SetChannel 10 0", 0);
	CMD_ExecuteCommand("Choice $CH10 \"SetChannel 12 222\" \"SetChannel 12 333\" \"SetChannel 12 444\" \"SetChannel 12 555\" \"SetChannel 12 666\"", 0);
	SELFTEST_ASSERT_CHANNEL(12, 222);
	CMD_ExecuteCommand("SetChannel 10 3", 0);
	CMD_ExecuteCommand("Choice $CH10 \"SetChannel 12 222\" \"SetChannel 12 333\" \"SetChannel 12 444\" \"SetChannel 12 555\" \"SetChannel 12 666\"", 0);
	SELFTEST_ASSERT_CHANNEL(12, 555);
	CMD_ExecuteCommand("SetChannel 10 2", 0);
	CMD_ExecuteCommand("Choice $CH10+1 \"SetChannel 12 222\" \"SetChannel 12 333\" \"SetChannel 12 444\" \"SetChannel 12 555\" \"SetChannel 12 666\"", 0);
	SELFTEST_ASSERT_CHANNEL(12, 555);
}


#endif
