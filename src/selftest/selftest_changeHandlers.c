#ifdef WINDOWS

#include "selftest_local.h".

void Test_ChangeHandlers() {
	// reset whole device
	SIM_ClearOBK();
	SIM_ClearAndPrepareForMQTTTesting("handlerTester", "bekens");

	// this will only happens when Channel1 value changes from not equal to 0 to the one equal to 0
	CMD_ExecuteCommand("addChangeHandler Channel1 == 0 addChannel 10 1111", 0);
	SELFTEST_ASSERT_CHANNEL(1, 0);
	SELFTEST_ASSERT_CHANNEL(10, 0);
	CMD_ExecuteCommand("setChannel 1 0",0);
	SELFTEST_ASSERT_CHANNEL(1, 0);
	SELFTEST_ASSERT_CHANNEL(10, 0);
	CMD_ExecuteCommand("setChannel 1 1", 0);
	SELFTEST_ASSERT_CHANNEL(1, 1);
	SELFTEST_ASSERT_CHANNEL(10, 0);
	CMD_ExecuteCommand("setChannel 1 0", 0); // this will fire change handler first time
	SELFTEST_ASSERT_CHANNEL(1, 0);
	SELFTEST_ASSERT_CHANNEL(10, 1111);
	CMD_ExecuteCommand("setChannel 1 1", 0);
	SELFTEST_ASSERT_CHANNEL(1, 1);
	SELFTEST_ASSERT_CHANNEL(10, 1111);
	CMD_ExecuteCommand("setChannel 1 0", 0); // this will fire change handler second time
	SELFTEST_ASSERT_CHANNEL(1, 0);
	SELFTEST_ASSERT_CHANNEL(10, 2222);
	CMD_ExecuteCommand("toggleChannel 1", 0);
	SELFTEST_ASSERT_CHANNEL(1, 1);
	SELFTEST_ASSERT_CHANNEL(10, 2222);
	SELFTEST_ASSERT_CHANNEL(11, 0);
	CMD_ExecuteCommand("toggleChannel 1", 0); // this will fire change handler next time
	SELFTEST_ASSERT_CHANNEL(1, 0);
	SELFTEST_ASSERT_CHANNEL(10, 3333);
	SELFTEST_ASSERT_CHANNEL(11, 0);
	// let's make a change callback that will be fired from within a change callback
	CMD_ExecuteCommand("addChangeHandler Channel10 != 3333 addChannel 11 22", 0);
	SELFTEST_ASSERT_CHANNEL(1, 0);
	SELFTEST_ASSERT_CHANNEL(10, 3333);
	SELFTEST_ASSERT_CHANNEL(11, 0);
	CMD_ExecuteCommand("toggleChannel 1", 0);
	SELFTEST_ASSERT_CHANNEL(1, 1);
	SELFTEST_ASSERT_CHANNEL(10, 3333);
	SELFTEST_ASSERT_CHANNEL(11, 0);
	CMD_ExecuteCommand("toggleChannel 1", 0); // this will fire change handler next time
	SELFTEST_ASSERT_CHANNEL(1, 0);
	SELFTEST_ASSERT_CHANNEL(10, 4444);
	SELFTEST_ASSERT_CHANNEL(11, 22);

	SIM_ClearMQTTHistory();

}


#endif
