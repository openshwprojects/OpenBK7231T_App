#ifdef WINDOWS

#include "selftest_local.h".

void Test_Commands_Channels() {
	// reset whole device
	CMD_ExecuteCommand("clearAll", 0);

	CMD_ExecuteCommand("setChannel 5 12", 0);

	SELFTEST_ASSERT_CHANNEL(4, 0);
	SELFTEST_ASSERT_CHANNEL(5, 12);
	SELFTEST_ASSERT_CHANNEL(6, 0);

	CMD_ExecuteCommand("addChannel 5 1", 0);
	SELFTEST_ASSERT_CHANNEL(5, 13);
	CMD_ExecuteCommand("addChannel 5 -1", 0);
	SELFTEST_ASSERT_CHANNEL(5, 12);

	// this check will fail obviously!
	//SELFTEST_ASSERT_CHANNEL(5, 666);

	CMD_ExecuteCommand("setChannel 15 100", 0);
	SELFTEST_ASSERT_CHANNEL(15, 100);

	// Add value of channel 15 to channel 5.
	// 5 is 12, so 12+100 = 112
	SELFTEST_ASSERT_CHANNEL(5, 12);
	SELFTEST_ASSERT_CHANNEL(15, 100);
	CMD_ExecuteCommand("addChannel 5 $CH15", 0);
	SELFTEST_ASSERT_CHANNEL(5, 112);
	CMD_ExecuteCommand("addChannel 5 $CH15", 0);
	SELFTEST_ASSERT_CHANNEL(5, 212);

	// system should be capable of handling - sign
	CMD_ExecuteCommand("addChannel 5 -$CH15", 0);
	SELFTEST_ASSERT_CHANNEL(5, 112);

	// It can also do simple math expressions
	CMD_ExecuteCommand("setChannel 2 10", 0);
	// $CH2 is now 10
	// 10 times 2 is 20
	// but we subtract here
	// So 112-20 = 92
	CMD_ExecuteCommand("addChannel 5 -$CH2*2", 0);
	SELFTEST_ASSERT_CHANNEL(5, 92);
	// this check will fail obviously!
	//SELFTEST_ASSERT_CHANNEL(5, 666);
}


#endif
