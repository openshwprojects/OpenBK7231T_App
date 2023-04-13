#ifdef WINDOWS

#include "selftest_local.h"

void Test_Demo_SimpleShuttersScript() {
	// reset whole device
	SIM_ClearOBK(0);

	// Pins 3 and 4 are buttons
	PIN_SetPinRoleForPinIndex(3, IOR_Relay);
	PIN_SetPinChannelForPinIndex(3, 10);
	PIN_SetPinRoleForPinIndex(4, IOR_Relay);
	PIN_SetPinChannelForPinIndex(3, 11);

	Sim_RunMiliseconds(500, false);

	// change to OnClick if you want?
	CMD_ExecuteCommand("alias OpenShutter backlog clearRepeatingEvents; setChannel 11 0; setChannel 10 1; addRepeatingEvent 2 1 setChannel 10 0;", 0);
	CMD_ExecuteCommand("alias CloseShutter backlog clearRepeatingEvents; setChannel 10 0; setChannel 11 1; addRepeatingEvent 2 1 setChannel 11 0;", 0);

	CMD_ExecuteCommand("alias CloseShutterIfPossible if $activeRepeatingEvents==0 then CloseShutter", 0);
	CMD_ExecuteCommand("alias OpenShutterIfPossible if $activeRepeatingEvents==0 then OpenShutter", 0);

	SELFTEST_ASSERT_EXPRESSION("$activeRepeatingEvents", 0);
	SELFTEST_ASSERT_CHANNEL(11, 0);
	SELFTEST_ASSERT_CHANNEL(10, 0);
	CMD_ExecuteCommand("CloseShutterIfPossible",0);
	SELFTEST_ASSERT_EXPRESSION("$activeRepeatingEvents", 1);
	SELFTEST_ASSERT_CHANNEL(10, 0);
	SELFTEST_ASSERT_CHANNEL(11, 1);
	Sim_RunMiliseconds(500, false);
	SELFTEST_ASSERT_CHANNEL(10, 0);
	SELFTEST_ASSERT_CHANNEL(11, 1);
	Sim_RunMiliseconds(2500, false);
	SELFTEST_ASSERT_CHANNEL(11, 0);
	SELFTEST_ASSERT_CHANNEL(10, 0);
	SELFTEST_ASSERT_EXPRESSION("$activeRepeatingEvents", 0);
	Sim_RunMiliseconds(500, false);
	SELFTEST_ASSERT_EXPRESSION("$activeRepeatingEvents", 0);
	SELFTEST_ASSERT_CHANNEL(11, 0);
	SELFTEST_ASSERT_CHANNEL(10, 0);
	CMD_ExecuteCommand("OpenShutterIfPossible", 0);
	SELFTEST_ASSERT_EXPRESSION("$activeRepeatingEvents", 1);
	SELFTEST_ASSERT_CHANNEL(11, 0);
	SELFTEST_ASSERT_CHANNEL(10, 1);
	SELFTEST_ASSERT_EXPRESSION("$activeRepeatingEvents", 1);
	Sim_RunMiliseconds(500, false);
	SELFTEST_ASSERT_EXPRESSION("$activeRepeatingEvents", 1);
	SELFTEST_ASSERT_CHANNEL(11, 0);
	SELFTEST_ASSERT_CHANNEL(10, 1);
	SELFTEST_ASSERT_EXPRESSION("$activeRepeatingEvents", 1);
	Sim_RunMiliseconds(500, false);
	SELFTEST_ASSERT_CHANNEL(11, 0);
	SELFTEST_ASSERT_CHANNEL(10, 1);
	SELFTEST_ASSERT_EXPRESSION("$activeRepeatingEvents", 1);
	Sim_RunMiliseconds(2500, false);
	SELFTEST_ASSERT_CHANNEL(11, 0);
	SELFTEST_ASSERT_CHANNEL(10, 0);
	SELFTEST_ASSERT_EXPRESSION("$activeRepeatingEvents", 0);



}


#endif
