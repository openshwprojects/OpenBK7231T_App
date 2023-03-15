#ifdef WINDOWS

#include "selftest_local.h".

void Test_HassDiscovery_Relay_1x() {
	const char *shortName = "WinRelTest1x";
	const char *fullName = "Windows Relay Test 1x";

	SIM_ClearOBK(shortName);
	SIM_ClearAndPrepareForMQTTTesting("testDeviceOneRelay", "bekens");

	CFG_SetShortDeviceName(shortName);
	CFG_SetDeviceName(fullName);

	PIN_SetPinRoleForPinIndex(9, IOR_Relay);
	PIN_SetPinChannelForPinIndex(9, 1);

	SIM_ClearMQTTHistory();
	CMD_ExecuteCommand("scheduleHADiscovery 1", 0);
	Sim_RunSeconds(5, false);

	// OBK device should publish JSON on MQTT topic "homeassistant"
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT("homeassistant",true);
	SELFTEST_ASSERT_JSON_VALUE_STRING("dev", "name", shortName);
	SELFTEST_ASSERT_JSON_VALUE_STRING("dev", "sw", USER_SW_VER);
	SELFTEST_ASSERT_JSON_VALUE_STRING("dev", "mf", MANUFACTURER);
	SELFTEST_ASSERT_JSON_VALUE_STRING("dev", "mdl", PLATFORM_MCU_NAME);
	SELFTEST_ASSERT_JSON_VALUE_STRING(NULL, "avty_t", "~/connected");
	SELFTEST_ASSERT_JSON_VALUE_STRING(NULL, "pl_on", "1");
	SELFTEST_ASSERT_JSON_VALUE_STRING(NULL, "pl_off", "0");
	// we have used channel index 1
	SELFTEST_ASSERT_JSON_VALUE_STRING(NULL, "stat_t", "~/1/get");
	SELFTEST_ASSERT_JSON_VALUE_STRING(NULL, "cmd_t", "~/1/set");
}
void Test_HassDiscovery_Relay_2x() {
	// TODO
}


void Test_HassDiscovery_LED_CW() {
	const char *shortName = "WinCWtest";
	const char *fullName = "Windows Fake CoolWarm";
	const char *mqttName = "testCoolWarm";
	SIM_ClearOBK(shortName);
	SIM_ClearAndPrepareForMQTTTesting(mqttName, "bekens");

	CFG_SetShortDeviceName(shortName);
	CFG_SetDeviceName(fullName);

	PIN_SetPinRoleForPinIndex(24, IOR_PWM);
	PIN_SetPinChannelForPinIndex(24, 1);
	PIN_SetPinRoleForPinIndex(26, IOR_PWM);
	PIN_SetPinChannelForPinIndex(26, 2);

	SIM_ClearMQTTHistory();
	CMD_ExecuteCommand("scheduleHADiscovery 1", 0);
	Sim_RunSeconds(5, false);
	
	// OBK device should publish JSON on MQTT topic "homeassistant"
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT("homeassistant", true);
	SELFTEST_ASSERT_JSON_VALUE_STRING("dev", "name", shortName);
	SELFTEST_ASSERT_JSON_VALUE_STRING("dev", "sw", USER_SW_VER);
	SELFTEST_ASSERT_JSON_VALUE_STRING("dev", "mf", MANUFACTURER);
	SELFTEST_ASSERT_JSON_VALUE_STRING("dev", "mdl", PLATFORM_MCU_NAME);
	SELFTEST_ASSERT_JSON_VALUE_STRING(NULL, "avty_t", "~/connected");
	SELFTEST_ASSERT_JSON_VALUE_STRING(NULL, "pl_on", "1");
	SELFTEST_ASSERT_JSON_VALUE_STRING(NULL, "pl_off", "0");
	// LED funcs
	SELFTEST_ASSERT_JSON_VALUE_STRING(NULL, "on_cmd_type", "first");
	SELFTEST_ASSERT_JSON_VALUE_STRING(NULL, "clr_temp_cmd_t", va("cmnd/%s/led_temperature", mqttName));
	SELFTEST_ASSERT_JSON_VALUE_STRING(NULL, "clr_temp_stat_t", "~/led_temperature/get");
	SELFTEST_ASSERT_JSON_VALUE_STRING(NULL, "stat_t", "~/led_enableAll/get");
	// The line bellow should trigger an assert
	//SELFTEST_ASSERT_JSON_VALUE_STRING(NULL, "stat_t", "~/blabla/get");
	SELFTEST_ASSERT_JSON_VALUE_STRING(NULL, "cmd_t", "cmnd/testCoolWarm/led_enableAll");
	SELFTEST_ASSERT_JSON_VALUE_STRING(NULL, "bri_stat_t", "~/led_dimmer/get");
	SELFTEST_ASSERT_JSON_VALUE_STRING(NULL, "bri_cmd_t", "cmnd/testCoolWarm/led_dimmer");
	SELFTEST_ASSERT_JSON_VALUE_INTEGER(NULL, "bri_scl", 100);
}

void Test_HassDiscovery_LED_RGB() {
	// TODO
}

void Test_HassDiscovery_LED_RGBCW() {
	// TODO
}
void Test_HassDiscovery_LED_SingleColor() {
	// TODO
}
void Test_HassDiscovery_DHT11() {
	// TODO
}
void Test_HassDiscovery_Battery() {
	const char *shortName = "BatteryTest";
	const char *fullName = "Windows Fake Battery";
	const char *mqttName = "testBattery";

	SIM_ClearOBK(shortName);
	SIM_ClearAndPrepareForMQTTTesting(mqttName, "bekens");

	CFG_SetShortDeviceName(shortName);
	CFG_SetDeviceName(fullName);

	PIN_SetPinRoleForPinIndex(24, IOR_BAT_ADC);
	PIN_SetPinChannelForPinIndex(24, 1);
	PIN_SetPinRoleForPinIndex(26, IOR_BAT_Relay);
	PIN_SetPinChannelForPinIndex(26, 2);

	CMD_ExecuteCommand("startDriver Battery", 0);
	SIM_ClearMQTTHistory();

	CMD_ExecuteCommand("scheduleHADiscovery 1", 0);
	Sim_RunSeconds(10, false);


	// OBK device should publish JSON on MQTT topic "homeassistant"
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT("homeassistant", true);
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT_ANY("homeassistant", true, "dev", 0, "name", shortName);
	// first dev - as battery
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT_ANY("homeassistant", true, 0, 0, "dev_cla", "battery");
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT_ANY("homeassistant", true, 0, 0, "unit_of_meas", "%");
	// second dev - as voltage
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT_ANY("homeassistant", true, 0, 0, "dev_cla", "voltage");
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT_ANY("homeassistant", true, 0, 0, "unit_of_meas", "mV");
}
void Test_HassDiscovery_SHTSensor() {
	const char *shortName = "myShortName";
	const char *fullName = "Windows Fake SHT";
	const char *mqttName = "fakeSHT";

	SIM_ClearOBK(shortName);
	SIM_ClearAndPrepareForMQTTTesting(mqttName, "bekens");

	CFG_SetShortDeviceName(shortName);
	CFG_SetDeviceName(fullName);

	PIN_SetPinRoleForPinIndex(24, IOR_SHT3X_CLK);
	PIN_SetPinChannelForPinIndex(24, 1);
	PIN_SetPinRoleForPinIndex(26, IOR_SHT3X_DAT);
	PIN_SetPinChannelForPinIndex(26, 2);

	CMD_ExecuteCommand("startDriver SHT3X", 0);
	SIM_ClearMQTTHistory();

	CMD_ExecuteCommand("scheduleHADiscovery 1", 0);
	Sim_RunSeconds(10, false);

	// first dev -
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT_ANY("homeassistant", true, 0, 0, "dev_cla", "temperature");
	//SELFTEST_ASSERT_HAS_MQTT_JSON_SENT_ANY("homeassistant", true, 0, 0, "unit_of_meas", "°C");
	// second dev - 
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT_ANY("homeassistant", true, 0, 0, "dev_cla", "humidity");
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT_ANY("homeassistant", true, 0, 0, "unit_of_meas", "%");
	
}
void Test_HassDiscovery_BL0942() {
	const char *shortName = "PowerMeteringFake";
	const char *fullName = "Windows Fake BL0942";
	const char *mqttName = "fakeBL0942";

	SIM_ClearOBK(shortName);
	SIM_ClearAndPrepareForMQTTTesting(mqttName, "bekens");

	CFG_SetShortDeviceName(shortName);
	CFG_SetDeviceName(fullName);

	CMD_ExecuteCommand("startDriver BL0942", 0);
	SIM_ClearMQTTHistory();

	CMD_ExecuteCommand("scheduleHADiscovery 1", 0);
	Sim_RunSeconds(10, false);

	// generic tests to see if something power-related was published
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT_ANY("homeassistant", true, 0, 0, "unit_of_meas", "V");
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT_ANY("homeassistant", true, 0, 0, "unit_of_meas", "W");
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT_ANY("homeassistant", true, 0, 0, "unit_of_meas", "Wh");
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT_ANY("homeassistant", true, 0, 0, "unit_of_meas", "A");

	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT_ANY("homeassistant", true, 0, 0, "dev_cla", "voltage");
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT_ANY("homeassistant", true, 0, 0, "dev_cla", "power");
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT_ANY("homeassistant", true, 0, 0, "dev_cla", "current");

	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT_ANY("homeassistant", true, 0, 0, "stat_t", "~/power/get");
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT_ANY("homeassistant", true, 0, 0, "stat_t", "~/current/get");
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT_ANY("homeassistant", true, 0, 0, "stat_t", "~/voltage/get");

}
void Test_HassDiscovery() {
	Test_HassDiscovery_SHTSensor();
	Test_HassDiscovery_BL0942();
	Test_HassDiscovery_Battery();
	Test_HassDiscovery_Relay_1x();
	Test_HassDiscovery_Relay_2x();
	Test_HassDiscovery_LED_CW();
	Test_HassDiscovery_LED_RGB();
	Test_HassDiscovery_LED_RGBCW();
	Test_HassDiscovery_LED_SingleColor();
	Test_HassDiscovery_DHT11();
}


#endif
