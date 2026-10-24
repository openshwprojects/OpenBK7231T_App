#ifdef WINDOWS

#include "selftest_local.h"
#include "../httpserver/new_http.h"
#include "../cJSON/cJSON.h"

// Replays what an Amazon Echo does with a Hue bridge, against a cool/warm white bulb.
static cJSON *Hue_Light1() {
	// GET .../lights returns {"1":{...}}; GET .../lights/1 returns the light itself
	cJSON *json = Test_GetLastJSON();
	cJSON *one = cJSON_GetObjectItemCaseSensitive(json, "1");
	return one ? one : json;
}
static cJSON *Hue_State() {
	return cJSON_GetObjectItemCaseSensitive(Hue_Light1(), "state");
}
static int Hue_StateInt(const char *key) {
	cJSON *v = cJSON_GetObjectItemCaseSensitive(Hue_State(), key);
	return v ? v->valueint : -1;
}
static int Hue_StateOn() {
	return cJSON_IsTrue(cJSON_GetObjectItemCaseSensitive(Hue_State(), "on"));
}
static const char *Hue_String(cJSON *obj, const char *key) {
	cJSON *v = cJSON_GetObjectItemCaseSensitive(obj, key);
	return (v && v->valuestring) ? v->valuestring : "";
}
// [{"success":{"/lights/1/state/bri":127}}, ...] - true if the reply confirms the given path
static int Hue_ReplyConfirms(const char *path) {
	cJSON *item;
	cJSON_ArrayForEach(item, Test_GetLastJSON()) {
		cJSON *ok = cJSON_GetObjectItemCaseSensitive(item, "success");
		if (ok && cJSON_GetObjectItemCaseSensitive(ok, path))
			return 1;
	}
	return 0;
}

void Test_Hue_CW() {
	SIM_ClearOBK(0);
	// wired like a Tuya CW bulb: cool white on channel 4, warm white on channel 5
	PIN_SetPinRoleForPinIndex(8, IOR_PWM);
	PIN_SetPinChannelForPinIndex(8, 4);
	PIN_SetPinRoleForPinIndex(7, IOR_PWM);
	PIN_SetPinChannelForPinIndex(7, 5);
	CFG_SetDeviceName("Lamp \"Right\"");   // a quote in the name must be escaped in the JSON
	CMD_ExecuteCommand("startDriver SSDP", 0);
	CMD_ExecuteCommand("startDriver Hue", 0);
	CMD_ExecuteCommand("led_enableAll 1", 0);
	CMD_ExecuteCommand("led_dimmer 100", 0);
	CMD_ExecuteCommand("led_temperature 500", 0);

	// 1. register
	Test_FakeHTTPClientPacket_POST_withJSONReply("api", "{\"devicetype\":\"Echo\"}");
	SELFTEST_ASSERT(cJSON_GetObjectItemCaseSensitive(cJSON_GetArrayItem(Test_GetLastJSON(), 0), "success") != 0);

	// 2. list lights: one colour temperature light with the current state and the device's name
	Test_FakeHTTPClientPacket_JSON("api/1234AB/lights");
	SELFTEST_ASSERT_STRING(Hue_String(Hue_Light1(), "type"), "Color temperature light");
	SELFTEST_ASSERT_STRING(Hue_String(Hue_Light1(), "name"), "Lamp \"Right\"");
	SELFTEST_ASSERT(Hue_StateOn());
	SELFTEST_ASSERT(Hue_StateInt("bri") == 254);
	SELFTEST_ASSERT(Hue_StateInt("ct") == 500);
	SELFTEST_ASSERT(strlen(Hue_String(Hue_Light1(), "uniqueid")) > 20);

	// 3. "Alexa, turn off the lamp"
	Test_FakeHTTPClientPacket_PUT_withJSONReply("api/1234AB/lights/1/state", "{\"on\":false}");
	SELFTEST_ASSERT(Hue_ReplyConfirms("/lights/1/state/on"));
	SELFTEST_ASSERT(LED_GetEnableAll() == 0);
	SELFTEST_ASSERT_CHANNEL(5, 0);

	// 4. "Alexa, set the lamp to 50%" - no "on" in the request, so it also switches on
	Test_FakeHTTPClientPacket_PUT_withJSONReply("api/1234AB/lights/1/state", "{\"bri\":127}");
	SELFTEST_ASSERT(Hue_ReplyConfirms("/lights/1/state/bri"));
	SELFTEST_ASSERT(LED_GetEnableAll() == 1);
	SELFTEST_ASSERT(Float_Equals(LED_GetDimmer(), 50));

	// 5. "Alexa, make the lamp cool white" (153 mireds = 6500 K)
	Test_FakeHTTPClientPacket_PUT_withJSONReply("api/1234AB/lights/1/state", "{\"on\": true, \"ct\": 153}");
	SELFTEST_ASSERT(Hue_ReplyConfirms("/lights/1/state/on"));
	SELFTEST_ASSERT(Hue_ReplyConfirms("/lights/1/state/ct"));
	SELFTEST_ASSERT(LED_GetTemperature() <= 154);
	SELFTEST_ASSERT(CHANNEL_Get(4) > CHANNEL_Get(5));   // cool white brighter than warm

	// 6. the Echo reads the light back
	Test_FakeHTTPClientPacket_JSON("api/1234AB/lights/1");
	SELFTEST_ASSERT(Hue_StateOn());
	SELFTEST_ASSERT(Hue_StateInt("bri") == 127);
	SELFTEST_ASSERT(Hue_StateInt("ct") <= 154);

	// 7. whole bridge and config, and a light that doesn't exist
	Test_FakeHTTPClientPacket_JSON("api/1234AB");
	SELFTEST_ASSERT(cJSON_GetObjectItemCaseSensitive(Test_GetLastJSON(), "config") != 0);
	SELFTEST_ASSERT(cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(Test_GetLastJSON(), "lights"), "1") != 0);
	Test_FakeHTTPClientPacket_JSON("api/1234AB/config");
	SELFTEST_ASSERT_STRING(Hue_String(Test_GetLastJSON(), "apiversion"), "1.17.0");
	Test_FakeHTTPClientPacket_JSON("api/1234AB/lights/2");
	SELFTEST_ASSERT(cJSON_GetObjectItemCaseSensitive(cJSON_GetArrayItem(Test_GetLastJSON(), 0), "error") != 0);

	// 8. OpenBeken's own REST API under /api/ still answers as before
	Test_FakeHTTPClientPacket_JSON("api/info");
	SELFTEST_ASSERT(cJSON_GetObjectItemCaseSensitive(Test_GetLastJSON(), "uptime_s") != 0);
	Test_FakeHTTPClientPacket_JSON("api/pins");
	SELFTEST_ASSERT(cJSON_GetObjectItemCaseSensitive(Test_GetLastJSON(), "roles") != 0);
}

void Test_Hue_SingleChannel() {
	SIM_ClearOBK(0);
	PIN_SetPinRoleForPinIndex(24, IOR_PWM);
	PIN_SetPinChannelForPinIndex(24, 1);
	CMD_ExecuteCommand("startDriver SSDP", 0);
	CMD_ExecuteCommand("startDriver Hue", 0);
	CMD_ExecuteCommand("led_enableAll 1", 0);

	// a plain dimmer has no colour temperature
	Test_FakeHTTPClientPacket_JSON("api/1234AB/lights/1");
	SELFTEST_ASSERT_STRING(Hue_String(Hue_Light1(), "type"), "Dimmable light");
	SELFTEST_ASSERT(Hue_StateInt("ct") == -1);
	Test_FakeHTTPClientPacket_PUT_withJSONReply("api/1234AB/lights/1/state", "{\"ct\":300}");
	SELFTEST_ASSERT(!Hue_ReplyConfirms("/lights/1/state/ct"));
}

void Test_Hue() {
	Test_Hue_CW();
	Test_Hue_SingleChannel();
}

#endif
