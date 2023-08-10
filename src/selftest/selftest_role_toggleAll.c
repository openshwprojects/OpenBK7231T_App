#ifdef WINDOWS

#include "selftest_local.h"

void Test_Role_ToggleAll() {
	// reset whole device
	SIM_ClearOBK(0);

	PIN_SetPinRoleForPinIndex(10, IOR_Relay);
	PIN_SetPinChannelForPinIndex(10, 0);

	PIN_SetPinRoleForPinIndex(11, IOR_Relay);
	PIN_SetPinChannelForPinIndex(11, 1);

	PIN_SetPinRoleForPinIndex(12, IOR_Relay);
	PIN_SetPinChannelForPinIndex(12, 2);

	PIN_SetPinRoleForPinIndex(13, IOR_Relay);
	PIN_SetPinChannelForPinIndex(13, 3);

	PIN_SetPinRoleForPinIndex(14, IOR_Button_ToggleAll);

	SELFTEST_ASSERT_CHANNEL(0, 0);
	SELFTEST_ASSERT_CHANNEL(1, 0);
	SELFTEST_ASSERT_CHANNEL(2, 0);
	SELFTEST_ASSERT_CHANNEL(3, 0);

	CMD_ExecuteCommand("setChannel 2 1", 0);

	SELFTEST_ASSERT_CHANNEL(0, 0);
	SELFTEST_ASSERT_CHANNEL(1, 0);
	SELFTEST_ASSERT_CHANNEL(2, 1);
	SELFTEST_ASSERT_CHANNEL(3, 0);


	// user clicks button
	SIM_SimulateUserClickOnPin(14);

	// toggle all should disable all channels
	SELFTEST_ASSERT_CHANNEL(0, 0);
	SELFTEST_ASSERT_CHANNEL(1, 0);
	SELFTEST_ASSERT_CHANNEL(2, 0);
	SELFTEST_ASSERT_CHANNEL(3, 0);

	// nothing happens
	Sim_RunSeconds(1.5f, false);

	// user clicks button
	SIM_SimulateUserClickOnPin(14);

	// toggle all should enable all channels
	SELFTEST_ASSERT_CHANNEL(0, 1);
	SELFTEST_ASSERT_CHANNEL(1, 1);
	SELFTEST_ASSERT_CHANNEL(2, 1);
	SELFTEST_ASSERT_CHANNEL(3, 1);

	// nothing happens
	Sim_RunSeconds(1.5f, false);

	// user clicks button
	SIM_SimulateUserClickOnPin(14);

	// toggle all should disable all channels
	SELFTEST_ASSERT_CHANNEL(0, 0);
	SELFTEST_ASSERT_CHANNEL(1, 0);
	SELFTEST_ASSERT_CHANNEL(2, 0);
	SELFTEST_ASSERT_CHANNEL(3, 0);

	// user enables one channel
	CMD_ExecuteCommand("setChannel 0 1", 0);

	// nothing happens
	Sim_RunSeconds(1.5f, false);

	// user clicks button
	SIM_SimulateUserClickOnPin(14);

	// toggle all should disable all channels
	SELFTEST_ASSERT_CHANNEL(0, 0);
	SELFTEST_ASSERT_CHANNEL(1, 0);
	SELFTEST_ASSERT_CHANNEL(2, 0);
	SELFTEST_ASSERT_CHANNEL(3, 0);
}


#endif
