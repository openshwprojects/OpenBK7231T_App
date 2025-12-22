#ifdef WINDOWS

#include "selftest_local.h"

void Test_Shutters() {

	// reset whole device
	SIM_ClearOBK(0);
	CMD_ExecuteCommand("lfs_format", 0);


	PIN_SetPinRoleForPinIndex(4, IOR_ShutterA);
	PIN_SetPinRoleForPinIndex(5, IOR_ShutterB);

	PIN_SetPinRoleForPinIndex(6, IOR_Button_ShutterUp);
	PIN_SetPinRoleForPinIndex(7, IOR_Button_ShutterDown);

	CMD_ExecuteCommand("startDriver Shutters", 0);
	SELFTEST_ASSERT_PIN_BOOLEAN(4, false);
	SELFTEST_ASSERT_PIN_BOOLEAN(5, false);
	CMD_ExecuteCommand("ShutterMove0 100", 0);
	Sim_RunSeconds(1.0f, false);
	//Sim_RunFrames(1, false);
	SELFTEST_ASSERT_PIN_BOOLEAN(4, true);
	SELFTEST_ASSERT_PIN_BOOLEAN(5, false);
	CMD_ExecuteCommand("ShutterMove0 Stop", 0);
	Sim_RunFrames(1, false);
	SELFTEST_ASSERT_PIN_BOOLEAN(4, false);
	SELFTEST_ASSERT_PIN_BOOLEAN(5, false);



}

#endif