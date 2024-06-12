#ifdef WINDOWS

#include "selftest_local.h"

void Test_EnergyMeter_Basic() {
	SIM_ClearOBK(0);
	SIM_ClearAndPrepareForMQTTTesting("miscDevice", "bekens");

	PIN_SetPinRoleForPinIndex(9, IOR_Relay);
	PIN_SetPinChannelForPinIndex(9, 1);


	CMD_ExecuteCommand("startDriver TESTPOWER", 0);
	CMD_ExecuteCommand("SetupTestPower 0 0 0 0", 0);

	Sim_RunSeconds(10, false);
	SELFTEST_ASSERT_EXPRESSION("$voltage", 0);
	SELFTEST_ASSERT_EXPRESSION("$current", 0);
	SELFTEST_ASSERT_EXPRESSION("$power", 0);
	CMD_ExecuteCommand("SetupTestPower 230 0.26 60 0", 0);
	Sim_RunSeconds(10, false);
	SELFTEST_ASSERT_EXPRESSION("$voltage", 230);
	SELFTEST_ASSERT_EXPRESSION("$current", 0.26f);
	SELFTEST_ASSERT_EXPRESSION("$power", 60);

	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_FLOAT("miscDevice/voltage/get", 230.0f, false);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_FLOAT("miscDevice/current/get", 0.26f, false);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_FLOAT("miscDevice/power/get", 60.0f, false);

	SIM_ClearMQTTHistory();

	CMD_ExecuteCommand("SetupTestPower 241 0.36 80 0", 0);
	Sim_RunSeconds(10, false);
	SELFTEST_ASSERT_EXPRESSION("$voltage", 241);
	SELFTEST_ASSERT_EXPRESSION("$current", 0.36f);
	SELFTEST_ASSERT_EXPRESSION("$power", 80);

	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_FLOAT("miscDevice/voltage/get", 241.0f, false);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_FLOAT("miscDevice/current/get", 0.36f, false);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_FLOAT("miscDevice/power/get", 80.0f, false);

	SIM_ClearMQTTHistory();

	CMD_ExecuteCommand("SetupTestPower 221 0.46 70 0", 0);
	Sim_RunSeconds(10, false);
	SELFTEST_ASSERT_EXPRESSION("$voltage", 221);
	SELFTEST_ASSERT_EXPRESSION("$current", 0.46f);
	SELFTEST_ASSERT_EXPRESSION("$power", 70);


	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_FLOAT("miscDevice/voltage/get", 221.0f, false);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_FLOAT("miscDevice/current/get", 0.46f, false);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_FLOAT("miscDevice/power/get", 70.0f, false);

	SIM_ClearMQTTHistory();
}
void Test_EnergyMeter_Events() {
	SIM_ClearOBK(0);
	SIM_ClearAndPrepareForMQTTTesting("miscDevice", "bekens");

	PIN_SetPinRoleForPinIndex(9, IOR_Relay);
	PIN_SetPinChannelForPinIndex(9, 1);
	
	CMD_ExecuteCommand("startDriver TESTPOWER", 0);
	// TODO: fixme
	CMD_ExecuteCommand("SetupEnergyStats 1 60 60 0", 0);
	CMD_ExecuteCommand("SetupTestPower 0 0 0 0", 0);
	CMD_ExecuteCommand("SetChannel 10 0", 0);
	CMD_ExecuteCommand("addChangeHandler Power > 60 SetChannel 10 1234", 0);
	SELFTEST_ASSERT_CHANNEL(10, 0);
	Sim_RunSeconds(10, false);
	CMD_ExecuteCommand("SetupTestPower 230 0.26 50 0", 0);
	Sim_RunSeconds(10, false);
	SELFTEST_ASSERT_CHANNEL(10, 0);
	CMD_ExecuteCommand("addChangeHandler Power < 40 SetChannel 10 2345", 0);
	// transition 50->80 will trigger set 1234
	CMD_ExecuteCommand("SetupTestPower 241 0.36 80 0", 0);
	Sim_RunSeconds(10, false);
	SELFTEST_ASSERT_CHANNEL(10, 1234);
	Sim_RunSeconds(10, false);
	// transition 80->10 will trigger set 2345
	CMD_ExecuteCommand("SetupTestPower 241 0.36 10 0", 0);
	Sim_RunSeconds(10, false);
	SELFTEST_ASSERT_CHANNEL(10, 2345);
	// transition 10->20 will not change anything
	CMD_ExecuteCommand("SetChannel 10 1111", 0);
	CMD_ExecuteCommand("SetupTestPower 241 0.36 20 0", 0);
	Sim_RunSeconds(10, false);
	SELFTEST_ASSERT_CHANNEL(10, 1111);
	// transition 20->30 will not change anything
	CMD_ExecuteCommand("SetupTestPower 241 0.36 30 0", 0);
	Sim_RunSeconds(10, false);
	SELFTEST_ASSERT_CHANNEL(10, 1111);
	// transition 30->35 will not change anything
	CMD_ExecuteCommand("SetupTestPower 241 0.36 35 0", 0);
	Sim_RunSeconds(10, false);
	SELFTEST_ASSERT_CHANNEL(10, 1111);
	// transition 35->38 will not change anything
	CMD_ExecuteCommand("SetupTestPower 241 0.36 38 0", 0);
	Sim_RunSeconds(10, false);
	SELFTEST_ASSERT_CHANNEL(10, 1111);
	// transition 38->68 will trigger set 1234
	CMD_ExecuteCommand("SetupTestPower 241 0.36 68 0", 0);
	Sim_RunSeconds(10, false);
	SELFTEST_ASSERT_CHANNEL(10, 1234);
	// transition 68->20 will trigger set 2345
	CMD_ExecuteCommand("SetupTestPower 241 0.36 20 0", 0);
	Sim_RunSeconds(10, false);
	SELFTEST_ASSERT_CHANNEL(10, 2345);

	// same works for voltage
	CMD_ExecuteCommand("addChangeHandler Voltage > 251 SetChannel 11 5555", 0);
	SELFTEST_ASSERT_CHANNEL(11, 0);
	CMD_ExecuteCommand("SetupTestPower 241 0.36 20 0", 0);
	Sim_RunSeconds(10, false);
	SELFTEST_ASSERT_CHANNEL(11, 0);
	CMD_ExecuteCommand("SetupTestPower 245 0.36 20 0", 0);
	Sim_RunSeconds(10, false);
	SELFTEST_ASSERT_CHANNEL(11, 0);
	// This will trigger - voltage is over 251
	CMD_ExecuteCommand("SetupTestPower 255 0.36 20 0", 0);
	Sim_RunSeconds(10, false);
	SELFTEST_ASSERT_CHANNEL(11, 5555);
	CMD_ExecuteCommand("SetChannel 11 5", 0);
	SELFTEST_ASSERT_CHANNEL(11, 5);
	// no trigger
	CMD_ExecuteCommand("SetupTestPower 256 0.36 20 0", 0);
	Sim_RunSeconds(10, false);
	SELFTEST_ASSERT_CHANNEL(11, 5);
	// no trigger
	CMD_ExecuteCommand("SetupTestPower 266 0.36 20 0", 0);
	Sim_RunSeconds(10, false);
	SELFTEST_ASSERT_CHANNEL(11, 5);
	// no trigger
	CMD_ExecuteCommand("SetupTestPower 276 0.36 20 0", 0);
	Sim_RunSeconds(10, false);
	SELFTEST_ASSERT_CHANNEL(11, 5);
	// no trigger
	CMD_ExecuteCommand("SetupTestPower 221 0.36 20 0", 0);
	Sim_RunSeconds(10, false);
	SELFTEST_ASSERT_CHANNEL(11, 5);
	// This will trigger - voltage is over 251
	CMD_ExecuteCommand("SetupTestPower 255 0.36 20 0", 0);
	Sim_RunSeconds(10, false);
	SELFTEST_ASSERT_CHANNEL(11, 5555);
}
void Test_EnergyMeter_Tasmota() {
	SIM_ClearOBK(0);
	SIM_ClearAndPrepareForMQTTTesting("miscDevice", "bekens");

	PIN_SetPinRoleForPinIndex(9, IOR_Relay);
	PIN_SetPinChannelForPinIndex(9, 1);


	CMD_ExecuteCommand("startDriver TESTPOWER", 0);
	CMD_ExecuteCommand("SetupTestPower 0 0 0 0", 0);

	Sim_RunSeconds(10, false);
	CMD_ExecuteCommand("SetupTestPower 230 0.26 60 0", 0);
	Sim_RunSeconds(10, false);


	// read power
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=STATUS%208");
	SELFTEST_ASSERT_JSON_VALUE_FLOAT_NESTED2("StatusSNS", "ENERGY", "Voltage", 230);
	SELFTEST_ASSERT_JSON_VALUE_FLOAT_NESTED2("StatusSNS", "ENERGY", "Current", 0.26f);
	SELFTEST_ASSERT_JSON_VALUE_FLOAT_NESTED2("StatusSNS", "ENERGY", "Power", 60.0f);

	Sim_RunSeconds(10, false);
	CMD_ExecuteCommand("SetupTestPower 240 0.31 70 0", 0);
	Sim_RunSeconds(10, false);

	// read power
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=STATUS%208");
	SELFTEST_ASSERT_JSON_VALUE_FLOAT_NESTED2("StatusSNS", "ENERGY", "Voltage", 240);
	SELFTEST_ASSERT_JSON_VALUE_FLOAT_NESTED2("StatusSNS", "ENERGY", "Current", 0.31f);
	SELFTEST_ASSERT_JSON_VALUE_FLOAT_NESTED2("StatusSNS", "ENERGY", "Power", 70.0f);

	SIM_ClearMQTTHistory();
}

const char *test_turnOffIfNoPower =
"setChannel 10 0\r\n"
"again:\r\n"
"    delay_s 1\r\n"
"    if $power>1 then backlog setChannel 10 0; goto again\r\n"
"    addChannel 10 1\r\n"
"    if $CH10==20 then setChannel 1 2015\r\n"
"    goto again\r\n";

void Test_EnergyMeter_TurnOffScript() {
	SIM_ClearOBK(0);
	SIM_ClearAndPrepareForMQTTTesting("miscDevice", "bekens");

	// put file in LittleFS
	Test_FakeHTTPClientPacket_POST("api/lfs/myTurnOff.txt", test_turnOffIfNoPower);
	// get this file 
	Test_FakeHTTPClientPacket_GET("api/lfs/myTurnOff.txt");
	SELFTEST_ASSERT_HTML_REPLY(test_turnOffIfNoPower);

	CMD_ExecuteCommand("setChannel 1 1", 0);
	CMD_ExecuteCommand("startScript myTurnOff.txt", 0);
	SELFTEST_ASSERT_CHANNEL(1, 1);

	PIN_SetPinRoleForPinIndex(9, IOR_Relay);
	PIN_SetPinChannelForPinIndex(9, 1);

	CMD_ExecuteCommand("startDriver TESTPOWER", 0);
	CMD_ExecuteCommand("SetupTestPower 0 0 0 0", 0);

	CMD_ExecuteCommand("SetupTestPower 230 0.01 0.5 0", 0);
	int prevChannel10 = CHANNEL_Get(10);
	for (int i = 0; i < 10; i++) {
		Sim_RunSeconds(1.5f, false);
		int now10 = CHANNEL_Get(10);
		SELFTEST_ASSERT(now10 > prevChannel10);
		prevChannel10 = now10;
		SELFTEST_ASSERT_CHANNEL(1, 1);
	}
	// now reset will kick in
	CMD_ExecuteCommand("SetupTestPower 230 0.01 1.5 0", 0);
	for (int i = 0; i < 10; i++) {
		Sim_RunSeconds(1.5f, false);
		int now10 = CHANNEL_Get(10);
		SELFTEST_ASSERT(now10 == 0);
		prevChannel10 = now10;
		SELFTEST_ASSERT_CHANNEL(1, 1);
	}
	// again counting
	CMD_ExecuteCommand("SetupTestPower 230 0.01 0.5 0", 0);
	Sim_RunSeconds(1.5f, false);
	prevChannel10 = CHANNEL_Get(10);
	for (int i = 0; i < 10; i++) {
		Sim_RunSeconds(1.5f, false);
		int now10 = CHANNEL_Get(10);
		SELFTEST_ASSERT(now10 > prevChannel10);
		prevChannel10 = now10;
		SELFTEST_ASSERT_CHANNEL(1, 1);
	}
	// now reset will kick in
	CMD_ExecuteCommand("SetupTestPower 230 0.01 1.5 0", 0);
	for (int i = 0; i < 10; i++) {
		Sim_RunSeconds(1.5f, false);
		int now10 = CHANNEL_Get(10);
		SELFTEST_ASSERT(now10 == 0);
		prevChannel10 = now10;
		SELFTEST_ASSERT_CHANNEL(1, 1);
	}
	CMD_ExecuteCommand("SetupTestPower 230 0.01 0.5 0", 0);
	prevChannel10 = CHANNEL_Get(10);
	for (int i = 0; i < 12; i++) {
		Sim_RunSeconds(1.5f, false);
		int now10 = CHANNEL_Get(10);
		SELFTEST_ASSERT(now10 > prevChannel10);
		prevChannel10 = now10;
		SELFTEST_ASSERT_CHANNEL(1, 1);
	}
	// this loop should trigger turn off
	prevChannel10 = CHANNEL_Get(10);
	for (int i = 0; i < 3; i++) {
		Sim_RunSeconds(1.5f, false);
		int now10 = CHANNEL_Get(10);
		SELFTEST_ASSERT(now10 > prevChannel10);
		prevChannel10 = now10;
	}
	// turn off should have been triggered
	SELFTEST_ASSERT_CHANNEL(1, 2015);


	SIM_ClearMQTTHistory();
}
void Test_EnergyMeter() {
	Test_EnergyMeter_Basic();
	Test_EnergyMeter_Tasmota();
	Test_EnergyMeter_Events();
	Test_EnergyMeter_TurnOffScript();
}

#endif
