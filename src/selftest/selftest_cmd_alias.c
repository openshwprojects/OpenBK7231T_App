#ifdef WINDOWS

#include "selftest_local.h".

void Test_Commands_Alias() {
	// reset whole device
	SIM_ClearOBK(0);

	CMD_ExecuteCommand("alias test1 addChannel 2 10", 0);
	SELFTEST_ASSERT_CHANNEL(2, 0);
	CMD_ExecuteCommand("test1", 0);
	SELFTEST_ASSERT_CHANNEL(2, 10);
	CMD_ExecuteCommand("test1", 0);
	SELFTEST_ASSERT_CHANNEL(2, 20);
	CMD_ExecuteCommand("test1", 0);
	SELFTEST_ASSERT_CHANNEL(2, 30);

	CMD_ExecuteCommand("alias test2 addChannel 3 15", 0);
	CMD_ExecuteCommand("test2", 0);
	SELFTEST_ASSERT_CHANNEL(3, 15);

	// recursive alias...
	CMD_ExecuteCommand("alias test3 if $CH10 then test1 else test2", 0);
	CMD_ExecuteCommand("setChannel 10 1", 0);
	// this should run test1 - add 10 to CH2
	SELFTEST_ASSERT_CHANNEL(2, 30);
	CMD_ExecuteCommand("test3", 0);
	SELFTEST_ASSERT_CHANNEL(2, 40);
	CMD_ExecuteCommand("test3", 0);
	SELFTEST_ASSERT_CHANNEL(2, 50);
	CMD_ExecuteCommand("setChannel 10 0", 0);
	SELFTEST_ASSERT_CHANNEL(3, 15);
	// this should run test2 - add 15 to CH3
	CMD_ExecuteCommand("test3", 0);
	SELFTEST_ASSERT_CHANNEL(3, 30);
	SELFTEST_ASSERT_CHANNEL(2, 50);
	// this should run test2 - add 15 to CH3
	CMD_ExecuteCommand("test3", 0);
	SELFTEST_ASSERT_CHANNEL(3, 45);
	SELFTEST_ASSERT_CHANNEL(2, 50);
	// this should run test2 - add 15 to CH3
	CMD_ExecuteCommand("test3", 0);
	SELFTEST_ASSERT_CHANNEL(3, 60);
	SELFTEST_ASSERT_CHANNEL(2, 50);

	CMD_ExecuteCommand("alias test4 backlog setChannel 4 11; addChannel 4 9; setChannel 5 50", 0);
	CMD_ExecuteCommand("test4", 0);
	SELFTEST_ASSERT_CHANNEL(4, (11+9));
	SELFTEST_ASSERT_CHANNEL(5, 50);

	// this check will fail obviously!
	//SELFTEST_ASSERT_CHANNEL(5, 666);
}


#endif
