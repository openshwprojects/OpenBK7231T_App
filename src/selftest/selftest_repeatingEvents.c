#ifdef WINDOWS

#include "selftest_local.h"

void Test_RepeatingEvents() {
	// reset whole device
	SIM_ClearOBK(0);
	SELFTEST_ASSERT_CHANNEL(10, 0);
	// NOTE: addRepeatingEvent [RepeatTime] [RepeatCount]
	// This should fire once due to RepeatCount 1
	CMD_ExecuteCommand("addRepeatingEvent 2 1 addChannel 10 1", 0);
	Sim_RunSeconds(4.0f, false);
	SELFTEST_ASSERT_CHANNEL(10, 1);
	Sim_RunSeconds(6.0f, false);
	SELFTEST_ASSERT_CHANNEL(10, 1);
	// NOTE: addRepeatingEvent [RepeatTime] [RepeatCount]
	// This should fire twice due to RepeatCount 2
	CMD_ExecuteCommand("addRepeatingEvent 2 2 addChannel 11 1", 0);
	Sim_RunSeconds(6.0f, false);
	SELFTEST_ASSERT_CHANNEL(11, 2);
	Sim_RunSeconds(6.0f, false);
	SELFTEST_ASSERT_CHANNEL(11, 2);
}


#endif
