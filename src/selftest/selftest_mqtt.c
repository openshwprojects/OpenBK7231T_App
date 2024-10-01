#ifdef WINDOWS

#include "selftest_local.h"
#include "../hal/hal_wifi.h"

void SIM_ClearAndPrepareForMQTTTesting(const char *clientName, const char *groupName) {
	SIM_ClearOBK(0);
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
	SIM_ClearOBK(0);
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
	SIM_ClearOBK(0);
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


	// This should trigger MQTT publish
	// Third arg - [Topic][Value][bOptionalSkipPrefixAndSuffix]
	CMD_ExecuteCommand("publishInt test 123", 0);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("myTestDevice/test/get", "123", false);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_FLOAT("myTestDevice/test/get", 123, false);
	// if assert has passed, we can clear SIM MQTT history, it's no longer needed
	SIM_ClearMQTTHistory();
	// Third arg - [Topic][Value][bOptionalSkipPrefixAndSuffix]
	CMD_ExecuteCommand("publishInt myTestDevice/test 123 1", 0);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("myTestDevice/test", "123", false);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_FLOAT("myTestDevice/test", 123, false);
	// if assert has passed, we can clear SIM MQTT history, it's no longer needed
	SIM_ClearMQTTHistory();
	// Third arg - [Topic][Value][bOptionalSkipPrefixAndSuffix]
	CMD_ExecuteCommand("setChannel 0 123", 0);
	CMD_ExecuteCommand("setChannel 1 456", 0);

	Tokenizer_TokenizeString("$CH0", TOKENIZER_ALLOW_QUOTES | TOKENIZER_ALLOW_ESCAPING_QUOTATIONS);
	SELFTEST_ASSERT(strcmp(Tokenizer_GetArg(0), "123") == 0);
	Tokenizer_TokenizeString("\"$CH0\"", TOKENIZER_ALLOW_QUOTES | TOKENIZER_ALLOW_ESCAPING_QUOTATIONS | TOKENIZER_EXPAND_EARLY);
	SELFTEST_ASSERT(strcmp(Tokenizer_GetArg(0), "123") == 0);
	Tokenizer_TokenizeString("\"\\\"$CH0\\\"\"", TOKENIZER_ALLOW_QUOTES | TOKENIZER_ALLOW_ESCAPING_QUOTATIONS | TOKENIZER_EXPAND_EARLY);
	SELFTEST_ASSERT(strcmp(Tokenizer_GetArg(0), "\"123\"") == 0);
	
	CMD_ExecuteCommand("publish myTestDevice/test \"{\\\"temperature\\\":$CH0,\\\"humidity\\\":$CH1}\" 1", 0);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("myTestDevice/test", "{\"temperature\":123,\"humidity\":456}", false);
	SIM_ClearMQTTHistory();

	Tokenizer_TokenizeString("publish myTestDevice/test {\"temperature\":$CH0,\"humidity\":$CH1} 1", TOKENIZER_ALLOW_QUOTES | TOKENIZER_ALLOW_ESCAPING_QUOTATIONS | TOKENIZER_EXPAND_EARLY);
	SELFTEST_ASSERT(strcmp(Tokenizer_GetArg(2), "{\"temperature\":123,\"humidity\":456}") == 0);
	SIM_ClearMQTTHistory();

	CMD_ExecuteCommand("publish myTestDevice/test \"{\\\"temperature\\\":$CH0,\\\"humidity\\\":$CH1}\" 1", 0);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("myTestDevice/test", "{\"temperature\":123,\"humidity\":456}", false);
	SIM_ClearMQTTHistory();

	// with space in the middle
	CMD_ExecuteCommand("publish myTestDevice/test \"{\\\"temperature\\\":$CH0, \\\"humidity\\\":$CH1}\" 1", 0);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("myTestDevice/test", "{\"temperature\":123, \"humidity\":456}", false);
	SIM_ClearMQTTHistory();

	// with space in the middle
	CMD_ExecuteCommand("publish myTestDevice/test \"{\\\"msg\\\":\\\"Hello World With Spaces\\\"}\" 1", 0);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("myTestDevice/test", "{\"msg\":\"Hello World With Spaces\"}", false);
	SIM_ClearMQTTHistory();
}
void Test_MQTT_LED_CW() {
	SIM_ClearOBK(0);
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
	SIM_ClearOBK(0);
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

	// Assert raw channels for RGB
	SELFTEST_ASSERT_CHANNEL(1, 0);
	SELFTEST_ASSERT_CHANNEL(2, 100);
	SELFTEST_ASSERT_CHANNEL(3, 0);


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

	// Assert raw channels for RGB
	SELFTEST_ASSERT_CHANNEL(1, 0);
	SELFTEST_ASSERT_CHANNELEPSILON(2, (0x37/256.0f)*100.0f, 1.0f);
	SELFTEST_ASSERT_CHANNEL(3, 0);

	SIM_SendFakeMQTTAndRunSimFrame_CMND("led_dimmer", "100");
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("fakeRGBbulb/led_dimmer/get", "100", false);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("fakeRGBbulb/led_finalcolor_rgb/get", "00FF00", false);

	// Assert raw channels for RGB
	SELFTEST_ASSERT_CHANNEL(1, 0);
	SELFTEST_ASSERT_CHANNEL(2, 100);
	SELFTEST_ASSERT_CHANNEL(3, 0);


	//SIM_SendFakeMQTTAndRunSimFrame_CMND("{\"color\":{\"b\":255,\"c\":0,\"g\":255,\"r\":255},\"state\":\"1\"}", "");
	//SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("fakeRGBbulb/led_dimmer/get", "52", false);
	// if assert has passed, we can clear SIM MQTT history, it's no longer needed
	SIM_ClearMQTTHistory();
}
void Test_MQTT_LED_RGBCW() {
	SIM_ClearOBK(0);
	SIM_ClearAndPrepareForMQTTTesting("fakeRGBCWbulb", "bekens");

	PIN_SetPinRoleForPinIndex(24, IOR_PWM);
	PIN_SetPinChannelForPinIndex(24, 1);

	PIN_SetPinRoleForPinIndex(26, IOR_PWM);
	PIN_SetPinChannelForPinIndex(26, 2);

	PIN_SetPinRoleForPinIndex(9, IOR_PWM);
	PIN_SetPinChannelForPinIndex(9, 3);

	PIN_SetPinRoleForPinIndex(8, IOR_PWM);
	PIN_SetPinChannelForPinIndex(8, 4);

	PIN_SetPinRoleForPinIndex(7, IOR_PWM);
	PIN_SetPinChannelForPinIndex(7, 5);

	CFG_SetFlag(OBK_FLAG_MQTT_BROADCASTLEDFINALCOLOR, true);

	SIM_SendFakeMQTTAndRunSimFrame_CMND("led_enableAll", "1");
	SIM_SendFakeMQTTAndRunSimFrame_CMND("led_basecolor_rgb", "00FF00");
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("fakeRGBCWbulb/led_basecolor_rgb/get", "00FF00", false);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("fakeRGBCWbulb/led_enableAll/get", "1", false);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("fakeRGBCWbulb/led_finalcolor_rgb/get", "00FF00", false);
	// if assert has passed, we can clear SIM MQTT history, it's no longer needed
	SIM_ClearMQTTHistory();

	// Assert raw channels for RGB
	SELFTEST_ASSERT_CHANNEL(1, 0);
	SELFTEST_ASSERT_CHANNEL(2, 100);
	SELFTEST_ASSERT_CHANNEL(3, 0);
	SELFTEST_ASSERT_CHANNEL(4, 0);
	SELFTEST_ASSERT_CHANNEL(5, 0);


	SIM_SendFakeMQTTAndRunSimFrame_CMND("led_dimmer", "50");
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("fakeRGBCWbulb/led_dimmer/get", "50", false);
	// half of FF is 7F
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("fakeRGBCWbulb/led_finalcolor_rgb/get", "003700", false);
	// if assert has passed, we can clear SIM MQTT history, it's no longer needed
	SIM_ClearMQTTHistory();

	SIM_SendFakeMQTTAndRunSimFrame_CMND("add_dimmer", "1");
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("fakeRGBCWbulb/led_dimmer/get", "51", false);
	// if assert has passed, we can clear SIM MQTT history, it's no longer needed
	SIM_ClearMQTTHistory();


	SIM_SendFakeMQTTAndRunSimFrame_CMND("add_dimmer", "1");
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("fakeRGBCWbulb/led_dimmer/get", "52", false);
	// if assert has passed, we can clear SIM MQTT history, it's no longer needed
	SIM_ClearMQTTHistory();


	SIM_SendFakeMQTTAndRunSimFrame_CMND("led_dimmer", "55");
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("fakeRGBCWbulb/led_dimmer/get", "55", false);
	// if assert has passed, we can clear SIM MQTT history, it's no longer needed
	SIM_ClearMQTTHistory();

	SIM_SendFakeMQTTAndRunSimFrame_CMND("add_dimmer", "-5");
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("fakeRGBCWbulb/led_dimmer/get", "50", false);
	// half of FF is 7F
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("fakeRGBCWbulb/led_finalcolor_rgb/get", "003700", false);

	// Assert raw channels for RGB
	SELFTEST_ASSERT_CHANNEL(1, 0);
	SELFTEST_ASSERT_CHANNELEPSILON(2, (0x37 / 256.0f)*100.0f, 1.0f);
	SELFTEST_ASSERT_CHANNEL(3, 0);
	SELFTEST_ASSERT_CHANNEL(4, 0);
	SELFTEST_ASSERT_CHANNEL(5, 0);

	SIM_SendFakeMQTTAndRunSimFrame_CMND("led_dimmer", "100");
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("fakeRGBCWbulb/led_dimmer/get", "100", false);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("fakeRGBCWbulb/led_finalcolor_rgb/get", "00FF00", false);

	// Assert raw channels for RGB
	SELFTEST_ASSERT_CHANNEL(1, 0);
	SELFTEST_ASSERT_CHANNEL(2, 100);
	SELFTEST_ASSERT_CHANNEL(3, 0);
	SELFTEST_ASSERT_CHANNEL(4, 0);
	SELFTEST_ASSERT_CHANNEL(5, 0);

	CMD_ExecuteCommand("led_enableAll 1", 0);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("fakeRGBCWbulb/led_enableAll/get", "1", false);
	// if assert has passed, we can clear SIM MQTT history, it's no longer needed
	SIM_ClearMQTTHistory();

	CMD_ExecuteCommand("led_dimmer 100", 0);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("fakeRGBCWbulb/led_dimmer/get", "100", false);
	// if assert has passed, we can clear SIM MQTT history, it's no longer needed
	SIM_ClearMQTTHistory();

	CMD_ExecuteCommand("led_temperature 153", 0);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("fakeRGBCWbulb/led_temperature/get", "153", false);
	// if assert has passed, we can clear SIM MQTT history, it's no longer needed
	SIM_ClearMQTTHistory();

	SELFTEST_ASSERT_CHANNEL(1, 0);
	SELFTEST_ASSERT_CHANNEL(2, 0);
	SELFTEST_ASSERT_CHANNEL(3, 0);
	SELFTEST_ASSERT_CHANNEL(4, 100);
	SELFTEST_ASSERT_CHANNEL(5, 0);

	CMD_ExecuteCommand("led_enableAll 0", 0);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("fakeRGBCWbulb/led_enableAll/get", "0", false);
	// if assert has passed, we can clear SIM MQTT history, it's no longer needed
	SIM_ClearMQTTHistory();

	SELFTEST_ASSERT_CHANNEL(1, 0);
	SELFTEST_ASSERT_CHANNEL(2, 0);
	SELFTEST_ASSERT_CHANNEL(3, 0);
	SELFTEST_ASSERT_CHANNEL(4, 0);
	SELFTEST_ASSERT_CHANNEL(5, 0);

	CMD_ExecuteCommand("led_enableAll 1", 0);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("fakeRGBCWbulb/led_enableAll/get", "1", false);
	// if assert has passed, we can clear SIM MQTT history, it's no longer needed
	SIM_ClearMQTTHistory();

	CMD_ExecuteCommand("led_temperature 500", 0);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("fakeRGBCWbulb/led_temperature/get", "500", false);
	// if assert has passed, we can clear SIM MQTT history, it's no longer needed
	SIM_ClearMQTTHistory();

	SELFTEST_ASSERT_CHANNEL(1, 0);
	SELFTEST_ASSERT_CHANNEL(2, 0);
	SELFTEST_ASSERT_CHANNEL(3, 0);
	SELFTEST_ASSERT_CHANNEL(4, 0);
	SELFTEST_ASSERT_CHANNEL(5, 100);

	SIM_SendFakeMQTTAndRunSimFrame_CMND("led_dimmer", "50");
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("fakeRGBCWbulb/led_dimmer/get", "50", false);
	// if assert has passed, we can clear SIM MQTT history, it's no longer needed
	SIM_ClearMQTTHistory();

	SELFTEST_ASSERT_CHANNEL(1, 0);
	SELFTEST_ASSERT_CHANNEL(2, 0);
	SELFTEST_ASSERT_CHANNEL(3, 0);
	SELFTEST_ASSERT_CHANNEL(4, 0);
	SELFTEST_ASSERT_CHANNEL(5, 21);

	SIM_SendFakeMQTTAndRunSimFrame_CMND("led_dimmer", "100");
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("fakeRGBCWbulb/led_dimmer/get", "100", false);
	// if assert has passed, we can clear SIM MQTT history, it's no longer needed
	SIM_ClearMQTTHistory();

	SELFTEST_ASSERT_CHANNEL(1, 0);
	SELFTEST_ASSERT_CHANNEL(2, 0);
	SELFTEST_ASSERT_CHANNEL(3, 0);
	SELFTEST_ASSERT_CHANNEL(4, 0);
	SELFTEST_ASSERT_CHANNEL(5, 100);


	SIM_SendFakeMQTTAndRunSimFrame_CMND("led_basecolor_rgb", "00FFFF");
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("fakeRGBCWbulb/led_basecolor_rgb/get", "00FFFF", false);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("fakeRGBCWbulb/led_finalcolor_rgb/get", "00FFFF", false);
	// if assert has passed, we can clear SIM MQTT history, it's no longer needed
	SIM_ClearMQTTHistory();

	// Assert raw channels for RGB
	SELFTEST_ASSERT_CHANNEL(1, 0);
	SELFTEST_ASSERT_CHANNEL(2, 100);
	SELFTEST_ASSERT_CHANNEL(3, 100);
	SELFTEST_ASSERT_CHANNEL(4, 0);
	SELFTEST_ASSERT_CHANNEL(5, 0);


	//SIM_SendFakeMQTTAndRunSimFrame_CMND("{\"color\":{\"b\":255,\"c\":0,\"g\":255,\"r\":255},\"state\":\"1\"}", "");
	//SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("fakeRGBCWbulb/led_dimmer/get", "52", false);
	// if assert has passed, we can clear SIM MQTT history, it's no longer needed
	SIM_ClearMQTTHistory();
}
void Test_MQTT_Misc() {
	SIM_ClearOBK(0);
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

	CMD_ExecuteCommand("publish myMagicResult \"{\\\"state\\\": \\\"TOGGLE\\\"}\"", 0);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("miscDevice/myMagicResult/get", "{\"state\": \"TOGGLE\"}", false);
	SIM_ClearMQTTHistory();

	//SIM_SendFakeMQTTAndRunSimFrame_CMND("setChannel", "3 100");
	//CMD_ExecuteCommand("publish myMagicResult \"{\\\"state\\\": \\\"$CH1\\\"}\"", 0);
	//SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("miscDevice/myMagicResult/get", "{\"state\": \"100\"}", false);
	//SIM_ClearMQTTHistory();
	
}
void Test_MQTT_Topic_With_Slashes() {
	SIM_ClearOBK(0);
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
	SIM_ClearOBK(0);
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


void Test_MQTT_Average() {
	SIM_ClearOBK(0);
	SIM_ClearAndPrepareForMQTTTesting("obk/08C65DE9", "bekens");

	// start value
	CMD_ExecuteCommand("setChannel 10 0", 0);
	int numbers[] = { 12, 91, 64, 12, 64 };
	int cnt = sizeof(numbers) / sizeof(numbers[0]);
	int checkSum = 0;
	for (int i = 0; i < cnt; i++) {
		char cmd[128];
		SELFTEST_ASSERT_CHANNEL(10, checkSum);
		sprintf(cmd,"setChannel 1 %i", numbers[i]);
		CMD_ExecuteCommand(cmd, 0);
		CMD_ExecuteCommand("addChannel 10 $CH1", 0);
		checkSum += numbers[i];
		SELFTEST_ASSERT_CHANNEL(10, checkSum);
	}
	// 243 / 5 = 48.6
	float average = (float)checkSum / (float)cnt;
	SIM_ClearMQTTHistory();
	CMD_ExecuteCommand("publishFloat myAverageResult $CH10/5", 0);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_FLOAT("obk/08C65DE9/myAverageResult/get", average, false);
	// if assert has passed, we can clear SIM MQTT history, it's no longer needed
	SIM_ClearMQTTHistory();
}

void Test_MQTT(){
	Test_MQTT_Misc();
	Test_MQTT_Get_And_Reply();
	Test_MQTT_Channels();
	Test_MQTT_LED_CW();
	Test_MQTT_LED_RGB();
	Test_MQTT_LED_RGBCW();
	Test_MQTT_Topic_With_Slash();
	Test_MQTT_Topic_With_Slashes();
	Test_MQTT_Average();
}

#endif
