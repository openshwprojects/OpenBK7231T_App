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
void Test_HassDiscovery_Channel_Current_div100() {
	const char *shortName = "WinCurTest";
	const char *fullName = "Windows Fake Cur";
	const char *mqttName = "testCur";
	SIM_ClearOBK(shortName);
	SIM_ClearAndPrepareForMQTTTesting(mqttName, "bekens");

	CFG_SetShortDeviceName(shortName);
	CFG_SetDeviceName(fullName);

	// in this example, i am using channel 4
	CHANNEL_SetType(4, ChType_Current_div100);

	SIM_ClearMQTTHistory();
	CMD_ExecuteCommand("scheduleHADiscovery 1", 0);
	Sim_RunSeconds(5, false);

	// OBK device should publish JSON on MQTT topic "homeassistant"
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT("homeassistant", true);
	SELFTEST_ASSERT_JSON_VALUE_STRING("dev", "name", shortName);
	SELFTEST_ASSERT_JSON_VALUE_STRING("dev", "sw", USER_SW_VER);
	SELFTEST_ASSERT_JSON_VALUE_STRING("dev", "mf", MANUFACTURER);
	SELFTEST_ASSERT_JSON_VALUE_STRING("dev", "mdl", PLATFORM_MCU_NAME);
	//SELFTEST_ASSERT_JSON_VALUE_STRING(0, "name", "WinCurTest Current");
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "~", mqttName);
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "dev_cla", "current");
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "unit_of_meas", "A");
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "val_tpl", "{{ float(value)*0.01|round(3) }}");
	// in this example, i am using channel 4
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "stat_t", "~/4/get");
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "stat_cla", "measurement");
}
void Test_HassDiscovery_Channel_Current_div1000() {
	const char *shortName = "WinCurTest";
	const char *fullName = "Windows Fake Cur";
	const char *mqttName = "testCur";
	SIM_ClearOBK(shortName);
	SIM_ClearAndPrepareForMQTTTesting(mqttName, "bekens");

	CFG_SetShortDeviceName(shortName);
	CFG_SetDeviceName(fullName);

	// in this example, i am using channel 14
	CHANNEL_SetType(14, ChType_Current_div1000);

	SIM_ClearMQTTHistory();
	CMD_ExecuteCommand("scheduleHADiscovery 1", 0);
	Sim_RunSeconds(5, false);

	// OBK device should publish JSON on MQTT topic "homeassistant"
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT("homeassistant", true);
	SELFTEST_ASSERT_JSON_VALUE_STRING("dev", "name", shortName);
	SELFTEST_ASSERT_JSON_VALUE_STRING("dev", "sw", USER_SW_VER);
	SELFTEST_ASSERT_JSON_VALUE_STRING("dev", "mf", MANUFACTURER);
	SELFTEST_ASSERT_JSON_VALUE_STRING("dev", "mdl", PLATFORM_MCU_NAME);
	//SELFTEST_ASSERT_JSON_VALUE_STRING(0, "name", "WinCurTest Current");
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "~", mqttName);
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "dev_cla", "current");
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "unit_of_meas", "A");
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "val_tpl", "{{ float(value)*0.001|round(3) }}");
	// in this example, i am using channel 14
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "stat_t", "~/14/get");
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "stat_cla", "measurement");
}
void Test_HassDiscovery_Channel_Voltage_div10() {
	const char *shortName = "WinVolTest";
	const char *fullName = "Windows Fake Vol";
	const char *mqttName = "testVol";
	SIM_ClearOBK(shortName);
	SIM_ClearAndPrepareForMQTTTesting(mqttName, "bekens");

	CFG_SetShortDeviceName(shortName);
	CFG_SetDeviceName(fullName);

	// in this example, i am using channel 15
	CHANNEL_SetType(23, ChType_Voltage_div10);

	SIM_ClearMQTTHistory();
	CMD_ExecuteCommand("scheduleHADiscovery 1", 0);
	Sim_RunSeconds(5, false);

	// OBK device should publish JSON on MQTT topic "homeassistant"
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT("homeassistant", true);
	SELFTEST_ASSERT_JSON_VALUE_STRING("dev", "name", shortName);
	SELFTEST_ASSERT_JSON_VALUE_STRING("dev", "sw", USER_SW_VER);
	SELFTEST_ASSERT_JSON_VALUE_STRING("dev", "mf", MANUFACTURER);
	SELFTEST_ASSERT_JSON_VALUE_STRING("dev", "mdl", PLATFORM_MCU_NAME);
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "name", "WinVolTest Voltage");
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "~", mqttName);
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "dev_cla", "voltage");
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "unit_of_meas", "V");
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "val_tpl", "{{ float(value)*0.1|round(2) }}");
	// in this example, i am using channel 23
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "stat_t", "~/23/get");
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
void Test_HassDiscovery_Channel_Temperature_div10() {
	const char *shortName = "WinTempTest";
	const char *fullName = "Windows Fake Temp";
	const char *mqttName = "testTemp";
	SIM_ClearOBK(shortName);
	SIM_ClearAndPrepareForMQTTTesting(mqttName, "bekens");

	CFG_SetShortDeviceName(shortName);
	CFG_SetDeviceName(fullName);

	CHANNEL_SetType(3, ChType_Temperature_div10);

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
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "stat_t", "~/3/get");
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "val_tpl", "{{ float(value)*0.1|round(2) }}");
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "stat_cla", "measurement");
}
void Test_HassDiscovery_Channel_Toggle() {
	const char *shortName = "WinToggleTest";
	const char *fullName = "Windows Fake Toggle";
	const char *mqttName = "testToggle";
	SIM_ClearOBK(shortName);
	SIM_ClearAndPrepareForMQTTTesting(mqttName, "bekens");

	CFG_SetShortDeviceName(shortName);
	CFG_SetDeviceName(fullName);

	CHANNEL_SetType(4, ChType_Toggle);

	SIM_ClearMQTTHistory();
	CMD_ExecuteCommand("scheduleHADiscovery 1", 0);
	Sim_RunSeconds(5, false);

	// OBK device should publish JSON on MQTT topic "homeassistant"
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT("homeassistant", true);
	SELFTEST_ASSERT_JSON_VALUE_STRING("dev", "name", shortName);
	SELFTEST_ASSERT_JSON_VALUE_STRING("dev", "sw", USER_SW_VER);
	SELFTEST_ASSERT_JSON_VALUE_STRING("dev", "mf", MANUFACTURER);
	SELFTEST_ASSERT_JSON_VALUE_STRING("dev", "mdl", PLATFORM_MCU_NAME);
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "~", mqttName);
	//SELFTEST_ASSERT_JSON_VALUE_STRING(0, "unit_of_meas", "C");
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "stat_t", "~/4/get");
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "cmd_t", "~/4/set");
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "pl_on", "1");
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "pl_off", "0");
}
void Test_HassDiscovery_Channel_DimmerLightDetection() {
	const char *shortName = "WinDimmerTest";
	const char *fullName = "Windows Fake Dimmer";
	const char *mqttName = "testDimmer";
	SIM_ClearOBK(shortName);
	SIM_ClearAndPrepareForMQTTTesting(mqttName, "bekens");

	CFG_SetShortDeviceName(shortName);
	CFG_SetDeviceName(fullName);

	CHANNEL_SetType(4, ChType_Toggle);
	CHANNEL_SetType(5, ChType_Dimmer);

	SIM_ClearMQTTHistory();
	CMD_ExecuteCommand("scheduleHADiscovery 1", 0);
	Sim_RunSeconds(5, false);

	// OBK device should publish JSON on MQTT topic "homeassistant"
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT("homeassistant", true);
	SELFTEST_ASSERT_JSON_VALUE_STRING("dev", "name", shortName);
	SELFTEST_ASSERT_JSON_VALUE_STRING("dev", "sw", USER_SW_VER);
	SELFTEST_ASSERT_JSON_VALUE_STRING("dev", "mf", MANUFACTURER);
	SELFTEST_ASSERT_JSON_VALUE_STRING("dev", "mdl", PLATFORM_MCU_NAME);
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "~", mqttName);
	//SELFTEST_ASSERT_JSON_VALUE_STRING(0, "unit_of_meas", "C");
	// state topic (toggle)
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "stat_t", "~/4/get");
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "cmd_t", "~/4/set");
	// brightness topic (dimmer)
#if 0
	// cause error
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "bri_stat_t", "~/4/get");
#else
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "bri_stat_t", "~/5/get");
#endif
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "bri_cmd_t", "~/5/set");
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "pl_on", "1");
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "pl_off", "0");
}
void Test_HassDiscovery_Channel_Toggle_2x() {
	const char *shortName = "WinToggleTest";
	const char *fullName = "Windows Fake Toggle";
	const char *mqttName = "testToggle";
	SIM_ClearOBK(shortName);
	SIM_ClearAndPrepareForMQTTTesting(mqttName, "bekens");

	CFG_SetShortDeviceName(shortName);
	CFG_SetDeviceName(fullName);

	CHANNEL_SetType(4, ChType_Toggle);
	CHANNEL_SetType(5, ChType_Toggle);

	SIM_ClearMQTTHistory();
	CMD_ExecuteCommand("scheduleHADiscovery 1", 0);
	Sim_RunSeconds(5, false);

	// OBK device should publish JSON on MQTT topic "homeassistant"
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT("homeassistant", true);
	SELFTEST_ASSERT_JSON_VALUE_STRING("dev", "name", shortName);
	SELFTEST_ASSERT_JSON_VALUE_STRING("dev", "sw", USER_SW_VER);
	SELFTEST_ASSERT_JSON_VALUE_STRING("dev", "mf", MANUFACTURER);
	SELFTEST_ASSERT_JSON_VALUE_STRING("dev", "mdl", PLATFORM_MCU_NAME);
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "~", mqttName);
	//SELFTEST_ASSERT_JSON_VALUE_STRING(0, "unit_of_meas", "C");
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT_ANY("homeassistant", true, 0, 0, "pl_on", "1");
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT_ANY("homeassistant", true, 0, 0, "pl_off", "0");
	// we have used channel index 4
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT_ANY("homeassistant", true, 0, 0, "stat_t", "~/4/get");
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT_ANY("homeassistant", true, 0, 0, "cmd_t", "~/4/set");
	// AND we have used channel index 5
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT_ANY("homeassistant", true, 0, 0, "stat_t", "~/5/get");
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT_ANY("homeassistant", true, 0, 0, "cmd_t", "~/5/set");
}
void Test_HassDiscovery_Ext() {
	Test_HassDiscovery_TuyaMCU_VoltageCurrentPower();
	Test_HassDiscovery_Channel_Humidity();
	Test_HassDiscovery_Channel_Temperature();
	Test_HassDiscovery_Channel_Temperature_div10();
	Test_HassDiscovery_Channel_Voltage_div10();
	Test_HassDiscovery_Channel_Current_div100();
	Test_HassDiscovery_Channel_Current_div1000();
	Test_HassDiscovery_Channel_Toggle();
	Test_HassDiscovery_Channel_Toggle_2x();
	Test_HassDiscovery_Channel_DimmerLightDetection();
}


#endif
