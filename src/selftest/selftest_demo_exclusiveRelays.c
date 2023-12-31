#ifdef WINDOWS

#include "selftest_local.h"



void Test_Demo_ExclusiveRelays() {
	// reset whole device
	SIM_ClearOBK(0);
	CMD_ExecuteCommand("setChannel 0 0", 0);
	CMD_ExecuteCommand("setChannel 1 0", 0);
	CMD_ExecuteCommand("setChannel 2 0", 0);
	SELFTEST_ASSERT_CHANNEL(0, 0);
	SELFTEST_ASSERT_CHANNEL(1, 0);
	SELFTEST_ASSERT_CHANNEL(2, 0);
	CMD_ExecuteCommand("addChangeHandler Channel0 == 1 backlog setChannel 1 0; setChannel 2 0", 0);
	CMD_ExecuteCommand("addChangeHandler Channel1 == 1 backlog setChannel 0 0; setChannel 2 0", 0);
	CMD_ExecuteCommand("addChangeHandler Channel2 == 1 backlog setChannel 0 0; setChannel 1 0", 0);

	CMD_ExecuteCommand("setChannel 2 1", 0);
	SELFTEST_ASSERT_CHANNEL(0, 0);
	SELFTEST_ASSERT_CHANNEL(1, 0);
	SELFTEST_ASSERT_CHANNEL(2, 1);

	CMD_ExecuteCommand("setChannel 2 1", 0);
	SELFTEST_ASSERT_CHANNEL(0, 0);
	SELFTEST_ASSERT_CHANNEL(1, 0);
	SELFTEST_ASSERT_CHANNEL(2, 1);

	CMD_ExecuteCommand("setChannel 1 1", 0);
	SELFTEST_ASSERT_CHANNEL(0, 0);
	SELFTEST_ASSERT_CHANNEL(1, 1);
	SELFTEST_ASSERT_CHANNEL(2, 0);

	CMD_ExecuteCommand("setChannel 0 1", 0);
	SELFTEST_ASSERT_CHANNEL(0, 1);
	SELFTEST_ASSERT_CHANNEL(1, 0);
	SELFTEST_ASSERT_CHANNEL(2, 0);

	CMD_ExecuteCommand("toggleChannel 1", 0);
	SELFTEST_ASSERT_CHANNEL(0, 0);
	SELFTEST_ASSERT_CHANNEL(1, 1);
	SELFTEST_ASSERT_CHANNEL(2, 0);

	CMD_ExecuteCommand("toggleChannel 1", 0);
	SELFTEST_ASSERT_CHANNEL(0, 0);
	SELFTEST_ASSERT_CHANNEL(1, 0);
	SELFTEST_ASSERT_CHANNEL(2, 0);

	CMD_ExecuteCommand("setChannel 0 0", 0);
	SELFTEST_ASSERT_CHANNEL(0, 0);
	SELFTEST_ASSERT_CHANNEL(1, 0);
	SELFTEST_ASSERT_CHANNEL(2, 0);

	CMD_ExecuteCommand("setChannel 1 0", 0);
	SELFTEST_ASSERT_CHANNEL(0, 0);
	SELFTEST_ASSERT_CHANNEL(1, 0);
	SELFTEST_ASSERT_CHANNEL(2, 0);
}


#endif
