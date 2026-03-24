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
	// Run 1 second so DRV_Shutters_RunEverySecond fires and registers the shutter struct.
	// The struct is lazily created the first time RunEverySecond scans the pins.
	Sim_RunSeconds(1.0f, false);
	// Initially both pins should be off (shutter is stopped at start)
	SELFTEST_ASSERT_PIN_BOOLEAN(4, false);
	SELFTEST_ASSERT_PIN_BOOLEAN(5, false);

	// Issue an open command (100% = fully open = SHUTTER_OPENING direction)
	CMD_ExecuteCommand("ShutterMove0 100", 0);
	// After issuing the open command: ShutterA=1 (opening), ShutterB=0
	SELFTEST_ASSERT_PIN_BOOLEAN(4, true);
	SELFTEST_ASSERT_PIN_BOOLEAN(5, false);

	Sim_RunSeconds(1.0f, false);
	// Still moving after 1 second (default open time is 10s)
	SELFTEST_ASSERT_PIN_BOOLEAN(4, true);
	SELFTEST_ASSERT_PIN_BOOLEAN(5, false);

	// Stop the shutter
	CMD_ExecuteCommand("ShutterMove0 Stop", 0);
	Sim_RunFrames(1, false);
	// After Stop: both pins must be off
	SELFTEST_ASSERT_PIN_BOOLEAN(4, false);
	SELFTEST_ASSERT_PIN_BOOLEAN(5, false);



}

#endif