#ifdef WINDOWS

#include "selftest_local.h"

void Test_ChangeHandlers_MQTT() {
	// reset whole device
	SIM_ClearOBK(0);
	SIM_ClearAndPrepareForMQTTTesting("handlerTester","bekens");

	// change handlers and MQTT
	// this will only happens when Channel12 value changes from not equal to 0 to the one equal to 0
	CMD_ExecuteCommand("addChangeHandler Channel12 == 0 publish myChannel valueIsZero", 0);
	// this will only happens when Channel12 value changes from not equal to 1 to the one equal to 1
	CMD_ExecuteCommand("addChangeHandler Channel12 == 1 publish myChannel valueIsOne", 0);
	// this will only happens when Channel12 value changes from not equal to 2 to the one equal to 2
	CMD_ExecuteCommand("addChangeHandler Channel12 == 2 publish myChannel valueIsTwo", 0);
	// this will only happen every time when channel changes, and publish value multiplied by 10
	CMD_ExecuteCommand("addEventHandler OnChannelChange 12 publishInt twelveValue $CH12*10", 0);
	
	// set relay on 12 so we will get a publish
	CMD_ExecuteCommand("setPinRole 9 Rel", 0);
	CMD_ExecuteCommand("setPinChannel 9 12", 0);

	// This should trigger MQTT publish
	CMD_ExecuteCommand("setChannel 12 1", 0);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("handlerTester/12/get", "1", false);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("handlerTester/myChannel/get", "valueIsOne", false);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_FLOAT("handlerTester/twelveValue/get", 10, false);
	// if assert has passed, we can clear SIM MQTT history, it's no longer needed
	SIM_ClearMQTTHistory();

	// This should trigger MQTT publish
	CMD_ExecuteCommand("setChannel 12 2", 0);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("handlerTester/12/get", "2", false);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("handlerTester/myChannel/get", "valueIsTwo", false);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_FLOAT("handlerTester/twelveValue/get", 20, false);
	// if assert has passed, we can clear SIM MQTT history, it's no longer needed
	SIM_ClearMQTTHistory();

	// This should trigger MQTT publish
	CMD_ExecuteCommand("setChannel 12 0", 0);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("handlerTester/12/get", "0", false);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("handlerTester/myChannel/get", "valueIsZero", false);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_FLOAT("handlerTester/twelveValue/get", 0, false);
	// if assert has passed, we can clear SIM MQTT history, it's no longer needed
	SIM_ClearMQTTHistory();

	// This should trigger MQTT publish
	CMD_ExecuteCommand("setChannel 12 2", 0);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("handlerTester/12/get", "2", false);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_STR("handlerTester/myChannel/get", "valueIsTwo", false);
	SELFTEST_ASSERT_HAD_MQTT_PUBLISH_FLOAT("handlerTester/twelveValue/get", 20, false);
	// if assert has passed, we can clear SIM MQTT history, it's no longer needed
	SIM_ClearMQTTHistory();
}


#endif
