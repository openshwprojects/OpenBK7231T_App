#ifdef WINDOWS

#include "selftest_local.h"

void Test_Demo_ButtonScrollingChannelValues() {
	// reset whole device
	SIM_ClearOBK(0);

	// Pins 3 is button
	PIN_SetPinRoleForPinIndex(3, IOR_Button_ScriptOnly);
	SIM_SetSimulatedPinValue(3, true);

	// cycle press will cycle values 0, 1, 2 and 3 for channel 10
	// AddChannel [ChannelIndex][ValueToAdd][ClampMin][ClampMax][bWrapInsteadOfClamp]
	CMD_ExecuteCommand("alias Do_Cycle_Press addChannel 10 1 0 3 1", 0);

	CMD_ExecuteCommand("addEventHandler OnPress 3 Do_Cycle_Press", 0);

	Sim_RunMiliseconds(500, false);


	// start with 0
	CMD_ExecuteCommand("SetChannel 10 0", 0);

	SELFTEST_ASSERT_CHANNEL(10, 0); // Fan state - 0 to 4 values

	// user clicks NEXT SPEED
	SIM_SimulateUserClickOnPin(3);
	// Now lowest speed is set
	SELFTEST_ASSERT_CHANNEL(10, 1); // Fan state - 0 to 4 values

	// user clicks NEXT SPEED
	SIM_SimulateUserClickOnPin(3);
	// Now lowest speed is set
	SELFTEST_ASSERT_CHANNEL(10, 2); // Fan state - 0 to 4 values

	// user clicks NEXT SPEED
	SIM_SimulateUserClickOnPin(3);
	// Now lowest speed is set
	SELFTEST_ASSERT_CHANNEL(10, 3); // Fan state - 0 to 4 values

	// user clicks NEXT SPEED
	SIM_SimulateUserClickOnPin(3);
	// Now lowest speed is set
	SELFTEST_ASSERT_CHANNEL(10, 0); // Fan state - 0 to 4 values

	// user clicks NEXT SPEED
	SIM_SimulateUserClickOnPin(3);
	// Now lowest speed is set
	SELFTEST_ASSERT_CHANNEL(10, 1); // Fan state - 0 to 4 values
}


#endif
