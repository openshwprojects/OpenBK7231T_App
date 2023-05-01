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


void Test_HassDiscovery_Channel_Humidity() {
	const char *shortName = "WinHumTest";
	const char *fullName = "Windows Fake Hum";
	const char *mqttName = "testHum";
	SIM_ClearOBK(shortName);
	SIM_ClearAndPrepareForMQTTTesting(mqttName, "bekens");

	CFG_SetShortDeviceName(shortName);
	CFG_SetDeviceName(fullName);

	// in this example, i am using channel 15
	CHANNEL_SetType(15, ChType_Humidity);

	SIM_ClearMQTTHistory();
	CMD_ExecuteCommand("scheduleHADiscovery 1", 0);
	Sim_RunSeconds(5, false);

	// OBK device should publish JSON on MQTT topic "homeassistant"
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT("homeassistant", true);
	SELFTEST_ASSERT_JSON_VALUE_STRING("dev", "name", shortName);
	SELFTEST_ASSERT_JSON_VALUE_STRING("dev", "sw", USER_SW_VER);
	SELFTEST_ASSERT_JSON_VALUE_STRING("dev", "mf", MANUFACTURER);
	SELFTEST_ASSERT_JSON_VALUE_STRING("dev", "mdl", PLATFORM_MCU_NAME);
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "name", "WinHumTest Humidity");
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "~", mqttName);
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "dev_cla", "humidity");
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "unit_of_meas", "%");
	// in this example, i am using channel 15
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "stat_t", "~/15/get");
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "stat_cla", "measurement");
}
void Test_HassDiscovery_Channel_Temperature() {
	const char *shortName = "WinTempTest";
	const char *fullName = "Windows Fake Temp";
	const char *mqttName = "testTemp";
	SIM_ClearOBK(shortName);
	SIM_ClearAndPrepareForMQTTTesting(mqttName, "bekens");

	CFG_SetShortDeviceName(shortName);
	CFG_SetDeviceName(fullName);

	CHANNEL_SetType(0, ChType_Temperature);

	SIM_ClearMQTTHistory();
	CMD_ExecuteCommand("scheduleHADiscovery 1", 0);
	Sim_RunSeconds(5, false);

	// OBK device should publish JSON on MQTT topic "homeassistant"
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT("homeassistant", true);
	SELFTEST_ASSERT_JSON_VALUE_STRING("dev", "name", shortName);
	SELFTEST_ASSERT_JSON_VALUE_STRING("dev", "sw", USER_SW_VER);
	SELFTEST_ASSERT_JSON_VALUE_STRING("dev", "mf", MANUFACTURER);
	SELFTEST_ASSERT_JSON_VALUE_STRING("dev", "mdl", PLATFORM_MCU_NAME);
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "name", "WinTempTest Temperature");
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "~", mqttName);
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "dev_cla", "temperature");
	//SELFTEST_ASSERT_JSON_VALUE_STRING(0, "unit_of_meas", "C");
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "stat_t", "~/0/get");
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "stat_cla", "measurement");
}
void Test_HassDiscovery_Ext() {
	Test_HassDiscovery_TuyaMCU_VoltageCurrentPower();
	Test_HassDiscovery_Channel_Humidity();
	Test_HassDiscovery_Channel_Temperature();
}


#endif
