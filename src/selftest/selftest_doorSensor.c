#ifdef WINDOWS

#include "selftest_local.h"
#include "../driver/drv_ntp.h"

int Simulator_GetNoChangeTimePassed();

void Test_DoorSensor() {
	// reset whole device
	SIM_ClearOBK(0);

	// must have mqtt
	SIM_ClearAndPrepareForMQTTTesting("miscDevice", "bekens");

	CMD_ExecuteCommand("startDriver DoorSensor", 0);
	CMD_ExecuteCommand("DSTime 100", 0);
	// start state
	CHANNEL_Set(1, 0, 0);

	PIN_SetPinRoleForPinIndex(9, IOR_DoorSensorWithDeepSleep);
	PIN_SetPinChannelForPinIndex(9, 1);

	for (int i = 0; i < 10; i++) {
		SELFTEST_ASSERT(Simulator_GetNoChangeTimePassed() == i);
		Main_OnEverySecond();
		SELFTEST_ASSERT(Simulator_GetNoChangeTimePassed() == (1+i));
	}
	// simulate change
	CHANNEL_Set(1, 1, 0);
	SELFTEST_ASSERT(Simulator_GetNoChangeTimePassed() == 0);
	int timeNoChange;
	for (timeNoChange = 0; timeNoChange < 10; timeNoChange++) {
		SELFTEST_ASSERT(Simulator_GetNoChangeTimePassed() == timeNoChange);
		Main_OnEverySecond();
		SELFTEST_ASSERT(Simulator_GetNoChangeTimePassed() == 1 + timeNoChange);
	}
	SELFTEST_ASSERT(Simulator_GetNoChangeTimePassed() == timeNoChange);
	CMD_ExecuteCommand("DSTime +5", 0);
	timeNoChange -= 5;
	SELFTEST_ASSERT(Simulator_GetNoChangeTimePassed() == timeNoChange);
	for (; timeNoChange < 20; timeNoChange++) {
		SELFTEST_ASSERT(Simulator_GetNoChangeTimePassed() == timeNoChange);
		Main_OnEverySecond();
		SELFTEST_ASSERT(Simulator_GetNoChangeTimePassed() == 1 + timeNoChange);
	}
	CMD_ExecuteCommand("DSTime clear", 0);
	SELFTEST_ASSERT(Simulator_GetNoChangeTimePassed() == 0);
	for (int i = 0; i < 8; i++) {
		SELFTEST_ASSERT(Simulator_GetNoChangeTimePassed() == i);
		Main_OnEverySecond();
		SELFTEST_ASSERT(Simulator_GetNoChangeTimePassed() == (1 + i));
	}
	CMD_ExecuteCommand("DSTime clear", 0);
	SELFTEST_ASSERT(Simulator_GetNoChangeTimePassed() == 0);

}

#endif
