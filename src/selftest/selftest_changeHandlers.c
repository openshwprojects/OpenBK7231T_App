#ifdef WINDOWS

#include "selftest_local.h"

void Test_ChangeHandlers() {
	// reset whole device
	SIM_ClearOBK(0);
	SIM_ClearAndPrepareForMQTTTesting("handlerTester", "bekens");

	// this will only happens when Channel1 value changes from not equal to 0 to the one equal to 0
	CMD_ExecuteCommand("addChangeHandler Channel1 == 0 addChannel 10 1111", 0);
	SELFTEST_ASSERT_CHANNEL(1, 0);
	SELFTEST_ASSERT_CHANNEL(10, 0);
	CMD_ExecuteCommand("setChannel 1 0", 0);
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

	SELFTEST_ASSERT_CHANNEL(7, 0);
	SELFTEST_ASSERT_CHANNEL(8, 0);
	// when channel 7 value changes, add one to channel 8
	CMD_ExecuteCommand("addEventHandler OnChannelChange 7 addChannel 8 1", 0);
	SELFTEST_ASSERT_CHANNEL(7, 0);
	SELFTEST_ASSERT_CHANNEL(8, 0);
	CMD_ExecuteCommand("toggleChannel 7", 0);
	SELFTEST_ASSERT_CHANNEL(7, 1);
	SELFTEST_ASSERT_CHANNEL(8, 1);
	CMD_ExecuteCommand("toggleChannel 7", 0);
	SELFTEST_ASSERT_CHANNEL(7, 0);
	SELFTEST_ASSERT_CHANNEL(8, 2);
	CMD_ExecuteCommand("toggleChannel 7", 0);
	SELFTEST_ASSERT_CHANNEL(7, 1);
	SELFTEST_ASSERT_CHANNEL(8, 3);
	CMD_ExecuteCommand("setChannel 7 1", 0);
	// there was no change!
	SELFTEST_ASSERT_CHANNEL(7, 1);
	SELFTEST_ASSERT_CHANNEL(8, 3);
	CMD_ExecuteCommand("setChannel 7 1", 0);
	// there was no change!
	SELFTEST_ASSERT_CHANNEL(7, 1);
	SELFTEST_ASSERT_CHANNEL(8, 3);
	CMD_ExecuteCommand("setChannel 7 1", 0);
	// there was no change!
	SELFTEST_ASSERT_CHANNEL(7, 1);
	SELFTEST_ASSERT_CHANNEL(8, 3);
	CMD_ExecuteCommand("setChannel 7 0", 0);
	// changed value.
	SELFTEST_ASSERT_CHANNEL(7, 0);
	SELFTEST_ASSERT_CHANNEL(8, 4);
	CMD_ExecuteCommand("setChannel 7 1", 0);
	// changed value.
	SELFTEST_ASSERT_CHANNEL(7, 1);
	SELFTEST_ASSERT_CHANNEL(8, 5);
	CMD_ExecuteCommand("setChannel 7 2", 0);
	// changed value.
	SELFTEST_ASSERT_CHANNEL(7, 2);
	SELFTEST_ASSERT_CHANNEL(8, 6);
	CMD_ExecuteCommand("setChannel 7 3", 0);
	// changed value.
	SELFTEST_ASSERT_CHANNEL(7, 3);
	SELFTEST_ASSERT_CHANNEL(8, 7);
	CMD_ExecuteCommand("setChannel 7 3", 0);
	// there was no change!
	SELFTEST_ASSERT_CHANNEL(7, 3);
	SELFTEST_ASSERT_CHANNEL(8, 7);
	CMD_ExecuteCommand("setChannel 7 2", 0);
	// changed value.
	SELFTEST_ASSERT_CHANNEL(7, 2);
	SELFTEST_ASSERT_CHANNEL(8, 8);

}

void Test_ChangeHandlers_EnsureThatChannelVariableIsExpandedAtHandlerRunTime() {
	// reset whole device
	SIM_ClearOBK(0);
	SIM_ClearAndPrepareForMQTTTesting("handlerTester", "bekens");

	// this will only happens when Channel1 value changes from not equal to 0 to the one equal to 0
	CMD_ExecuteCommand("setChannel 15 2+2", 0);
	CMD_ExecuteCommand("setChannel 1 0", 0);
	CMD_ExecuteCommand("addChangeHandler Channel1 == 0 addChannel 14 $CH15", 0);
	SELFTEST_ASSERT_CHANNEL(1, 0);
	SELFTEST_ASSERT_CHANNEL(14, 0);
	SELFTEST_ASSERT_CHANNEL(15, 2+2);
	CMD_ExecuteCommand("setChannel 1 1", 0);
	SELFTEST_ASSERT_CHANNEL(1, 1);
	SELFTEST_ASSERT_CHANNEL(14, 0);
	SELFTEST_ASSERT_CHANNEL(15, 2 + 2);
	CMD_ExecuteCommand("setChannel 1 0", 0);
	SELFTEST_ASSERT_CHANNEL(1, 0);
	SELFTEST_ASSERT_CHANNEL(14, (2+2));
	SELFTEST_ASSERT_CHANNEL(15, 2 + 2);
	CMD_ExecuteCommand("setChannel 1 1", 0);
	SELFTEST_ASSERT_CHANNEL(1, 1);
	SELFTEST_ASSERT_CHANNEL(14, (2 + 2));
	SELFTEST_ASSERT_CHANNEL(15, 2 + 2);
	CMD_ExecuteCommand("setChannel 1 0", 0);
	SELFTEST_ASSERT_CHANNEL(1, 0);
	SELFTEST_ASSERT_CHANNEL(14, 2*(2 + 2));
	SELFTEST_ASSERT_CHANNEL(15, 2 + 2);
	// now, 15 changes to 123 and handler will add 123
	CMD_ExecuteCommand("setChannel 15 100+20+3", 0);
	SELFTEST_ASSERT_CHANNEL(15, 123);
	CMD_ExecuteCommand("setChannel 1 1", 0);
	SELFTEST_ASSERT_CHANNEL(1, 1);
	SELFTEST_ASSERT_CHANNEL(14, 2 * (2 + 2));
	SELFTEST_ASSERT_CHANNEL(15, 123);
	CMD_ExecuteCommand("setChannel 1 0", 0);
	SELFTEST_ASSERT_CHANNEL(1, 0);
	SELFTEST_ASSERT_CHANNEL(14, (2 * (2 + 2))+123);
	SELFTEST_ASSERT_CHANNEL(15, 123);
	CMD_ExecuteCommand("setChannel 1 1", 0);
	SELFTEST_ASSERT_CHANNEL(1, 1);
	SELFTEST_ASSERT_CHANNEL(14, (2 * (2 + 2)) + 123);
	SELFTEST_ASSERT_CHANNEL(15, 123);
	CMD_ExecuteCommand("setChannel 1 0", 0);
	SELFTEST_ASSERT_CHANNEL(1, 0);
	SELFTEST_ASSERT_CHANNEL(14, (2 * (2 + 2))+2*123);
	SELFTEST_ASSERT_CHANNEL(15, 123);
	CMD_ExecuteCommand("setChannel 1 1", 0);
	SELFTEST_ASSERT_CHANNEL(1, 1);
	SELFTEST_ASSERT_CHANNEL(14, (2 * (2 + 2)) + 2 * 123);
	SELFTEST_ASSERT_CHANNEL(15, 123);
	CMD_ExecuteCommand("setChannel 1 0", 0);
	SELFTEST_ASSERT_CHANNEL(1, 0);
	SELFTEST_ASSERT_CHANNEL(14, (2 * (2 + 2)) + 3 * 123);
	SELFTEST_ASSERT_CHANNEL(15, 123);

	CMD_ExecuteCommand("setChannel 20 999", 0);
	CMD_ExecuteCommand("addChangeHandler Channel20 == 0 setChannel 21 1", 0);
	CMD_ExecuteCommand("addChangeHandler Channel20 != 0 setChannel 21 0", 0);
	CMD_ExecuteCommand("setChannel 20 0", 0);
	SELFTEST_ASSERT_CHANNEL(20, 0);
	SELFTEST_ASSERT_CHANNEL(21, 1);
	CMD_ExecuteCommand("setChannel 20 1", 0);
	SELFTEST_ASSERT_CHANNEL(20, 1);
	SELFTEST_ASSERT_CHANNEL(21, 0);
	CMD_ExecuteCommand("setChannel 20 0", 0);
	SELFTEST_ASSERT_CHANNEL(20, 0);
	SELFTEST_ASSERT_CHANNEL(21, 1)
		CMD_ExecuteCommand("setChannel 20 1", 0);
	SELFTEST_ASSERT_CHANNEL(20, 1);
	SELFTEST_ASSERT_CHANNEL(21, 0);

	SIM_ClearMQTTHistory();

}


#endif
