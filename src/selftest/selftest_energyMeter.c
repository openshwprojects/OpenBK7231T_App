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
	CMD_ExecuteCommand("SetupTestPower 230 0.26 60 0", 0);
	Sim_RunSeconds(10, false);

	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_FLOAT("miscDevice/voltage/get", 230.0f, false);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_FLOAT("miscDevice/current/get", 0.26f, false);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_FLOAT("miscDevice/power/get", 60.0f, false);

	SIM_ClearMQTTHistory();

	CMD_ExecuteCommand("SetupTestPower 241 0.36 80 0", 0);
	Sim_RunSeconds(10, false);

	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_FLOAT("miscDevice/voltage/get", 241.0f, false);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_FLOAT("miscDevice/current/get", 0.36f, false);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_FLOAT("miscDevice/power/get", 80.0f, false);

	SIM_ClearMQTTHistory();

	CMD_ExecuteCommand("SetupTestPower 221 0.46 70 0", 0);
	Sim_RunSeconds(10, false);

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
void Test_EnergyMeter() {
	Test_EnergyMeter_Basic();
	Test_EnergyMeter_Tasmota();
	Test_EnergyMeter_Events();
}

#endif
