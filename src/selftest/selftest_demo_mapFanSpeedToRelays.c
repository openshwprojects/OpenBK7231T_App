#ifdef WINDOWS

#include "selftest_local.h"



void Test_Demo_MapFanSpeedToRelays() {
	// reset whole device
	SIM_ClearOBK(0);

	// not belongs here, but some quick expression tests
	CMD_ExecuteCommand("setChannel 0 123", 0);
	SELFTEST_ASSERT_EXPRESSION("$CH0", 123);
	CMD_ExecuteCommand("setChannel 0 234", 0);
	SELFTEST_ASSERT_EXPRESSION("$CH0", 234);
	CMD_ExecuteCommand("setChannel 0 0", 0);
	SELFTEST_ASSERT_EXPRESSION("$CH0", 0);

	// This demo uses following channels:
	// channel 0 - input value, 0 to 100 speed
	// channel 1 - internal usage, mapped to 0, 1, or 2 or 3
	// channels 10, 11, 12 - three relays, all off is fan off ,first is low speed, second mid, third full

	// Home Assistant only sets speed as an intneger 0% to 100%
	// and then OBK maps it to 3 relays
	CMD_ExecuteCommand("addEventHandler OnChannelChange 0 MapRanges 1 $CH0 0 33 66", 0);

	// 10, 11, and 12 are relays
	CMD_ExecuteCommand("addChangeHandler Channel1 == 0 backlog SetChannel 10 0; SetChannel 11 0; SetChannel 12 0", 0);
	CMD_ExecuteCommand("addChangeHandler Channel1 == 1 backlog SetChannel 10 1; SetChannel 11 0; SetChannel 12 0", 0);
	CMD_ExecuteCommand("addChangeHandler Channel1 == 2 backlog SetChannel 10 0; SetChannel 11 1; SetChannel 12 0", 0);
	CMD_ExecuteCommand("addChangeHandler Channel1 == 3 backlog SetChannel 10 0; SetChannel 11 0; SetChannel 12 1", 0);

	// setting channel 0 to 0 should set channel 1 to 0
	CMD_ExecuteCommand("setChannel 0 0", 0);
	SELFTEST_ASSERT_CHANNEL(0, 0);
	SELFTEST_ASSERT_CHANNEL(1, 0);

	SELFTEST_ASSERT_CHANNEL(10, 0);
	SELFTEST_ASSERT_CHANNEL(11, 0);
	SELFTEST_ASSERT_CHANNEL(12, 0);

	// setting channel 0 to 10 should set channel 1 to 1 to because 10 belongs to (0,33>
	CMD_ExecuteCommand("setChannel 0 10", 0);
	SELFTEST_ASSERT_CHANNEL(0, 10);
	SELFTEST_ASSERT_CHANNEL(1, 1);

	SELFTEST_ASSERT_CHANNEL(10, 1);
	SELFTEST_ASSERT_CHANNEL(11, 0);
	SELFTEST_ASSERT_CHANNEL(12, 0);

	// setting channel 0 to 34 should set channel 1 to 2 to because 34 belongs to (33,66>
	CMD_ExecuteCommand("setChannel 0 34", 0);
	SELFTEST_ASSERT_CHANNEL(0, 34);
	SELFTEST_ASSERT_CHANNEL(1, 2);

	SELFTEST_ASSERT_CHANNEL(10, 0);
	SELFTEST_ASSERT_CHANNEL(11, 1);
	SELFTEST_ASSERT_CHANNEL(12, 0);

	// setting channel 0 to 65 should set channel 1 to 2 to because 65 belongs to (33,66>
	CMD_ExecuteCommand("setChannel 0 65", 0);
	SELFTEST_ASSERT_CHANNEL(0, 65);
	SELFTEST_ASSERT_CHANNEL(1, 2);

	SELFTEST_ASSERT_CHANNEL(10, 0);
	SELFTEST_ASSERT_CHANNEL(11, 1);
	SELFTEST_ASSERT_CHANNEL(12, 0);

	// setting channel 0 to 99 should set channel 1 to 3 to because 99 belongs to (66,oo>
	CMD_ExecuteCommand("setChannel 0 99", 0);
	SELFTEST_ASSERT_CHANNEL(0, 99);
	SELFTEST_ASSERT_CHANNEL(1, 3);

	SELFTEST_ASSERT_CHANNEL(10, 0);
	SELFTEST_ASSERT_CHANNEL(11, 0);
	SELFTEST_ASSERT_CHANNEL(12, 1);

	// setting channel 0 to 10 should set channel 1 to 1 to because 10 belongs to (0,33>
	CMD_ExecuteCommand("setChannel 0 10", 0);
	SELFTEST_ASSERT_CHANNEL(0, 10);
	SELFTEST_ASSERT_CHANNEL(1, 1);

	SELFTEST_ASSERT_CHANNEL(10, 1);
	SELFTEST_ASSERT_CHANNEL(11, 0);
	SELFTEST_ASSERT_CHANNEL(12, 0);
}


#endif
