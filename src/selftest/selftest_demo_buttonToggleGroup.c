#ifdef WINDOWS

#include "selftest_local.h"

void Test_Demo_ButtonToggleGroup() {
	// reset whole device
	SIM_ClearOBK(0);

	PIN_SetPinRoleForPinIndex(1, IOR_Relay);
	PIN_SetPinChannelForPinIndex(1, 1);

	PIN_SetPinRoleForPinIndex(2, IOR_Relay);
	PIN_SetPinChannelForPinIndex(2, 2);

	PIN_SetPinRoleForPinIndex(3, IOR_Relay);
	PIN_SetPinChannelForPinIndex(3, 3);

	PIN_SetPinRoleForPinIndex(4, IOR_Relay);
	PIN_SetPinChannelForPinIndex(4, 4);

	CMD_ExecuteCommand("alias set_all_on backlog setChannel 1 1; setChannel 2 1; setChannel 3 1; setChannel 4 1;", 0);
	CMD_ExecuteCommand("alias set_all_off backlog setChannel 1 0; setChannel 2 0; setChannel 3 0; setChannel 4 0;", 0);
	CMD_ExecuteCommand("alias myToggle if $CH1||$CH2||$CH3||$CH4 then set_all_off else set_all_on", 0);
	CMD_ExecuteCommand("addEventHandler OnChannelChange 5 myToggle", 0);
	
	Sim_RunMiliseconds(500, false);


	CMD_ExecuteCommand("set_all_off", 0);
	// all 0
	SELFTEST_ASSERT_CHANNEL(1, 0);
	SELFTEST_ASSERT_CHANNEL(2, 0);
	SELFTEST_ASSERT_CHANNEL(3, 0);
	SELFTEST_ASSERT_CHANNEL(4, 0);

	CMD_ExecuteCommand("set_all_on", 0);
	// all 1
	SELFTEST_ASSERT_CHANNEL(1, 1);
	SELFTEST_ASSERT_CHANNEL(2, 1);
	SELFTEST_ASSERT_CHANNEL(3, 1);
	SELFTEST_ASSERT_CHANNEL(4, 1);

	// toggling 5 causes toggle all
	CMD_ExecuteCommand("toggleChannel 5", 0);
	// now all should be 0
	SELFTEST_ASSERT_CHANNEL(1, 0);
	SELFTEST_ASSERT_CHANNEL(2, 0);
	SELFTEST_ASSERT_CHANNEL(3, 0);
	SELFTEST_ASSERT_CHANNEL(4, 0);

	// toggling 5 causes toggle all
	CMD_ExecuteCommand("toggleChannel 5", 0);
	// now all should be 1
	SELFTEST_ASSERT_CHANNEL(1, 1);
	SELFTEST_ASSERT_CHANNEL(2, 1);
	SELFTEST_ASSERT_CHANNEL(3, 1);
	SELFTEST_ASSERT_CHANNEL(4, 1);

	CMD_ExecuteCommand("setChannel 3 0", 0);
	SELFTEST_ASSERT_CHANNEL(3, 0);

	// toggling 5 causes toggle all
	CMD_ExecuteCommand("toggleChannel 5", 0);
	// now all should be 0
	SELFTEST_ASSERT_CHANNEL(1, 0);
	SELFTEST_ASSERT_CHANNEL(2, 0);
	SELFTEST_ASSERT_CHANNEL(3, 0);
	SELFTEST_ASSERT_CHANNEL(4, 0);

	// toggling 5 causes toggle all
	CMD_ExecuteCommand("toggleChannel 5", 0);
	// now all should be 1
	SELFTEST_ASSERT_CHANNEL(1, 1);
	SELFTEST_ASSERT_CHANNEL(2, 1);
	SELFTEST_ASSERT_CHANNEL(3, 1);
	SELFTEST_ASSERT_CHANNEL(4, 1);

	// toggling 5 causes toggle all
	CMD_ExecuteCommand("toggleChannel 5", 0);
	// now all should be 0
	SELFTEST_ASSERT_CHANNEL(1, 0);
	SELFTEST_ASSERT_CHANNEL(2, 0);
	SELFTEST_ASSERT_CHANNEL(3, 0);
	SELFTEST_ASSERT_CHANNEL(4, 0);

	CMD_ExecuteCommand("setChannel 3 1", 0);
	SELFTEST_ASSERT_CHANNEL(3, 1);

	// toggling 5 causes toggle all
	CMD_ExecuteCommand("toggleChannel 5", 0);
	// now all should be 0
	SELFTEST_ASSERT_CHANNEL(1, 0);
	SELFTEST_ASSERT_CHANNEL(2, 0);
	SELFTEST_ASSERT_CHANNEL(3, 0);
	SELFTEST_ASSERT_CHANNEL(4, 0);

	CMD_ExecuteCommand("setChannel 2 1", 0);
	SELFTEST_ASSERT_CHANNEL(2, 1);

	// toggling 5 causes toggle all
	CMD_ExecuteCommand("toggleChannel 5", 0);
	// now all should be 0
	SELFTEST_ASSERT_CHANNEL(1, 0);
	SELFTEST_ASSERT_CHANNEL(2, 0);
	SELFTEST_ASSERT_CHANNEL(3, 0);
	SELFTEST_ASSERT_CHANNEL(4, 0);

	// toggling 5 causes toggle all
	CMD_ExecuteCommand("toggleChannel 5", 0);
	// now all should be 1
	SELFTEST_ASSERT_CHANNEL(1, 1);
	SELFTEST_ASSERT_CHANNEL(2, 1);
	SELFTEST_ASSERT_CHANNEL(3, 1);
	SELFTEST_ASSERT_CHANNEL(4, 1);

	CMD_ExecuteCommand("setChannel 3 0", 0);
	SELFTEST_ASSERT_CHANNEL(3, 0);
	CMD_ExecuteCommand("setChannel 2 0", 0);
	SELFTEST_ASSERT_CHANNEL(2, 0);
	CMD_ExecuteCommand("setChannel 4 0", 0);
	SELFTEST_ASSERT_CHANNEL(4, 0);
	CMD_ExecuteCommand("setChannel 1 0", 0);
	SELFTEST_ASSERT_CHANNEL(1, 0);

	// toggling 5 causes toggle all
	CMD_ExecuteCommand("toggleChannel 5", 0);
	// now all should be 1
	SELFTEST_ASSERT_CHANNEL(1, 1);
	SELFTEST_ASSERT_CHANNEL(2, 1);
	SELFTEST_ASSERT_CHANNEL(3, 1);
	SELFTEST_ASSERT_CHANNEL(4, 1);

	// force error
	//SELFTEST_ASSERT_CHANNEL(4, 123);
}


#endif
