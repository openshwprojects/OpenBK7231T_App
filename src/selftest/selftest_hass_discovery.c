#ifdef WINDOWS

#include "selftest_local.h"

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

	const char *shortName = "WinRelTest2x";
	const char *fullName = "Windows Relay Test 2x";

	SIM_ClearOBK(shortName);
	SIM_ClearAndPrepareForMQTTTesting("testDeviceOneRelay", "bekens");

	CFG_SetShortDeviceName(shortName);
	CFG_SetDeviceName(fullName);

	PIN_SetPinRoleForPinIndex(9, IOR_Relay);
	PIN_SetPinChannelForPinIndex(9, 1);

	PIN_SetPinRoleForPinIndex(10, IOR_Relay);
	PIN_SetPinChannelForPinIndex(10, 4);

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
	// we have used channel index 1
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT_ANY("homeassistant", true, 0, 0, "stat_t", "~/1/get");
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT_ANY("homeassistant", true, 0, 0, "cmd_t", "~/1/set");
	// AND we have used channel index 4
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT_ANY("homeassistant", true, 0, 0, "stat_t", "~/4/get");
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT_ANY("homeassistant", true, 0, 0, "cmd_t", "~/4/set");

	// this must cause an assert
	//SELFTEST_ASSERT_HAS_MQTT_JSON_SENT_ANY("homeassistant", true, 0, 0, "cmd_t", "~/5/set");
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
	const char *shortName = "RGBCWtest";
	const char *fullName = "Windows Fake RGBCW";
	const char *mqttName = "testRGBCW";

	SIM_ClearOBK(shortName);
	SIM_ClearAndPrepareForMQTTTesting(mqttName, "bekens");

	PIN_SetPinRoleForPinIndex(24, IOR_PWM);
	PIN_SetPinChannelForPinIndex(24, 1);

	PIN_SetPinRoleForPinIndex(26, IOR_PWM);
	PIN_SetPinChannelForPinIndex(26, 2);

	PIN_SetPinRoleForPinIndex(9, IOR_PWM);
	PIN_SetPinChannelForPinIndex(9, 3);

	PIN_SetPinRoleForPinIndex(6, IOR_PWM);
	PIN_SetPinChannelForPinIndex(6, 4);

	PIN_SetPinRoleForPinIndex(7, IOR_PWM);
	PIN_SetPinChannelForPinIndex(7, 5);

	SIM_ClearMQTTHistory();

	CMD_ExecuteCommand("scheduleHADiscovery 1", 0);
	Sim_RunSeconds(10, false);


	// OBK device should publish JSON on MQTT topic "homeassistant"
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT("homeassistant", true);
	//SELFTEST_ASSERT_HAS_MQTT_JSON_SENT_ANY("homeassistant", true, "dev", 0, "name", shortName);
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT_ANY("homeassistant", true, 0, 0, "bri_cmd_t", "cmnd/testRGBCW/led_dimmer");
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT_ANY("homeassistant", true, 0, 0, "bri_stat_t", "~/led_dimmer/get");
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT_ANY("homeassistant", true, 0, 0, "bri_scl", "100");
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT_ANY("homeassistant", true, 0, 0, "~", mqttName);
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT_ANY("homeassistant", true, 0, 0, "rgb_stat_t", "~/led_basecolor_rgb/get");
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT_ANY("homeassistant", true, 0, 0, "rgb_cmd_t", "cmnd/testRGBCW/led_basecolor_rgb");
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT_ANY("homeassistant", true, 0, 0, "clr_temp_cmd_t", "cmnd/testRGBCW/led_temperature");
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT_ANY("homeassistant", true, 0, 0, "clr_temp_stat_t", "~/led_temperature/get");
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT_ANY("homeassistant", true, 0, 0, "cmd_t", "cmnd/testRGBCW/led_enableAll");
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT_ANY("homeassistant", true, 0, 0, "stat_t", "~/led_enableAll/get");

}
void Test_HassDiscovery_LED_RGBW() {
	const char *shortName = "RGBWtest";
	const char *fullName = "Windows Fake RGBW";
	const char *mqttName = "testRGBW";

	SIM_ClearOBK(shortName);
	SIM_ClearAndPrepareForMQTTTesting(mqttName, "bekens");

	PIN_SetPinRoleForPinIndex(24, IOR_PWM);
	PIN_SetPinChannelForPinIndex(24, 1);

	PIN_SetPinRoleForPinIndex(26, IOR_PWM);
	PIN_SetPinChannelForPinIndex(26, 2);

	PIN_SetPinRoleForPinIndex(9, IOR_PWM);
	PIN_SetPinChannelForPinIndex(9, 3);

	PIN_SetPinRoleForPinIndex(6, IOR_PWM);
	PIN_SetPinChannelForPinIndex(6, 4);

	CFG_SetFlag(OBK_FLAG_LED_EMULATE_COOL_WITH_RGB, 1);

	SIM_ClearMQTTHistory();

	CMD_ExecuteCommand("scheduleHADiscovery 1", 0);
	Sim_RunSeconds(10, false);


	// OBK device should publish JSON on MQTT topic "homeassistant"
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT("homeassistant", true);
	//SELFTEST_ASSERT_HAS_MQTT_JSON_SENT_ANY("homeassistant", true, "dev", 0, "name", shortName);
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT_ANY("homeassistant", true, 0, 0, "bri_cmd_t", "cmnd/testRGBW/led_dimmer");
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT_ANY("homeassistant", true, 0, 0, "bri_stat_t", "~/led_dimmer/get");
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT_ANY("homeassistant", true, 0, 0, "bri_scl", "100");
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT_ANY("homeassistant", true, 0, 0, "~", mqttName);
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT_ANY("homeassistant", true, 0, 0, "rgb_stat_t", "~/led_basecolor_rgb/get");
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT_ANY("homeassistant", true, 0, 0, "rgb_cmd_t", "cmnd/testRGBW/led_basecolor_rgb");
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT_ANY("homeassistant", true, 0, 0, "clr_temp_cmd_t", "cmnd/testRGBW/led_temperature");
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT_ANY("homeassistant", true, 0, 0, "clr_temp_stat_t", "~/led_temperature/get");
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT_ANY("homeassistant", true, 0, 0, "cmd_t", "cmnd/testRGBW/led_enableAll");
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT_ANY("homeassistant", true, 0, 0, "stat_t", "~/led_enableAll/get");

}
void Test_HassDiscovery_LED_SingleColor() {
	// TODO
}
void Test_HassDiscovery_DHT11() {
	const char *shortName = "DHTtest";
	const char *fullName = "Windows Fake DHT11";
	const char *mqttName = "testDHT";
	
	SIM_ClearOBK(shortName);
	SIM_ClearAndPrepareForMQTTTesting(mqttName, "bekens");

	PIN_SetPinRoleForPinIndex(24, IOR_DHT11);
	// for testing purposes, set channels 15 and 25
	PIN_SetPinChannelForPinIndex(24, 15);
	PIN_SetPinChannel2ForPinIndex(24, 25);

	SIM_ClearMQTTHistory();

	CMD_ExecuteCommand("scheduleHADiscovery 1", 0);
	Sim_RunSeconds(10, false);


	// OBK device should publish JSON on MQTT topic "homeassistant"
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT("homeassistant", true);
	//SELFTEST_ASSERT_HAS_MQTT_JSON_SENT_ANY("homeassistant", true, "dev", 0, "name", shortName);
	// first dev - as temperature
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT_ANY("homeassistant", true, 0, 0, "dev_cla", "temperature");
	//SELFTEST_ASSERT_HAS_MQTT_JSON_SENT_ANY("homeassistant", true, 0, 0, "unit_of_meas", "°C");
	// old method - round
	//SELFTEST_ASSERT_HAS_MQTT_JSON_SENT_ANY("homeassistant", true, 0, 0, "val_tpl", "{{ float(value)*0.1|round(2) }}");
	// new method - format
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT_ANY("homeassistant", true, 0, 0, "val_tpl", "{{ '%0.2f'|format(float(value)*0.1) }}");
	// second dev - humidity
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT_ANY("homeassistant", true, 0, 0, "dev_cla", "humidity");
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT_ANY("homeassistant", true, 0, 0, "unit_of_meas", "%");
	// few lines above I have assigned channels 15 and 25 to humidity and temperature,
	// let's see if their setting made to HA discovery correctly
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT_ANY("homeassistant", true, 0, 0, "stat_t", "~/25/get");
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT_ANY("homeassistant", true, 0, 0, "stat_t", "~/15/get");

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
void Test_HassDiscovery_BL0937() {
	const char *shortName = "PowerMeteringFake";
	const char *fullName = "Windows Fake BL0937";
	const char *mqttName = "fakeBL0937";

	SIM_ClearOBK(shortName);
	SIM_ClearAndPrepareForMQTTTesting(mqttName, "bekens");

	CFG_SetShortDeviceName(shortName);
	CFG_SetDeviceName(fullName);

	PIN_SetPinRoleForPinIndex(24, IOR_BL0937_CF);
	PIN_SetPinRoleForPinIndex(25, IOR_BL0937_SEL);
	PIN_SetPinRoleForPinIndex(26, IOR_BL0937_CF1);

	CMD_ExecuteCommand("startDriver BL0937", 0);
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
void Test_HassDiscovery_digitalInput() {
	const char *shortName = "DigitalInputTest";
	const char *fullName = "Windows Fake DINput";
	const char *mqttName = "fakeDin";

	SIM_ClearOBK(shortName);
	SIM_ClearAndPrepareForMQTTTesting(mqttName, "bekens");

	CFG_SetShortDeviceName(shortName);
	CFG_SetDeviceName(fullName);

	PIN_SetPinRoleForPinIndex(24, IOR_DigitalInput);
	PIN_SetPinChannelForPinIndex(24, 5);

	// this label will be sent via HASS Discovery to Home Assistant
	CMD_ExecuteCommand("setChannelLabel 5 myDigitalValue", 0);
	CMD_ExecuteCommand("scheduleHADiscovery 1", 0);
	Sim_RunSeconds(10, false);

	// generic tests to see if something power-related was published
	// OBK device should publish JSON on MQTT topic "homeassistant"
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT("homeassistant", true);
	SELFTEST_ASSERT_JSON_VALUE_STRING("dev", "name", shortName);
	SELFTEST_ASSERT_JSON_VALUE_STRING("dev", "sw", USER_SW_VER);
	SELFTEST_ASSERT_JSON_VALUE_STRING("dev", "mf", MANUFACTURER);
	SELFTEST_ASSERT_JSON_VALUE_STRING("dev", "mdl", PLATFORM_MCU_NAME);
	SELFTEST_ASSERT_JSON_VALUE_STRING(NULL, "avty_t", "~/connected");
	SELFTEST_ASSERT_JSON_VALUE_STRING(NULL, "pl_on", "0");
	SELFTEST_ASSERT_JSON_VALUE_STRING(NULL, "pl_off", "1");
	// we have used channel index 1
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT_ANY("homeassistant", true, 0, 0, "stat_t", "~/5/get");
	// this label will be sent via HASS Discovery to Home Assistant
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT_ANY("homeassistant", true, 0, 0, "name", "myDigitalValue");

}

void Test_HassDiscovery_digitalInputNoAVTY() {
	const char *shortName = "DigitalInputTest";
	const char *fullName = "Windows Fake DINput";
	const char *mqttName = "fakeDin";

	SIM_ClearOBK(shortName);
	SIM_ClearAndPrepareForMQTTTesting(mqttName, "bekens");

	CFG_SetShortDeviceName(shortName);
	CFG_SetDeviceName(fullName);

	CFG_SetFlag(OBK_FLAG_NOT_PUBLISH_AVAILABILITY, true); // disabled avty

	PIN_SetPinRoleForPinIndex(24, IOR_DigitalInput);
	PIN_SetPinChannelForPinIndex(24, 5);

	// this label will be sent via HASS Discovery to Home Assistant
	CMD_ExecuteCommand("setChannelLabel 5 myDigitalValue", 0);
	CMD_ExecuteCommand("scheduleHADiscovery 1", 0);
	Sim_RunSeconds(10, false);

	// generic tests to see if something power-related was published
	// OBK device should publish JSON on MQTT topic "homeassistant"
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT("homeassistant", true);
	SELFTEST_ASSERT_JSON_VALUE_STRING("dev", "name", shortName);
	SELFTEST_ASSERT_JSON_VALUE_STRING("dev", "sw", USER_SW_VER);
	SELFTEST_ASSERT_JSON_VALUE_STRING("dev", "mf", MANUFACTURER);
	SELFTEST_ASSERT_JSON_VALUE_STRING("dev", "mdl", PLATFORM_MCU_NAME);
	SELFTEST_ASSERT_JSON_VALUE_STRING_NOT_PRESENT(NULL, "avty_t"); // disabled avty
	SELFTEST_ASSERT_JSON_VALUE_STRING(NULL, "pl_on", "0");
	SELFTEST_ASSERT_JSON_VALUE_STRING(NULL, "pl_off", "1");
	// we have used channel index 1
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT_ANY("homeassistant", true, 0, 0, "stat_t", "~/5/get");
	// this label will be sent via HASS Discovery to Home Assistant
	SELFTEST_ASSERT_HAS_MQTT_JSON_SENT_ANY("homeassistant", true, 0, 0, "name", "myDigitalValue");

}

void Test_HassDiscovery() {
	Test_HassDiscovery_SHTSensor();
	Test_HassDiscovery_BL0942();
	Test_HassDiscovery_BL0937();
	Test_HassDiscovery_Battery();
	Test_HassDiscovery_Relay_1x();
	Test_HassDiscovery_Relay_2x();
	Test_HassDiscovery_LED_CW();
	Test_HassDiscovery_LED_RGB();
	Test_HassDiscovery_LED_RGBCW();
	Test_HassDiscovery_LED_SingleColor();
	Test_HassDiscovery_LED_RGBW();
	Test_HassDiscovery_DHT11();
	Test_HassDiscovery_digitalInput();
	Test_HassDiscovery_digitalInputNoAVTY();
}


#endif
