#ifdef WINDOWS

#include "selftest_local.h"

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

void Test_HassDiscovery_TuyaMCU_Power10() {
	const char *shortName = "WinTuyatest";
	const char *fullName = "Windows Fake Tuya";
	const char *mqttName = "testTuya";
	SIM_ClearOBK(shortName);
	SIM_ClearAndPrepareForMQTTTesting(mqttName, "bekens");

	CFG_SetShortDeviceName(shortName);
	CFG_SetDeviceName(fullName);

	CHANNEL_SetType(1, ChType_Power_div10);

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
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "name", "Humidity");
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
	//SELFTEST_ASSERT_JSON_VALUE_STRING(0, "val_tpl", "{{ float(value)*0.01|round(3) }}");
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "val_tpl", "{{ '%0.3f'|format(float(value)*0.01) }}");
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
	//SELFTEST_ASSERT_JSON_VALUE_STRING(0, "val_tpl", "{{ float(value)*0.001|round(3) }}");
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "val_tpl", "{{ '%0.3f'|format(float(value)*0.001) }}");
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
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "name", "Voltage");
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "~", mqttName);
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "dev_cla", "voltage");
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "unit_of_meas", "V");
	//SELFTEST_ASSERT_JSON_VALUE_STRING(0, "val_tpl", "{{ float(value)*0.1|round(2) }}");
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "val_tpl", "{{ '%0.2f'|format(float(value)*0.1) }}");
	
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
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "name", "Temperature");
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
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "name", "Temperature");
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "~", mqttName);
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "dev_cla", "temperature");
	//SELFTEST_ASSERT_JSON_VALUE_STRING(0, "unit_of_meas", "C");
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "stat_t", "~/3/get");
	//SELFTEST_ASSERT_JSON_VALUE_STRING(0, "val_tpl", "{{ float(value)*0.1|round(2) }}");
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "val_tpl", "{{ '%0.2f'|format(float(value)*0.1) }}");
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
void Test_HassDiscovery_Channel_Illuminance() {
	const char *shortName = "WinIlluminanceTest";
	const char *fullName = "Windows Fake Illuminance";
	const char *mqttName = "testIlluminance";
	SIM_ClearOBK(shortName);
	SIM_ClearAndPrepareForMQTTTesting(mqttName, "bekens");

	CFG_SetShortDeviceName(shortName);
	CFG_SetDeviceName(fullName);

	CHANNEL_SetType(4, ChType_Illuminance);

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
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "dev_cla", "illuminance");
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "unit_of_meas", "lx");
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "stat_t", "~/4/get");
}
void Test_HassDiscovery_Channel_Motion() {
	const char *shortName = "WinMotionTest";
	const char *fullName = "Windows Fake Motion";
	const char *mqttName = "testMotion";
	SIM_ClearOBK(shortName);
	SIM_ClearAndPrepareForMQTTTesting(mqttName, "bekens");

	CFG_SetShortDeviceName(shortName);
	CFG_SetDeviceName(fullName);

	CHANNEL_SetType(4, ChType_Motion);

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
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "dev_cla", "motion");
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "stat_t", "~/4/get");
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "pl_on", "1");
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "pl_off", "0");
}
void Test_HassDiscovery_Channel_Motion_inv() {
	const char *shortName = "WinMotionTest";
	const char *fullName = "Windows Fake Motion";
	const char *mqttName = "testMotion";
	SIM_ClearOBK(shortName);
	SIM_ClearAndPrepareForMQTTTesting(mqttName, "bekens");

	CFG_SetShortDeviceName(shortName);
	CFG_SetDeviceName(fullName);

	CHANNEL_SetType(4, ChType_Motion_n);

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
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "dev_cla", "motion");
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "stat_t", "~/4/get");
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "pl_on", "0"); // inv so swapped
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "pl_off", "1"); // inv so swapped
}
void Test_HassDiscovery_Channel_LowMidHigh() {
	const char *shortName = "WinReadOnlyLowMidHighTest";
	const char *fullName = "Windows Fake ReadOnlyLowMidHigh";
	const char *mqttName = "testReadOnlyLowMidHigh";
	SIM_ClearOBK(shortName);
	SIM_ClearAndPrepareForMQTTTesting(mqttName, "bekens");

	CFG_SetShortDeviceName(shortName);
	CFG_SetDeviceName(fullName);

	CHANNEL_SetType(4, ChType_ReadOnlyLowMidHigh);

	SIM_ClearMQTTHistory();
	CMD_ExecuteCommand("scheduleHADiscovery 1", 0);
	Sim_RunSeconds(5, false);

	// OBK device should publish JSON on MQTT topic "homeassistant"
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT("homeassistant", true);
	SELFTEST_ASSERT_JSON_VALUE_STRING("dev", "name", shortName);
	SELFTEST_ASSERT_JSON_VALUE_STRING("dev", "sw", USER_SW_VER);
	SELFTEST_ASSERT_JSON_VALUE_STRING("dev", "mf", MANUFACTURER);
	SELFTEST_ASSERT_JSON_VALUE_STRING("dev", "mdl", PLATFORM_MCU_NAME);

}
void Test_HassDiscovery_Channel_Custom() {
	const char *shortName = "WinCustom";
	const char *fullName = "Windows Fake Custom";
	const char *mqttName = "testCustom";
	SIM_ClearOBK(shortName);
	SIM_ClearAndPrepareForMQTTTesting(mqttName, "bekens");

	CFG_SetShortDeviceName(shortName);
	CFG_SetDeviceName(fullName);

	CHANNEL_SetType(4, ChType_Custom);

	SIM_ClearMQTTHistory();
	CMD_ExecuteCommand("scheduleHADiscovery 1", 0);
	Sim_RunSeconds(5, false);

	// OBK device should publish JSON on MQTT topic "homeassistant"
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT("homeassistant", true);
	SELFTEST_ASSERT_JSON_VALUE_STRING("dev", "name", shortName);
	SELFTEST_ASSERT_JSON_VALUE_STRING("dev", "sw", USER_SW_VER);
	SELFTEST_ASSERT_JSON_VALUE_STRING("dev", "mf", MANUFACTURER);
	SELFTEST_ASSERT_JSON_VALUE_STRING("dev", "mdl", PLATFORM_MCU_NAME);

	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "stat_t", "~/4/get");
}
void Test_HassDiscovery_Channel_Motion_longName() {
	const char *shortName = "multifunction_PIR_obkE1552B06";
	const char *fullName = "multifunction_PIR_OpenBK7231N_E1552B06";
	const char *mqttName = "testMotion";
	SIM_ClearOBK(shortName);
	SIM_ClearAndPrepareForMQTTTesting(mqttName, "bekens");

	CFG_SetShortDeviceName(shortName);
	CFG_SetDeviceName(fullName);

	CHANNEL_SetType(4, ChType_Motion);

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
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "dev_cla", "motion");
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "stat_t", "~/4/get");
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "pl_on", "1");
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "pl_off", "0");
}
void Test_HassDiscovery_Channel_Motion_With_dInput() {
	const char *shortName = "WinMotionTest";
	const char *fullName = "Windows Fake Motion";
	const char *mqttName = "testMotion";
	SIM_ClearOBK(shortName);
	SIM_ClearAndPrepareForMQTTTesting(mqttName, "bekens");

	CFG_SetShortDeviceName(shortName);
	CFG_SetDeviceName(fullName);

	CHANNEL_SetType(4, ChType_Motion);

	// dInput should not SHADOW the channel type
	PIN_SetPinRoleForPinIndex(10, IOR_DigitalInput);
	PIN_SetPinChannelForPinIndex(10, 4);

	PIN_SetPinRoleForPinIndex(11, IOR_LED_n);
	PIN_SetPinChannelForPinIndex(11, 4);

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
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "dev_cla", "motion");
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "stat_t", "~/4/get");
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "pl_on", "1");
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "pl_off", "0");
}
void Test_HassDiscovery_Channel_DimmerLightDetection_Dual() {
	const char *shortName = "DualMCUDimmer";
	const char *fullName = "WinDualMCUDimmer";
	const char *mqttName = "dualDimmer";
	SIM_ClearOBK(shortName);
	SIM_ClearAndPrepareForMQTTTesting(mqttName, "bekens");

	CFG_SetShortDeviceName(shortName);
	CFG_SetDeviceName(fullName);

	CHANNEL_SetType(2, ChType_Toggle);
	CHANNEL_SetType(3, ChType_Dimmer);

	CHANNEL_SetType(4, ChType_Toggle);
	CHANNEL_SetType(5, ChType_Dimmer);

	SIM_ClearMQTTHistory();
	CMD_ExecuteCommand("scheduleHADiscovery 1", 0);
	Sim_RunSeconds(5, false);

	// homeassistant/light/WinDualMCUDimmer_light_2/config

	// OBK device should publish JSON on MQTT topic "homeassistant"
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT("homeassistant/light/WinDualMCUDimmer_light_2/config", true);
	SELFTEST_ASSERT_JSON_VALUE_STRING("dev", "name", shortName);
	SELFTEST_ASSERT_JSON_VALUE_STRING("dev", "sw", USER_SW_VER);
	SELFTEST_ASSERT_JSON_VALUE_STRING("dev", "mf", MANUFACTURER);
	SELFTEST_ASSERT_JSON_VALUE_STRING("dev", "mdl", PLATFORM_MCU_NAME);
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "~", mqttName);
	//SELFTEST_ASSERT_JSON_VALUE_STRING(0, "unit_of_meas", "C");
	// state topic (toggle)
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "stat_t", "~/2/get");
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "cmd_t", "~/2/set");
	// brightness topic (dimmer)
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "bri_stat_t", "~/3/get");
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "bri_cmd_t", "~/3/set");
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "pl_on", "1");
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "pl_off", "0");

	// OBK device should publish JSON on MQTT topic "homeassistant"
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT("homeassistant/light/WinDualMCUDimmer_light_4/config", true);
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
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "bri_stat_t", "~/5/get");
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "bri_cmd_t", "~/5/set");
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
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT("homeassistant/light", true);
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
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "bri_stat_t", "~/5/get");
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
	Test_HassDiscovery_TuyaMCU_Power10();
	Test_HassDiscovery_Channel_Humidity();
	Test_HassDiscovery_Channel_Temperature();
	Test_HassDiscovery_Channel_Temperature_div10();
	Test_HassDiscovery_Channel_Voltage_div10();
	Test_HassDiscovery_Channel_Current_div100();
	Test_HassDiscovery_Channel_Current_div1000();
	Test_HassDiscovery_Channel_Toggle();
	Test_HassDiscovery_Channel_Toggle_2x();
	Test_HassDiscovery_Channel_DimmerLightDetection();
	Test_HassDiscovery_Channel_DimmerLightDetection_Dual();
	Test_HassDiscovery_Channel_Motion();
	Test_HassDiscovery_Channel_Motion_With_dInput();
	Test_HassDiscovery_Channel_Motion_longName();
	Test_HassDiscovery_Channel_Motion_inv();
	Test_HassDiscovery_Channel_Illuminance();
	Test_HassDiscovery_Channel_LowMidHigh();
	Test_HassDiscovery_Channel_Custom();


}


#endif
