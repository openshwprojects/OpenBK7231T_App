#ifdef WINDOWS

#include "selftest_local.h"
#include "../hal/hal_wifi.h"

void SIM_ClearAndPrepareForMQTTTesting(const char *clientName, const char *groupName) {
	SIM_ClearOBK();
	SIM_ClearMQTTHistory();
	Main_OnWiFiStatusChange(WIFI_STA_CONNECTED);
	CFG_SetMQTTClientId(clientName);
	CFG_SetMQTTGroupTopic(groupName);
	MQTT_init();

	for (int i = 0; i < 20; i++) {
		MQTT_RunEverySecondUpdate();
	}
}
void Test_MQTT_Get_And_Reply() {
	SIM_ClearOBK();
	SIM_ClearAndPrepareForMQTTTesting("myTestDevice", "bekens");
	char buffer[512];
	char buffer2[512];

	int maxChannelsToTest = 15;

	for (int i = 0; i < maxChannelsToTest; i++) {
		sprintf(buffer, "setChannel %i %i", i, (i * 7 + 10));
		CMD_ExecuteCommand(buffer, 0);
	}

	SIM_ClearMQTTHistory();
	for (int i = 0; i < maxChannelsToTest; i++) {
		// send get
		sprintf(buffer2, "myTestDevice/%i/get", i);
		SIM_SendFakeMQTT(buffer2, "");

		// expect get reply
		int expected = (i * 7 + 10);
		sprintf(buffer, "%i", expected);
		sprintf(buffer2, "myTestDevice/%i/get", i);
		SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR(buffer2, buffer, false);
		SIM_ClearMQTTHistory();
	}
}
void Test_MQTT_Channels() {
	SIM_ClearOBK();
	SIM_ClearAndPrepareForMQTTTesting("myTestDevice", "bekens");

	PIN_SetPinRoleForPinIndex(9, IOR_Relay);
	PIN_SetPinChannelForPinIndex(9, 1);

	// This should trigger MQTT publish
	CMD_ExecuteCommand("setChannel 1 1", 0);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("myTestDevice/1/get", "1", false);
	// if assert has passed, we can clear SIM MQTT history, it's no longer needed
	SIM_ClearMQTTHistory();

	// This should trigger MQTT publish
	CMD_ExecuteCommand("setChannel 1 0", 0);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("myTestDevice/1/get", "0", false);
	// if assert has passed, we can clear SIM MQTT history, it's no longer needed
	SIM_ClearMQTTHistory();

	// This should trigger MQTT publish
	CMD_ExecuteCommand("setChannel 1 1", 0);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("myTestDevice/1/get", "1", false);
	// if assert has passed, we can clear SIM MQTT history, it's no longer needed
	SIM_ClearMQTTHistory();

	// This should trigger MQTT publish
	CMD_ExecuteCommand("setChannel 1 0", 0);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("myTestDevice/1/get", "0", false);
	// if assert has passed, we can clear SIM MQTT history, it's no longer needed
	SIM_ClearMQTTHistory();

	// This should trigger MQTT publish
	SIM_SendFakeMQTTRawChannelSet(1, "toggle");
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("myTestDevice/1/get", "1", false);
	// if assert has passed, we can clear SIM MQTT history, it's no longer needed
	SIM_ClearMQTTHistory();

	// This should trigger MQTT publish
	SIM_SendFakeMQTTRawChannelSet(1, "toggle");
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("myTestDevice/1/get", "0", false);
	// if assert has passed, we can clear SIM MQTT history, it's no longer needed
	SIM_ClearMQTTHistory();

	// This should trigger MQTT publish
	SIM_SendFakeMQTTRawChannelSet(1, "toggle");
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("myTestDevice/1/get", "1", false);
	// if assert has passed, we can clear SIM MQTT history, it's no longer needed
	SIM_ClearMQTTHistory();

	// This should trigger MQTT publish
	CMD_ExecuteCommand("setChannel 1 0", 0);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("myTestDevice/1/get", "0", false);
	// if assert has passed, we can clear SIM MQTT history, it's no longer needed
	SIM_ClearMQTTHistory();

	// This should trigger MQTT publish
	CMD_ExecuteCommand("publish myTestVal 123", 0);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("myTestDevice/myTestVal/get", "123", false);
	// if assert has passed, we can clear SIM MQTT history, it's no longer needed
	SIM_ClearMQTTHistory();


	CMD_ExecuteCommand("setChannel 15 505", 0);
	// This should trigger MQTT publish
	CMD_ExecuteCommand("publish anotherVal $CH15", 0);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("myTestDevice/anotherVal/get", "505", false);
	// if assert has passed, we can clear SIM MQTT history, it's no longer needed
	SIM_ClearMQTTHistory();

	// This should trigger MQTT publish
	CMD_ExecuteCommand("publishInt anotherVal $CH15*10", 0);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("myTestDevice/anotherVal/get", "5050", false);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_FLOAT("myTestDevice/anotherVal/get", 5050, false);
	// if assert has passed, we can clear SIM MQTT history, it's no longer needed
	SIM_ClearMQTTHistory();

	// This should trigger MQTT publish
	CMD_ExecuteCommand("publishFloat nextVal $CH15*0.001", 0);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_FLOAT("myTestDevice/nextVal/get", 0.505f, false);
	// if assert has passed, we can clear SIM MQTT history, it's no longer needed
	SIM_ClearMQTTHistory();

	// This should trigger MQTT publish
	CMD_ExecuteCommand("addEventHandler OnChannelChange 20 publishFloat myAliasedVal $CH20*0.1", 0);
	CMD_ExecuteCommand("setChannel 20 5", 0);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_FLOAT("myTestDevice/myAliasedVal/get", 0.5f, false);
	// if assert has passed, we can clear SIM MQTT history, it's no longer needed
	SIM_ClearMQTTHistory();

	// next test
	CMD_ExecuteCommand("setChannel 20 123", 0);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_FLOAT("myTestDevice/myAliasedVal/get", 12.3f, false);
	// if assert has passed, we can clear SIM MQTT history, it's no longer needed
	SIM_ClearMQTTHistory();
}
void Test_MQTT_LED_CW() {
	SIM_ClearOBK();
	SIM_ClearAndPrepareForMQTTTesting("myTestDevice", "bekens");

	PIN_SetPinRoleForPinIndex(24, IOR_PWM);
	PIN_SetPinChannelForPinIndex(24, 1);

	PIN_SetPinRoleForPinIndex(26, IOR_PWM);
	PIN_SetPinChannelForPinIndex(26, 2);

	CMD_ExecuteCommand("led_enableAll 1", 0);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("myTestDevice/led_enableAll/get", "1", false);
	// if assert has passed, we can clear SIM MQTT history, it's no longer needed
	SIM_ClearMQTTHistory();

	CMD_ExecuteCommand("led_dimmer 100", 0);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("myTestDevice/led_dimmer/get", "100", false);
	// if assert has passed, we can clear SIM MQTT history, it's no longer needed
	SIM_ClearMQTTHistory();

	CMD_ExecuteCommand("led_temperature 153", 0);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("myTestDevice/led_temperature/get", "153", false);
	// if assert has passed, we can clear SIM MQTT history, it's no longer needed
	SIM_ClearMQTTHistory();

	SELFTEST_ASSERT_CHANNEL(1, 100);
	SELFTEST_ASSERT_CHANNEL(2, 0);

	CMD_ExecuteCommand("led_enableAll 0", 0);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("myTestDevice/led_enableAll/get", "0", false);
	// if assert has passed, we can clear SIM MQTT history, it's no longer needed
	SIM_ClearMQTTHistory();

	SELFTEST_ASSERT_CHANNEL(1, 0);
	SELFTEST_ASSERT_CHANNEL(2, 0);

	CMD_ExecuteCommand("led_enableAll 1", 0);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("myTestDevice/led_enableAll/get", "1", false);
	// if assert has passed, we can clear SIM MQTT history, it's no longer needed
	SIM_ClearMQTTHistory();

	CMD_ExecuteCommand("led_temperature 500", 0);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("myTestDevice/led_temperature/get", "500", false);
	// if assert has passed, we can clear SIM MQTT history, it's no longer needed
	SIM_ClearMQTTHistory();

	SELFTEST_ASSERT_CHANNEL(1, 0);
	SELFTEST_ASSERT_CHANNEL(2, 100);

	SIM_SendFakeMQTTAndRunSimFrame_CMND("led_dimmer","50");
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("myTestDevice/led_dimmer/get", "50", false);
	// if assert has passed, we can clear SIM MQTT history, it's no longer needed
	SIM_ClearMQTTHistory();

	SELFTEST_ASSERT_CHANNEL(1, 0);
	SELFTEST_ASSERT_CHANNEL(2, 21);

	SIM_SendFakeMQTTAndRunSimFrame_CMND("led_dimmer", "100");
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("myTestDevice/led_dimmer/get", "100", false);
	// if assert has passed, we can clear SIM MQTT history, it's no longer needed
	SIM_ClearMQTTHistory();

	SELFTEST_ASSERT_CHANNEL(1, 0);
	SELFTEST_ASSERT_CHANNEL(2, 100);
}
void Test_MQTT_LED_RGB() {
	SIM_ClearOBK();
	SIM_ClearAndPrepareForMQTTTesting("fakeRGBbulb", "bekens");

	PIN_SetPinRoleForPinIndex(24, IOR_PWM);
	PIN_SetPinChannelForPinIndex(24, 1);

	PIN_SetPinRoleForPinIndex(26, IOR_PWM);
	PIN_SetPinChannelForPinIndex(26, 2);

	PIN_SetPinRoleForPinIndex(9, IOR_PWM);
	PIN_SetPinChannelForPinIndex(9, 3);

	CFG_SetFlag(OBK_FLAG_MQTT_BROADCASTLEDFINALCOLOR, true);

	SIM_SendFakeMQTTAndRunSimFrame_CMND("led_enableAll", "1");
	SIM_SendFakeMQTTAndRunSimFrame_CMND("led_basecolor_rgb", "00FF00");
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("fakeRGBbulb/led_basecolor_rgb/get", "00FF00", false);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("fakeRGBbulb/led_enableAll/get", "1", false);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("fakeRGBbulb/led_finalcolor_rgb/get", "00FF00", false);
	// if assert has passed, we can clear SIM MQTT history, it's no longer needed
	SIM_ClearMQTTHistory();


	SIM_SendFakeMQTTAndRunSimFrame_CMND("led_dimmer", "50");
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("fakeRGBbulb/led_dimmer/get", "50", false);
	// half of FF is 7F
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("fakeRGBbulb/led_finalcolor_rgb/get", "003700", false);
	// if assert has passed, we can clear SIM MQTT history, it's no longer needed
	SIM_ClearMQTTHistory();

	SIM_SendFakeMQTTAndRunSimFrame_CMND("add_dimmer", "1");
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("fakeRGBbulb/led_dimmer/get", "51", false);
	// if assert has passed, we can clear SIM MQTT history, it's no longer needed
	SIM_ClearMQTTHistory();


	SIM_SendFakeMQTTAndRunSimFrame_CMND("add_dimmer", "1");
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("fakeRGBbulb/led_dimmer/get", "52", false);
	// if assert has passed, we can clear SIM MQTT history, it's no longer needed
	SIM_ClearMQTTHistory();


	SIM_SendFakeMQTTAndRunSimFrame_CMND("led_dimmer", "55");
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("fakeRGBbulb/led_dimmer/get", "55", false);
	// if assert has passed, we can clear SIM MQTT history, it's no longer needed
	SIM_ClearMQTTHistory();

	SIM_SendFakeMQTTAndRunSimFrame_CMND("add_dimmer", "-5");
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("fakeRGBbulb/led_dimmer/get", "50", false);
	// half of FF is 7F
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("fakeRGBbulb/led_finalcolor_rgb/get", "003700", false);
	// if assert has passed, we can clear SIM MQTT history, it's no longer needed
	SIM_ClearMQTTHistory();
}
void Test_MQTT_Misc() {
	SIM_ClearOBK();
	SIM_ClearAndPrepareForMQTTTesting("miscDevice", "bekens");

	CMD_ExecuteCommand("addEventHandler OnChannelChange 5 publish myMagicResult $CH5", 0);
	// set channel 5 to 50 and see what we get
	SIM_SendFakeMQTTAndRunSimFrame_CMND("setChannel", "5 50");
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("miscDevice/myMagicResult/get", "50", false);
	SIM_ClearMQTTHistory();

	CMD_ExecuteCommand("addEventHandler OnChannelChange 6 publishFloat mySecond $CH6*0.1", 0);
	// set channel 6 to 123 and see what we get
	SIM_SendFakeMQTTAndRunSimFrame_CMND("setChannel", "6 123");
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_FLOAT("miscDevice/mySecond/get", 12.3f, false);
	SIM_ClearMQTTHistory();

	//SIM_SendFakeMQTTAndRunSimFrame_CMND("setChannel", "3 100");
	//CMD_ExecuteCommand("addEventHandler OnChannelChange 7 publishFloat thirdTest $CH7*0.01+$CH3", 0);
	//// set channel 7 to 31.4 and see what we get
	//SIM_SendFakeMQTTAndRunSimFrame_CMND("setChannel", "7 314");
	//SELFTEST_ASSERT_HAD_MQTT_PUBLISH_FLOAT("miscDevice/thirdTest/get", (314*0.01f+100), false);
	//SIM_ClearMQTTHistory();
}
void Test_MQTT_Topic_With_Slashes() {
	SIM_ClearOBK();
	SIM_ClearAndPrepareForMQTTTesting("obk/kitchen/mySwitch1", "bekens");

	PIN_SetPinRoleForPinIndex(24, IOR_Relay);
	PIN_SetPinChannelForPinIndex(24, 1);

	// This should trigger MQTT publish
	CMD_ExecuteCommand("setChannel 1 1", 0);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("obk/kitchen/mySwitch1/1/get", "1", false);
	// if assert has passed, we can clear SIM MQTT history, it's no longer needed
	SIM_ClearMQTTHistory();

	// This should trigger MQTT publish
	CMD_ExecuteCommand("setChannel 1 0", 0);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("obk/kitchen/mySwitch1/1/get", "0", false);
	// if assert has passed, we can clear SIM MQTT history, it's no longer needed
	SIM_ClearMQTTHistory();

	SIM_SendFakeMQTTAndRunSimFrame_CMND("POWER", "1");
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("obk/kitchen/mySwitch1/1/get", "1", false);
	// if assert has passed, we can clear SIM MQTT history, it's no longer needed
	SIM_ClearMQTTHistory();

	SIM_SendFakeMQTTAndRunSimFrame_CMND("POWER", "0");
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("obk/kitchen/mySwitch1/1/get", "0", false);
	// if assert has passed, we can clear SIM MQTT history, it's no longer needed
	SIM_ClearMQTTHistory();

	SIM_SendFakeMQTTAndRunSimFrame_CMND("POWER", "TOGGLE");
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("obk/kitchen/mySwitch1/1/get", "1", false);
	// if assert has passed, we can clear SIM MQTT history, it's no longer needed
	SIM_ClearMQTTHistory();

	SIM_SendFakeMQTTAndRunSimFrame_CMND_ViaGroupTopic("POWER", "TOGGLE");
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("obk/kitchen/mySwitch1/1/get", "0", false);
	// if assert has passed, we can clear SIM MQTT history, it's no longer needed
	SIM_ClearMQTTHistory();

	SIM_SendFakeMQTTAndRunSimFrame_CMND_ViaGroupTopic("POWER", "TOGGLE");
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("obk/kitchen/mySwitch1/1/get", "1", false);
	// if assert has passed, we can clear SIM MQTT history, it's no longer needed
	SIM_ClearMQTTHistory();


	SIM_SendFakeMQTTRawChannelSet(1, "10");
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("obk/kitchen/mySwitch1/1/get", "10", false);
	// if assert has passed, we can clear SIM MQTT history, it's no longer needed
	SIM_ClearMQTTHistory();

	SIM_SendFakeMQTTRawChannelSet_ViaGroupTopic(1, "20");
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("obk/kitchen/mySwitch1/1/get", "20", false);
	// if assert has passed, we can clear SIM MQTT history, it's no longer needed
	SIM_ClearMQTTHistory();

	SIM_SendFakeMQTTRawChannelSet_ViaGroupTopic(1, "1");
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("obk/kitchen/mySwitch1/1/get", "1", false);
	// if assert has passed, we can clear SIM MQTT history, it's no longer needed
	SIM_ClearMQTTHistory();

	SIM_SendFakeMQTTRawChannelSet_ViaGroupTopic(1, "TOGGLE");
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("obk/kitchen/mySwitch1/1/get", "0", false);
	// if assert has passed, we can clear SIM MQTT history, it's no longer needed
	SIM_ClearMQTTHistory();
}

void Test_MQTT_Topic_With_Slash() {
	SIM_ClearOBK();
	SIM_ClearAndPrepareForMQTTTesting("obk/08C65DE9", "bekens");

	PIN_SetPinRoleForPinIndex(24, IOR_Relay);
	PIN_SetPinChannelForPinIndex(24, 1);

	// This should trigger MQTT publish
	CMD_ExecuteCommand("setChannel 1 1", 0);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("obk/08C65DE9/1/get", "1", false);
	// if assert has passed, we can clear SIM MQTT history, it's no longer needed
	SIM_ClearMQTTHistory();

	// This should trigger MQTT publish
	CMD_ExecuteCommand("setChannel 1 0", 0);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("obk/08C65DE9/1/get", "0", false);
	// if assert has passed, we can clear SIM MQTT history, it's no longer needed
	SIM_ClearMQTTHistory();

	SIM_SendFakeMQTTAndRunSimFrame_CMND("POWER", "1");
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("obk/08C65DE9/1/get", "1", false);
	// if assert has passed, we can clear SIM MQTT history, it's no longer needed
	SIM_ClearMQTTHistory();

	SIM_SendFakeMQTTAndRunSimFrame_CMND("POWER", "0");
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("obk/08C65DE9/1/get", "0", false);
	// if assert has passed, we can clear SIM MQTT history, it's no longer needed
	SIM_ClearMQTTHistory();

	SIM_SendFakeMQTTAndRunSimFrame_CMND("POWER", "TOGGLE");
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("obk/08C65DE9/1/get", "1", false);
	// if assert has passed, we can clear SIM MQTT history, it's no longer needed
	SIM_ClearMQTTHistory();

	SIM_SendFakeMQTTAndRunSimFrame_CMND_ViaGroupTopic("POWER", "TOGGLE");
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("obk/08C65DE9/1/get", "0", false);
	// if assert has passed, we can clear SIM MQTT history, it's no longer needed
	SIM_ClearMQTTHistory();

	SIM_SendFakeMQTTAndRunSimFrame_CMND_ViaGroupTopic("POWER", "TOGGLE");
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("obk/08C65DE9/1/get", "1", false);
	// if assert has passed, we can clear SIM MQTT history, it's no longer needed
	SIM_ClearMQTTHistory();


	SIM_SendFakeMQTTRawChannelSet(1, "10");
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("obk/08C65DE9/1/get", "10", false);
	// if assert has passed, we can clear SIM MQTT history, it's no longer needed
	SIM_ClearMQTTHistory();

	SIM_SendFakeMQTTRawChannelSet_ViaGroupTopic(1, "20");
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("obk/08C65DE9/1/get", "20", false);
	// if assert has passed, we can clear SIM MQTT history, it's no longer needed
	SIM_ClearMQTTHistory();
}


void Test_MQTT(){
	Test_MQTT_Get_And_Reply();
	Test_MQTT_Misc();
	Test_MQTT_Channels();
	Test_MQTT_LED_CW();
	Test_MQTT_LED_RGB();
	Test_MQTT_Topic_With_Slash();
	Test_MQTT_Topic_With_Slashes();
}

#endif
