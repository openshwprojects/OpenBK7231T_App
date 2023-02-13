#ifdef WINDOWS

#include "selftest_local.h"

void Test_EnergyMeter_Basic() {
	SIM_ClearOBK();
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
void Test_EnergyMeter_Tasmota() {
	SIM_ClearOBK();
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
}

#endif
