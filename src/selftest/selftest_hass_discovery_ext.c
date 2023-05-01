#ifdef WINDOWS

#include "selftest_local.h".

void Test_HassDiscovery_TuyaMCU_VoltageCurrentPower() {
	const char *shortName = "WinTuyatest";
	const char *fullName = "Windows Fake Tuya";
	const char *mqttName = "testTuya";
	SIM_ClearOBK(shortName);
	SIM_ClearAndPrepareForMQTTTesting(mqttName, "bekens");

	CFG_SetShortDeviceName(shortName);
	CFG_SetDeviceName(fullName);

	CHANNEL_SetType(0, ChType_Voltage_div10);
	CHANNEL_SetType(1, ChType_Power);
	CHANNEL_SetType(2, ChType_Current_div100);

	SIM_ClearMQTTHistory();
	CMD_ExecuteCommand("scheduleHADiscovery 1", 0);
	Sim_RunSeconds(5, false);
	
	// OBK device should publish JSON on MQTT topic "homeassistant"
	/*SELFTEST_ASSERT_HAS_MQTT_JSON_SENT("homeassistant", true);
	SELFTEST_ASSERT_JSON_VALUE_STRING("dev", "name", shortName);
	SELFTEST_ASSERT_JSON_VALUE_STRING("dev", "sw", USER_SW_VER);
	SELFTEST_ASSERT_JSON_VALUE_STRING("dev", "mf", MANUFACTURER);
	SELFTEST_ASSERT_JSON_VALUE_STRING("dev", "mdl", PLATFORM_MCU_NAME);*/

}

void Test_HassDiscovery_Ext() {
	Test_HassDiscovery_TuyaMCU_VoltageCurrentPower();
}


#endif
