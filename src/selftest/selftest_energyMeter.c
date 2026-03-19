#ifdef WINDOWS

#include "selftest_local.h"

#if ENABLE_BL_SHARED


void Test_EnergyMeter_Basic() {
	SIM_ClearOBK(0);
	SIM_ClearAndPrepareForMQTTTesting("miscDevice", "bekens");

	PIN_SetPinRoleForPinIndex(9, IOR_Relay);
	PIN_SetPinChannelForPinIndex(9, 1);


	CMD_ExecuteCommand("startDriver TESTPOWER", 0);
	CMD_ExecuteCommand("SetupTestPower 0 0 0 0 0", 0);

	Sim_RunSeconds(10, false);
	SELFTEST_ASSERT_EXPRESSION("$voltage", 0);
	SELFTEST_ASSERT_EXPRESSION("$current", 0);
	SELFTEST_ASSERT_EXPRESSION("$power", 0);
	SELFTEST_ASSERT_EXPRESSION("$frequency", 0);
	CMD_ExecuteCommand("SetupTestPower 230 0.26 60 50.00 0", 0);
	Sim_RunSeconds(10, false);
	SELFTEST_ASSERT_EXPRESSION("$voltage", 230);
	SELFTEST_ASSERT_EXPRESSION("$current", 0.26f);
	SELFTEST_ASSERT_EXPRESSION("$power", 60);
	SELFTEST_ASSERT_EXPRESSION("$frequency", 50.0f);

	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_FLOAT("miscDevice/voltage/get", 230.0f, false);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_FLOAT("miscDevice/current/get", 0.26f, false);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_FLOAT("miscDevice/power/get", 60.0f, false);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_FLOAT("miscDevice/138/get", 50.0f, false);

	SIM_ClearMQTTHistory();

	CMD_ExecuteCommand("SetupTestPower 241 0.36 80 49.90 0", 0);
	Sim_RunSeconds(10, false);
	SELFTEST_ASSERT_EXPRESSION("$voltage", 241);
	SELFTEST_ASSERT_EXPRESSION("$current", 0.36f);
	SELFTEST_ASSERT_EXPRESSION("$power", 80);
	SELFTEST_ASSERT_EXPRESSION("$frequency", 49.90f);

	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_FLOAT("miscDevice/voltage/get", 241.0f, false);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_FLOAT("miscDevice/current/get", 0.36f, false);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_FLOAT("miscDevice/power/get", 80.0f, false);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_FLOAT("miscDevice/138/get", 49.9f, false);

	SIM_ClearMQTTHistory();

	CMD_ExecuteCommand("SetupTestPower 221 0.46 70 52.4 0", 0);
	Sim_RunSeconds(10, false);
	SELFTEST_ASSERT_EXPRESSION("$voltage", 221);
	SELFTEST_ASSERT_EXPRESSION("$current", 0.46f);
	SELFTEST_ASSERT_EXPRESSION("$power", 70);
	SELFTEST_ASSERT_EXPRESSION("$frequency", 52.4f);


	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_FLOAT("miscDevice/voltage/get", 221.0f, false);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_FLOAT("miscDevice/current/get", 0.46f, false);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_FLOAT("miscDevice/power/get", 70.0f, false);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_FLOAT("miscDevice/138/get", 52.4f, false);


	SIM_ClearMQTTHistory();
}

// this takes values like 230V, etc
// and creates a fake BL0942 packet that is added to UART,
// it uses "default" calibration values
void Sim_SendFakeBL0942Packet(float v, float c, float p);

void Test_EnergyMeter_BL0942() {
	SIM_ClearOBK(0);
	SIM_ClearAndPrepareForMQTTTesting("miscDevice", "bekens");

	PIN_SetPinRoleForPinIndex(9, IOR_Relay);
	PIN_SetPinChannelForPinIndex(9, 1);

	CMD_ExecuteCommand("startDriver BL0942", 0);

	// set initial values reported by BL0942 via spoofed UART packets
	Sim_SendFakeBL0942Packet(230, 0.26, 60);
	Sim_RunSeconds(1, false);
	// simulate using doing calibration
	CMD_ExecuteCommand("PowerSet 60", 0);
	CMD_ExecuteCommand("VoltageSet 230", 0);
	CMD_ExecuteCommand("CurrentSet 0.26", 0);
	Sim_SendFakeBL0942Packet(230, 0.26, 60);
	Sim_RunSeconds(1, false);
	// verify calibration worked
	SELFTEST_ASSERT_EXPRESSION("$voltage", 230);
	SELFTEST_ASSERT_EXPRESSION("$current", 0.26f);
	SELFTEST_ASSERT_EXPRESSION("$power", 60);
	// test different current
	Sim_SendFakeBL0942Packet(230, 0.13, 30);
	Sim_RunSeconds(1, false);
	SELFTEST_ASSERT_EXPRESSION("$voltage", 230);
	SELFTEST_ASSERT_EXPRESSION("$current", 0.13f);
	SELFTEST_ASSERT_EXPRESSION("$power", 30);
	// simulate user calibrating 30W from BL0942 to be 60W IRL
	CMD_ExecuteCommand("PowerSet 60", 0);
	Sim_SendFakeBL0942Packet(230, 0.13, 30);
	Sim_RunSeconds(1, false);
	SELFTEST_ASSERT_EXPRESSION("$power", 60);
	Sim_RunSeconds(1, false);
	// now. if 30W from BL0942 is 60W IRL,
	// then 60W frm BL0942 should give us 120W
	Sim_SendFakeBL0942Packet(230, 0.13, 60);
	Sim_RunSeconds(1, false);
	SELFTEST_ASSERT_EXPRESSION("$power", 120);
	// as above, but non-integer
	Sim_SendFakeBL0942Packet(230, 0.13, 30);
	Sim_RunSeconds(1, false);
	CMD_ExecuteCommand("PowerSet 60.5", 0);
	Sim_SendFakeBL0942Packet(230, 0.13, 30);
	Sim_RunSeconds(1, false);
	SELFTEST_ASSERT_EXPRESSION("$power", 60.5);
	Sim_RunSeconds(1, false);
	// now. if 30W from BL0942 is 60.5W IRL,
	// then 60W frm BL0942 should give us 121W
	Sim_SendFakeBL0942Packet(230, 0.13, 60);
	Sim_RunSeconds(1, false);
	SELFTEST_ASSERT_EXPRESSION("$power", 121);

	SIM_ClearMQTTHistory();
}
void Test_EnergyMeter_CSE7766() {
	SIM_ClearOBK(0);
	SIM_ClearAndPrepareForMQTTTesting("miscDevice", "bekens");

	PIN_SetPinRoleForPinIndex(9, IOR_Relay);
	PIN_SetPinChannelForPinIndex(9, 1);

	CMD_ExecuteCommand("startDriver CSE7766", 0);

	CMD_ExecuteCommand("uartFakeHex 555A02FCD800062F00413200D7F2537B18023E9F7171FEEC", 0);
	CSE7766_RunEverySecond();


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
	CMD_ExecuteCommand("SetupTestPower 0 0 0 0 0", 0);
	CMD_ExecuteCommand("SetChannel 10 0", 0);
	CMD_ExecuteCommand("addChangeHandler Power > 60 SetChannel 10 1234", 0);
	SELFTEST_ASSERT_CHANNEL(10, 0);
	Sim_RunSeconds(10, false);
	CMD_ExecuteCommand("SetupTestPower 230 0.26 50 0 0", 0);
	Sim_RunSeconds(10, false);
	SELFTEST_ASSERT_CHANNEL(10, 0);
	CMD_ExecuteCommand("addChangeHandler Power < 40 SetChannel 10 2345", 0);
	// transition 50->80 will trigger set 1234
	CMD_ExecuteCommand("SetupTestPower 241 0.36 80 0 0", 0);
	Sim_RunSeconds(10, false);
	SELFTEST_ASSERT_CHANNEL(10, 1234);
	Sim_RunSeconds(10, false);
	// transition 80->10 will trigger set 2345
	CMD_ExecuteCommand("SetupTestPower 241 0.36 10 0 0", 0);
	Sim_RunSeconds(10, false);
	SELFTEST_ASSERT_CHANNEL(10, 2345);
	// transition 10->20 will not change anything
	CMD_ExecuteCommand("SetChannel 10 1111", 0);
	CMD_ExecuteCommand("SetupTestPower 241 0.36 20 0 0", 0);
	Sim_RunSeconds(10, false);
	SELFTEST_ASSERT_CHANNEL(10, 1111);
	// transition 20->30 will not change anything
	CMD_ExecuteCommand("SetupTestPower 241 0.36 30 0 0", 0);
	Sim_RunSeconds(10, false);
	SELFTEST_ASSERT_CHANNEL(10, 1111);
	// transition 30->35 will not change anything
	CMD_ExecuteCommand("SetupTestPower 241 0.36 35 0 0", 0);
	Sim_RunSeconds(10, false);
	SELFTEST_ASSERT_CHANNEL(10, 1111);
	// transition 35->38 will not change anything
	CMD_ExecuteCommand("SetupTestPower 241 0.36 38 0 0", 0);
	Sim_RunSeconds(10, false);
	SELFTEST_ASSERT_CHANNEL(10, 1111);
	// transition 38->68 will trigger set 1234
	CMD_ExecuteCommand("SetupTestPower 241 0.36 68 0 0", 0);
	Sim_RunSeconds(10, false);
	SELFTEST_ASSERT_CHANNEL(10, 1234);
	// transition 68->20 will trigger set 2345
	CMD_ExecuteCommand("SetupTestPower 241 0.36 20 0 0", 0);
	Sim_RunSeconds(10, false);
	SELFTEST_ASSERT_CHANNEL(10, 2345);

	// same works for voltage
	CMD_ExecuteCommand("addChangeHandler Voltage > 251 SetChannel 11 5555", 0);
	SELFTEST_ASSERT_CHANNEL(11, 0);
	CMD_ExecuteCommand("SetupTestPower 241 0.36 20 0 0", 0);
	Sim_RunSeconds(10, false);
	SELFTEST_ASSERT_CHANNEL(11, 0);
	CMD_ExecuteCommand("SetupTestPower 245 0.36 20 0 0", 0);
	Sim_RunSeconds(10, false);
	SELFTEST_ASSERT_CHANNEL(11, 0);
	// This will trigger - voltage is over 251
	CMD_ExecuteCommand("SetupTestPower 255 0.36 20 0 0", 0);
	Sim_RunSeconds(10, false);
	SELFTEST_ASSERT_CHANNEL(11, 5555);
	CMD_ExecuteCommand("SetChannel 11 5", 0);
	SELFTEST_ASSERT_CHANNEL(11, 5);
	// no trigger
	CMD_ExecuteCommand("SetupTestPower 256 0.36 20 0 0", 0);
	Sim_RunSeconds(10, false);
	SELFTEST_ASSERT_CHANNEL(11, 5);
	// no trigger
	CMD_ExecuteCommand("SetupTestPower 266 0.36 20 0 0", 0);
	Sim_RunSeconds(10, false);
	SELFTEST_ASSERT_CHANNEL(11, 5);
	// no trigger
	CMD_ExecuteCommand("SetupTestPower 276 0.36 20 0 0", 0);
	Sim_RunSeconds(10, false);
	SELFTEST_ASSERT_CHANNEL(11, 5);
	// no trigger
	CMD_ExecuteCommand("SetupTestPower 221 0.36 20 0 0", 0);
	Sim_RunSeconds(10, false);
	SELFTEST_ASSERT_CHANNEL(11, 5);
	// This will trigger - voltage is over 251
	CMD_ExecuteCommand("SetupTestPower 255 0.36 20 0 0", 0);
	Sim_RunSeconds(10, false);
	SELFTEST_ASSERT_CHANNEL(11, 5555);

	// current is * 1000 mA, so .093A becomes 93
	// mind the float precision gap
	CMD_ExecuteCommand("SetChannel 12 0", 0);
	CMD_ExecuteCommand("addChangeHandler Current > 93 SetChannel 12 1212", 0);
	SELFTEST_ASSERT_CHANNEL(12, 0);
	CMD_ExecuteCommand("SetupTestPower 230 0.001 213 50.1 0", 0);
	Sim_RunSeconds(10, false);
	SELFTEST_ASSERT_CHANNEL(12, 0);
	CMD_ExecuteCommand("SetupTestPower 230 0.090 213 50.1 0", 0);
	Sim_RunSeconds(10, false);
	SELFTEST_ASSERT_CHANNEL(12, 0);
	// This will trigger - current over 93mA
	CMD_ExecuteCommand("SetupTestPower 230 0.095 213 50.1 0", 0);
	Sim_RunSeconds(10, false);
	SELFTEST_ASSERT_CHANNEL(12, 1212);
	CMD_ExecuteCommand("SetChannel 12 5", 0);
	SELFTEST_ASSERT_CHANNEL(12, 5);
	// no trigger
	CMD_ExecuteCommand("SetupTestPower 230 123.456 213 50.1 0", 0);
	Sim_RunSeconds(10, false);
	SELFTEST_ASSERT_CHANNEL(12, 5);
	// no trigger
	CMD_ExecuteCommand("SetupTestPower 230 .095 213 50.1 0", 0);
	Sim_RunSeconds(10, false);
	SELFTEST_ASSERT_CHANNEL(12, 5);
	// no trigger
	CMD_ExecuteCommand("SetupTestPower 230 1.9 213 50.01 0", 0);
	Sim_RunSeconds(10, false);
	SELFTEST_ASSERT_CHANNEL(12, 5);
	// no trigger, but drop below threshold
	CMD_ExecuteCommand("SetupTestPower 230 .091 213 50.1 0", 0);
	Sim_RunSeconds(10, false);
	SELFTEST_ASSERT_CHANNEL(12, 5);
	// This will trigger - current is over 93mA
	CMD_ExecuteCommand("SetupTestPower 230 1.096 213 50.1 0", 0);
	Sim_RunSeconds(10, false);
	SELFTEST_ASSERT_CHANNEL(12, 1212);

	// frequency is * 100, so 53.10 becomes 5310
	CMD_ExecuteCommand("addChangeHandler Frequency > 5310 SetChannel 13 1313", 0);
	SELFTEST_ASSERT_CHANNEL(13, 0);
	CMD_ExecuteCommand("SetupTestPower 230 0.36 20 49.9 0", 0);
	Sim_RunSeconds(10, false);
	SELFTEST_ASSERT_CHANNEL(13, 0);
	CMD_ExecuteCommand("SetupTestPower 230 0.36 20 50.1 0", 0);
	Sim_RunSeconds(10, false);
	SELFTEST_ASSERT_CHANNEL(13, 0);
	// This will trigger - frequency over 53.10
	CMD_ExecuteCommand("SetupTestPower 230 0.36 20 60 0", 0);
	Sim_RunSeconds(10, false);
	SELFTEST_ASSERT_CHANNEL(13, 1313);
	CMD_ExecuteCommand("SetChannel 13 5", 0);
	SELFTEST_ASSERT_CHANNEL(13, 5);
	// no trigger
	CMD_ExecuteCommand("SetupTestPower 230 0.36 20 56.9 0", 0);
	Sim_RunSeconds(10, false);
	SELFTEST_ASSERT_CHANNEL(13, 5);
	// no trigger
	CMD_ExecuteCommand("SetupTestPower 230 0.36 20 70 0", 0);
	Sim_RunSeconds(10, false);
	SELFTEST_ASSERT_CHANNEL(13, 5);
	// no trigger
	CMD_ExecuteCommand("SetupTestPower 230 0.36 20 53.11 0", 0);
	Sim_RunSeconds(10, false);
	SELFTEST_ASSERT_CHANNEL(13, 5);
	// no trigger, but drop below threshold
	CMD_ExecuteCommand("SetupTestPower 230 0.36 20 52.00 0", 0);
	Sim_RunSeconds(10, false);
	SELFTEST_ASSERT_CHANNEL(13, 5);
	// This will trigger - frequency is over 53.10
	CMD_ExecuteCommand("SetupTestPower 230 0.36 20 53.11 0", 0);
	Sim_RunSeconds(10, false);
	SELFTEST_ASSERT_CHANNEL(13, 1313);
}
void Test_EnergyMeter_Tasmota() {
	SIM_ClearOBK(0);
	SIM_ClearAndPrepareForMQTTTesting("miscDevice", "bekens");

	PIN_SetPinRoleForPinIndex(9, IOR_Relay);
	PIN_SetPinChannelForPinIndex(9, 1);


	CMD_ExecuteCommand("startDriver TESTPOWER", 0);
	CMD_ExecuteCommand("SetupTestPower 0 0 0 0 0", 0);

	Sim_RunSeconds(10, false);
	CMD_ExecuteCommand("SetupTestPower 230 0.26 60 50.10 0", 0);
	Sim_RunSeconds(10, false);


	// read power
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=STATUS%208");
	SELFTEST_ASSERT_JSON_VALUE_FLOAT_NESTED2("StatusSNS", "ENERGY", "Voltage", 230);
	SELFTEST_ASSERT_JSON_VALUE_FLOAT_NESTED2("StatusSNS", "ENERGY", "Current", 0.26f);
	SELFTEST_ASSERT_JSON_VALUE_FLOAT_NESTED2("StatusSNS", "ENERGY", "Power", 60.0f);
	SELFTEST_ASSERT_JSON_VALUE_FLOAT_NESTED2("StatusSNS", "ENERGY", "Frequency", 50.10f);

	Sim_RunSeconds(10, false);
	CMD_ExecuteCommand("SetupTestPower 240 0.31 70 49.99 0", 0);
	Sim_RunSeconds(10, false);

	// read power
	Test_FakeHTTPClientPacket_JSON("cm?cmnd=STATUS%208");
	SELFTEST_ASSERT_JSON_VALUE_FLOAT_NESTED2("StatusSNS", "ENERGY", "Voltage", 240);
	SELFTEST_ASSERT_JSON_VALUE_FLOAT_NESTED2("StatusSNS", "ENERGY", "Current", 0.31f);
	SELFTEST_ASSERT_JSON_VALUE_FLOAT_NESTED2("StatusSNS", "ENERGY", "Power", 70.0f);
	SELFTEST_ASSERT_JSON_VALUE_FLOAT_NESTED2("StatusSNS", "ENERGY", "Frequency", 49.99f);

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
	CMD_ExecuteCommand("SetupTestPower 0 0 0 0 0", 0);

	CMD_ExecuteCommand("SetupTestPower 230 0.01 0.5 50.00 0", 0);
	int prevChannel10 = CHANNEL_Get(10);
	for (int i = 0; i < 10; i++) {
		Test_Power_RunEverySecond();
		Sim_RunSeconds(1.5f, false);
		int now10 = CHANNEL_Get(10);
		SELFTEST_ASSERT(now10 > prevChannel10);
		prevChannel10 = now10;
		SELFTEST_ASSERT_CHANNEL(1, 1);
	}
	// now reset will kick in
	CMD_ExecuteCommand("SetupTestPower 230 0.01 1.5 50.00 0", 0);
	for (int i = 0; i < 10; i++) {
		Test_Power_RunEverySecond();
		Sim_RunSeconds(1.5f, false);
		int now10 = CHANNEL_Get(10);
		SELFTEST_ASSERT(now10 == 0);
		prevChannel10 = now10;
		SELFTEST_ASSERT_CHANNEL(1, 1);
	}
	// again counting
	CMD_ExecuteCommand("SetupTestPower 230 0.01 0.5 50.00 0", 0);
	Sim_RunSeconds(1.5f, false);
	prevChannel10 = CHANNEL_Get(10);
	for (int i = 0; i < 10; i++) {
		Test_Power_RunEverySecond();
		Sim_RunSeconds(1.5f, false);
		int now10 = CHANNEL_Get(10);
		SELFTEST_ASSERT(now10 > prevChannel10);
		prevChannel10 = now10;
		SELFTEST_ASSERT_CHANNEL(1, 1);
	}
	// now reset will kick in
	CMD_ExecuteCommand("SetupTestPower 230 0.01 1.5 50.00 0", 0);
	for (int i = 0; i < 10; i++) {
		Test_Power_RunEverySecond();
		Sim_RunSeconds(1.5f, false);
		int now10 = CHANNEL_Get(10);
		SELFTEST_ASSERT(now10 == 0);
		prevChannel10 = now10;
		SELFTEST_ASSERT_CHANNEL(1, 1);
	}
	CMD_ExecuteCommand("SetupTestPower 230 0.01 0.5 50.00 0", 0);
	prevChannel10 = CHANNEL_Get(10);
	for (int i = 0; i < 12; i++) {
		Test_Power_RunEverySecond();
		Sim_RunSeconds(1.5f, false);
		int now10 = CHANNEL_Get(10);
		SELFTEST_ASSERT(now10 > prevChannel10);
		prevChannel10 = now10;
		SELFTEST_ASSERT_CHANNEL(1, 1);
	}
	// this loop should trigger turn off
	prevChannel10 = CHANNEL_Get(10);
	for (int i = 0; i < 3; i++) {
		Test_Power_RunEverySecond();
		Sim_RunSeconds(1.5f, false);
		int now10 = CHANNEL_Get(10);
		SELFTEST_ASSERT(now10 > prevChannel10);
		prevChannel10 = now10;
	}
	// turn off should have been triggered
	SELFTEST_ASSERT_CHANNEL(1, 2015);


	SIM_ClearMQTTHistory();
}

void Test_EnergyMeter_Limits() {
	SIM_ClearOBK(0);
	SIM_ClearAndPrepareForMQTTTesting("powerDevice", "bekens");

	PIN_SetPinRoleForPinIndex(9, IOR_Relay);
	PIN_SetPinChannelForPinIndex(9, 1);

	SIM_ClearMQTTHistory();

	CMD_ExecuteCommand("startDriver TESTPOWER", 0);
	Sim_RunSeconds(5, false);
	CMD_ExecuteCommand("SetupTestPower 230 2.0 460 49.90 0", 0);
	Sim_RunSeconds(60, false);

	CMD_ExecuteCommand("VCPPublishThreshold 5 0.5 10", 0);
	CMD_ExecuteCommand("SetupTestPower 233 2.2 463 49.90 0", 0);
	Sim_RunSeconds(5, false);
	// VCPPubishThreshold not reached - values should not have changed
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_FLOAT("powerDevice/voltage/get", 230, false);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_FLOAT("powerDevice/current/get", 2.0f, false);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_FLOAT("powerDevice/power/get", 460, false);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_FLOAT("powerDevice/138/get", 49.9f, false);

	CMD_ExecuteCommand("SetupTestPower 240 2.9 695 49.90 0", 0);
	Sim_RunSeconds(5, false);
	SELFTEST_ASSERT_EXPRESSION("$voltage", 240);
	SELFTEST_ASSERT_EXPRESSION("$current", 2.9f);
	SELFTEST_ASSERT_EXPRESSION("$power", 695);
	SELFTEST_ASSERT_EXPRESSION("$frequency", 49.90f);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_FLOAT("powerDevice/voltage/get", 240, false);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_FLOAT("powerDevice/current/get", 2.9f, false);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_FLOAT("powerDevice/power/get", 695, false);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_FLOAT("powerDevice/138/get", 49.9f, false);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_FLOAT("powerDevice/power_apparent/get", 512.60f, false);

	CMD_ExecuteCommand("VCPPublishThreshold 5 0.5 5 5 0.5", 0);
	CMD_ExecuteCommand("SetupTestPower 240 2.9 695 50.00 0", 0);
	Sim_RunSeconds(5, false);
	// VCPPubishThreshold not reached - values should not have changed
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_FLOAT("powerDevice/voltage/get", 240, false);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_FLOAT("powerDevice/current/get", 2.9f, false);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_FLOAT("powerDevice/power/get", 695, false);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_FLOAT("powerDevice/138/get", 49.9f, false);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_FLOAT("powerDevice/power_apparent/get", 696.0f, false);

	CMD_ExecuteCommand("SetupTestPower 240 2.9 705 50.70 0", 0);
	Sim_RunSeconds(10, false);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_FLOAT("powerDevice/voltage/get", 240, false);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_FLOAT("powerDevice/current/get", 2.9f, false);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_FLOAT("powerDevice/power/get", 705, false);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_FLOAT("powerDevice/138/get", 50.7f, false);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_FLOAT("powerDevice/power_apparent/get", 696.0f, false);

}
void Test_EnergyMeter_ResetBug() {
	SIM_ClearOBK(0);
	SIM_ClearAndPrepareForMQTTTesting("miscDevice", "bekens");

	PIN_SetPinRoleForPinIndex(9, IOR_Relay);
	PIN_SetPinChannelForPinIndex(9, 1);

	CMD_ExecuteCommand("startDriver TESTPOWER", 0);
	// 230V, 10A, 2300W, 50Hz
	CMD_ExecuteCommand("SetupTestPower 230 10 2300 50 0", 0);
	CMD_ExecuteCommand("SetupEnergyStats 1 60 60 0", 0);

	// Run for a while to accumulate energy
	// 100 seconds at 2300W
	// 2300W * 100s = 230,000 Joules
	// 230,000 / 3600 = 63.88 Wh
	Sim_RunSeconds(100, false);

	// Verify we have some energy
	// $energy is in kWh for TOTAL
	// 63.88 Wh = 0.06388 kWh
	// So $energy is > 0.06 but < 0.07
	SELFTEST_ASSERT_EXPRESSION("$energy>0.06", 1);
	SELFTEST_ASSERT_EXPRESSION("$energy<0.07", 1);
	SELFTEST_ASSERT_EXPRESSION("$today>0.06", 1);
	SELFTEST_ASSERT_EXPRESSION("$yesterday==0", 1);

	// reset with 0???
	CMD_ExecuteCommand("EnergyCntReset", 0);

	// Verify TOTAL is 0
	SELFTEST_ASSERT_EXPRESSION("$energycounter", 0);

	// Verify TODAY is 0
	SELFTEST_ASSERT_EXPRESSION("$today", 0);
	SELFTEST_ASSERT_EXPRESSION("$yesterday", 0);

	SIM_ClearMQTTHistory();
}
void Test_EnergyMeter() {
	Test_EnergyMeter_ResetBug();
	Test_EnergyMeter_CSE7766();
#ifndef LINUX
	// TODO: fix on Linux
	Test_EnergyMeter_BL0942();
#endif
	Test_EnergyMeter_Basic();
	Test_EnergyMeter_Tasmota();
	Test_EnergyMeter_Events();
	Test_EnergyMeter_TurnOffScript();
	Test_EnergyMeter_Limits();
}

#endif
#endif
