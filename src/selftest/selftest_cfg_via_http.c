#ifdef WINDOWS

#include "selftest_local.h"
#include "../httpserver/new_http.h"

void Test_CFG_Via_HTTP() {
	const char *mqtt_userName = "MyMQTTUser";
	SIM_ClearOBK(0);
	PIN_SetPinRoleForPinIndex(9, IOR_Relay);
	PIN_SetPinChannelForPinIndex(9, 1);
	CFG_SetMQTTUserName(mqtt_userName);
	CFG_SetMQTTHost("192.168.123.246");
	CFG_SetWiFiSSID("FakeNet3D");

	Test_FakeHTTPClientPacket_JSON("cm?cmnd=POWER");
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "POWER", "OFF");

	Test_FakeHTTPClientPacket_JSON("cm?cmnd=POWER");
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "POWER", "OFF");

	CMD_ExecuteCommand("setChannel 1 1", 0);

	Test_FakeHTTPClientPacket_JSON("cm?cmnd=POWER");
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "POWER", "ON");


	Test_FakeHTTPClientPacket_JSON("cm?cmnd=MQTTUser");
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "MQTTUser", mqtt_userName);
	CFG_SetMQTTUserName("UserName2");
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=MQTTUser");
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "MQTTUser", "UserName2");
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=MQTTUser");
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "MQTTUser", "UserName2");
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=MQTTUser%20NewName");
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "MQTTUser", "NewName");
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=MQTTUser%20another");
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "MQTTUser", "another");
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=MQTTHost");
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "MQTTHost", "192.168.123.246");
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=MQTTHost%208.8.8.8");
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "MQTTHost", "8.8.8.8");

	Test_FakeHTTPClientPacket_JSON("cm?cmnd=SSID1");
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "SSID1", "FakeNet3D");

	Test_FakeHTTPClientPacket_JSON("cm?cmnd=SSID1%20newApName");
	SELFTEST_ASSERT_JSON_VALUE_STRING(0, "SSID1", "newApName");
	// causes assert
	//SELFTEST_ASSERT_JSON_VALUE_STRING(0, "SSID1", "newAcName");
}

#endif
