#ifdef WINDOWS

#include "selftest_local.h"

void Test_ChargeLimitDriver() {
	// reset whole device
	SIM_ClearOBK(0);

	int start10 = 1000;
	CHANNEL_Set(10, start10, 0);
	CHANNEL_Set(12, 0, 0);
	CMD_ExecuteCommand("startDriver ChargingLimit", 0);
	// chSetupLimit 5 5000 3600 "POWER OFF"
	CMD_ExecuteCommand("alias MyEvent SetChannel 12 1", 0);
	CMD_ExecuteCommand("chSetupLimit 10 5 3600 MyEvent", 0);

	// simulate time passing
	for (int i = 0; i < 10; i++) {
		CMD_ExecuteCommand("addChannel 10 2", 0);
		Main_OnEverySecond();
		int val10 = CHANNEL_Get(10);
		int nowDelta = val10 - start10;
		printf("CH 12 is %i\n", CHANNEL_Get(12));
		if (nowDelta >= 5) {
			SELFTEST_ASSERT_CHANNEL(12,1);
		}
		else {
			SELFTEST_ASSERT_CHANNEL(12, 0);
		}
	}

	// simulate time passing - time limit 5 seconds
	CHANNEL_Set(12, 0, 0);
	CMD_ExecuteCommand("chSetupLimit 10 5 5 MyEvent", 0);
	for (int i = 0; i < 15; i++) {
		Main_OnEverySecond();
		// has 5 seconds passed?
		if (i >= 4) {
			SELFTEST_ASSERT_CHANNEL(12, 1);
		}
		else {
			SELFTEST_ASSERT_CHANNEL(12, 0);
		}
	}

}


#endif
