#ifdef WINDOWS

#include "selftest_local.h"

void Test_Commands_Calendar() {
	// reset whole device
	SIM_ClearOBK(0);

	CMD_ExecuteCommand("startDriver NTP", 0);
	//CMD_ExecuteCommand("ntp_timeZoneOfs 0", 0);
	// set 2022, 06, 10, 11:27:34, Friday
	NTP_SetSimulatedTime(1654853254);

	SELFTEST_ASSERT_EXPRESSION("$minute",27);
	SELFTEST_ASSERT_EXPRESSION("$hour", 9);
	SELFTEST_ASSERT_EXPRESSION("$second", 34);
	SELFTEST_ASSERT_EXPRESSION("$day", 5);
	//SELFTEST_ASSERT_EXPRESSION("$month", 5);
	//SELFTEST_ASSERT_EXPRESSION("$year", 2022);

//	CMD_ExecuteCommand("setChannel 5 $year", 0);
//	SELFTEST_ASSERT_EXPRESSION("$CH5", 2022);

//	CMD_ExecuteCommand("setChannel 5 $month", 0);
//	SELFTEST_ASSERT_EXPRESSION("$CH5", 5);

	CMD_ExecuteCommand("setChannel 5 $second", 0);
	SELFTEST_ASSERT_EXPRESSION("$CH5", 34);

	CMD_ExecuteCommand("setChannel 5 $minute", 0);
	SELFTEST_ASSERT_EXPRESSION("$CH5", 27);

	CMD_ExecuteCommand("setChannel 5 $hour", 0);
	SELFTEST_ASSERT_EXPRESSION("$CH5", 9);

	CMD_ExecuteCommand("setChannel 5 $hour+$minute", 0);
	SELFTEST_ASSERT_EXPRESSION("$CH5", 9+27);

	SELFTEST_ASSERT_CHANNEL(1, 0);

	// every day
	CMD_ExecuteCommand("addClockEvent 9:28:00 0xff 123 addChannel 1 54", 0);
	// sunday
	CMD_ExecuteCommand("addClockEvent 9:28:30 0x01 234 addChannel 2 12", 0);
	// friday
	CMD_ExecuteCommand("addClockEvent 9:28:30 32 456 addChannel 3 312", 0);
	// friday
	CMD_ExecuteCommand("addClockEvent 9:29:00 32 654 addChannel 3 88", 0);
	Sim_RunSeconds(30, false);

	SELFTEST_ASSERT_CHANNEL(1, 54);
	Sim_RunSeconds(30, false);
	SELFTEST_ASSERT_CHANNEL(1, 54);
	SELFTEST_ASSERT_CHANNEL(2, 0);
	SELFTEST_ASSERT_CHANNEL(3, 312);
	Sim_RunSeconds(30, false);
	SELFTEST_ASSERT_CHANNEL(1, 54);
	SELFTEST_ASSERT_CHANNEL(2, 0);
	SELFTEST_ASSERT_CHANNEL(3, (312+88));




}


#endif
