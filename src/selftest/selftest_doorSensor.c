#ifdef WINDOWS

#include "selftest_local.h"
#include "../driver/drv_ntp.h"

int Simulator_GetNoChangeTimePassed();
int Simulator_GetDoorSennsorAutomaticWakeUpAfterSleepTime();

const char *demo_noSleeper =
"again:\n"
"delay_s 0.25\n"
"if $CH1==1 then DSTime clear\n"
"goto again\n";

void Test_DoorSensor() {
	// reset whole device
	SIM_ClearOBK(0);

	// must have mqtt
	SIM_ClearAndPrepareForMQTTTesting("miscDevice", "bekens");

	CMD_ExecuteCommand("startDriver DoorSensor", 0);
	CMD_ExecuteCommand("DSTime 100", 0);
	SELFTEST_ASSERT(Simulator_GetDoorSennsorAutomaticWakeUpAfterSleepTime() == 0);
	CMD_ExecuteCommand("DSTime 100 1234", 0);
	SELFTEST_ASSERT(Simulator_GetDoorSennsorAutomaticWakeUpAfterSleepTime() == 1234);
	// no argument will not overwrite it
	CMD_ExecuteCommand("DSTime 200", 0);
	SELFTEST_ASSERT(Simulator_GetDoorSennsorAutomaticWakeUpAfterSleepTime() == 1234);
	CMD_ExecuteCommand("DSTime 100 345", 0);
	SELFTEST_ASSERT(Simulator_GetDoorSennsorAutomaticWakeUpAfterSleepTime() == 345);
	CMD_ExecuteCommand("DSTime 200 0", 0);
	SELFTEST_ASSERT(Simulator_GetDoorSennsorAutomaticWakeUpAfterSleepTime() == 0);


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

	CMD_ExecuteCommand("lfs_format", 0);

	// put file in LittleFS
	Test_FakeHTTPClientPacket_POST("api/lfs/demo_noSleeper.txt", demo_noSleeper);
	// get this file 
	Test_FakeHTTPClientPacket_GET("api/lfs/demo_noSleeper.txt");
	SELFTEST_ASSERT_HTML_REPLY(demo_noSleeper);

	CMD_ExecuteCommand("startScript demo_noSleeper.txt", 0);
	SELFTEST_ASSERT(Simulator_GetNoChangeTimePassed() == 0);
	for (int tr = 0; tr < 3; tr++) {
		for (int i = 0; i < 16; i++) {
			SELFTEST_ASSERT_INTEGER(CMD_GetCountActiveScriptThreads(), 1);
			SELFTEST_ASSERT(Simulator_GetNoChangeTimePassed() <= 1);
			Sim_RunSeconds(1.0f, 0);
			SELFTEST_ASSERT(Simulator_GetNoChangeTimePassed() <= 1);
		}
		// so script won't clear
		CHANNEL_Set(1, 0, 0);
		for (int i = 0; i < 16; i++) {
			Sim_RunSeconds(1.0f, 0);
		}
		SELFTEST_ASSERT(Simulator_GetNoChangeTimePassed() > 10);
		// so script will clear
		CHANNEL_Set(1, 1, 0);
		Sim_RunSeconds(1.0f, 0);
		Sim_RunSeconds(1.0f, 0);
		SELFTEST_ASSERT(Simulator_GetNoChangeTimePassed() <= 1);
	}
}

#endif
