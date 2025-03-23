#ifdef WINDOWS

#include "selftest_local.h"

void Test_MapRanges() {
	// reset whole device
	SIM_ClearOBK(0);

	CMD_ExecuteCommand("MapRanges 0 3.14 1 2 3 4 5 6", 0);
	// 3.14 is between 3 and 4, so we will get 3
	SELFTEST_ASSERT_CHANNEL(0, 3);

	CMD_ExecuteCommand("MapRanges 0 2.14 1 2 3 4 5 6", 0);
	// 2.14 is between 2 and 3, so we will get 2
	SELFTEST_ASSERT_CHANNEL(0, 2);

	CMD_ExecuteCommand("MapRanges 0 1.14 1 2 3 4 5 6", 0);
	// 1.14 is between 1 and 2, so we will get 1
	SELFTEST_ASSERT_CHANNEL(0, 1);

	CMD_ExecuteCommand("MapRanges 0 0.14 1 2 3 4 5 6", 0);
	// 0.14 is before 1, so we will get 0
	SELFTEST_ASSERT_CHANNEL(0, 0);

	CMD_ExecuteCommand("MapRanges 0 4.14 1 2 3 4 5 6", 0);
	// 4.14 is between 4 and 5, so we will get 4
	SELFTEST_ASSERT_CHANNEL(0, 4);

	CMD_ExecuteCommand("MapRanges 0 5.14 1 2 3 4 5 6", 0);
	// 5.14 is between 5 and 6, so we will get 5
	SELFTEST_ASSERT_CHANNEL(0, 5);

	CMD_ExecuteCommand("MapRanges 0 6.14 1 2 3 4 5 6", 0);
	// 6.14 is between 6 and 7, so we will get 6
	SELFTEST_ASSERT_CHANNEL(0, 6);

	// Fan test
	/*	The very best is to set all as a FAN device,
	were a speed of 0 percentage correspond to OFF - OFF - OFF,
	a speed 1 - 33 to ON - OFF - OFF, a seed of 34 - 77 to OFF - ON - OFF
	and a speed between 78 - 100 to OFF - OFF - ON...
	*/
	CMD_ExecuteCommand("MapRanges 0 0 0 33 66", 0);
	SELFTEST_ASSERT_CHANNEL(0, 0);

	CMD_ExecuteCommand("MapRanges 0 1 0 33 66", 0);
	SELFTEST_ASSERT_CHANNEL(0, 1);

	CMD_ExecuteCommand("MapRanges 0 32 0 33 66", 0);
	SELFTEST_ASSERT_CHANNEL(0, 1);

	CMD_ExecuteCommand("MapRanges 0 34 0 33 66", 0);
	SELFTEST_ASSERT_CHANNEL(0, 2);

	CMD_ExecuteCommand("MapRanges 0 65 0 33 66", 0);
	SELFTEST_ASSERT_CHANNEL(0, 2);

	CMD_ExecuteCommand("MapRanges 0 67 0 33 66", 0);
	SELFTEST_ASSERT_CHANNEL(0, 3);
}

#endif
