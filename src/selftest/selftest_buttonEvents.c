#ifdef WINDOWS

#include "selftest_local.h"

void Test_ButtonEvents() {
	// reset whole device
	SIM_ClearOBK(0);

	// by default, we have a pull up resistor - so high level
	SIM_SetSimulatedPinValue(9, true);
	PIN_SetPinRoleForPinIndex(9, IOR_Button);

	//
	// Test OnPress and OnRelease and OnClick
	//
	// addEventHandler [EventName] [PinIndex] [Command]
	CMD_ExecuteCommand("addEventHandler OnPress 9 addChannel 10 345", 0);
	CMD_ExecuteCommand("addEventHandler OnRelease 9 addChannel 11 123", 0);
	CMD_ExecuteCommand("addEventHandler OnClick 9 addChannel 12 22", 0);
	CMD_ExecuteCommand("addEventHandler OnDblClick 9 addChannel 13 1201", 0);
	SELFTEST_ASSERT_CHANNEL(10, 0);
	SELFTEST_ASSERT_CHANNEL(11, 0);
	SELFTEST_ASSERT_CHANNEL(12, 0);
	SELFTEST_ASSERT_CHANNEL(13, 0);
	// NOTE: respect BTN_DEBOUNCE_TICKS
	Sim_RunFrames(15, false);
	// user presses the button and shorts it to ground
	SIM_SetSimulatedPinValue(9, false);
	Sim_RunFrames(15, false);
	SELFTEST_ASSERT_CHANNEL(10, 345);
	SELFTEST_ASSERT_CHANNEL(11, 0);
	SELFTEST_ASSERT_CHANNEL(12, 0);
	SELFTEST_ASSERT_CHANNEL(13, 0);
	Sim_RunFrames(15, false);
	SIM_SetSimulatedPinValue(9, true);
	Sim_RunFrames(15, false);
	SELFTEST_ASSERT_CHANNEL(10, 345);
	SELFTEST_ASSERT_CHANNEL(11, 123);
	SELFTEST_ASSERT_CHANNEL(12, 0);
	SELFTEST_ASSERT_CHANNEL(13, 0);
	// wait extra frames for the click event to fire
	Sim_RunFrames(100, false);
	// the click happens with a certain delay after "onRelease" because it waits
	// too see if it's a click or a double click
	SELFTEST_ASSERT_CHANNEL(10, 345);
	SELFTEST_ASSERT_CHANNEL(11, 123);
	SELFTEST_ASSERT_CHANNEL(12, 22);
	SELFTEST_ASSERT_CHANNEL(13, 0);

	//
	// Test OnPress and OnRelease and OnDblClick
	//
	Sim_RunFrames(1000, false);
	Sim_RunFrames(1000, false);
	SELFTEST_ASSERT_CHANNEL(10, 345);
	SELFTEST_ASSERT_CHANNEL(11, 123);
	SELFTEST_ASSERT_CHANNEL(12, 22);
	SELFTEST_ASSERT_CHANNEL(13, 0);
	// user presses the button and shorts it to ground
	SIM_SetSimulatedPinValue(9, false);
	Sim_RunFrames(15, false);
	SELFTEST_ASSERT_CHANNEL(10, (345 + 345));
	SELFTEST_ASSERT_CHANNEL(11, 123);
	SELFTEST_ASSERT_CHANNEL(12, 22);
	SELFTEST_ASSERT_CHANNEL(13, 0);
	// user releases it
	SIM_SetSimulatedPinValue(9, true);
	Sim_RunFrames(15, false);
	SELFTEST_ASSERT_CHANNEL(10, (345 + 345));
	SELFTEST_ASSERT_CHANNEL(11, (123 + 123));
	SELFTEST_ASSERT_CHANNEL(12, 22);
	SELFTEST_ASSERT_CHANNEL(13, 0);
	// within a short time, the click is repeated
	// but the Click event will not get called, only DblClick
	Sim_RunFrames(15, false);
	//  SECOND TIME user presses the button and shorts it to ground
	SIM_SetSimulatedPinValue(9, false);
	Sim_RunFrames(15, false);
	// user releases it
	SIM_SetSimulatedPinValue(9, true);
	Sim_RunFrames(100, false);
	// Double Click 
	// Double click will happen now:
	SELFTEST_ASSERT_CHANNEL(10, (345 + 345));
	SELFTEST_ASSERT_CHANNEL(11, (123 + 123 + 123));
	SELFTEST_ASSERT_CHANNEL(12, 22);
	SELFTEST_ASSERT_CHANNEL(13, 1201);
	Sim_RunFrames(15, false);
	SELFTEST_ASSERT_CHANNEL(10, (345 + 345));
	SELFTEST_ASSERT_CHANNEL(11, (123 + 123 + 123));
	SELFTEST_ASSERT_CHANNEL(12, 22);
	SELFTEST_ASSERT_CHANNEL(13, 1201);
	Sim_RunFrames(100, false);
	SELFTEST_ASSERT_CHANNEL(10, (345 + 345));
	SELFTEST_ASSERT_CHANNEL(11, (123 + 123 + 123));
	SELFTEST_ASSERT_CHANNEL(12, 22);
	SELFTEST_ASSERT_CHANNEL(13, 1201);
}


#endif
