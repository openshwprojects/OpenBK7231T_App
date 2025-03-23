#ifdef WINDOWS

#include "selftest_local.h"

void Test_Role_ToggleAll_2() {
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

	PIN_SetPinRoleForPinIndex(5, IOR_Button_ToggleAll);
	PIN_SetPinChannelForPinIndex(5, 0);
	SIM_SetSimulatedPinValue(5, true);

	// nothing happens
	Sim_RunSeconds(1.5f, false);
	CMD_ExecuteCommand("setChannel 1 0", 0);
	CMD_ExecuteCommand("setChannel 2 0", 0);
	CMD_ExecuteCommand("setChannel 3 0", 0);
	CMD_ExecuteCommand("setChannel 4 0", 0);

	SELFTEST_ASSERT_CHANNEL(1, 0);
	SELFTEST_ASSERT_CHANNEL(2, 0);
	SELFTEST_ASSERT_CHANNEL(3, 0);
	SELFTEST_ASSERT_CHANNEL(4, 0);

	for (int i = 0; i < 10; i++) {
		printf("Tgl all test press %i test\n", i);
		Sim_RunMiliseconds(1500, false);
		SIM_SimulateUserClickOnPin(5);
		Sim_RunMiliseconds(1500, false);

		SELFTEST_ASSERT_CHANNEL(1, 1);
		SELFTEST_ASSERT_CHANNEL(2, 1);
		SELFTEST_ASSERT_CHANNEL(3, 1);
		SELFTEST_ASSERT_CHANNEL(4, 1);

		Sim_RunMiliseconds(1500, false);
		SIM_SimulateUserClickOnPin(5);
		Sim_RunMiliseconds(1500, false);

		SELFTEST_ASSERT_CHANNEL(1, 0);
		SELFTEST_ASSERT_CHANNEL(2, 0);
		SELFTEST_ASSERT_CHANNEL(3, 0);
		SELFTEST_ASSERT_CHANNEL(4, 0);
	}

	for (int i = 0; i < 10; i++) {
		printf("Tgl all test press v2 %i test\n", i);

		SELFTEST_ASSERT_CHANNEL(1, 0);
		SELFTEST_ASSERT_CHANNEL(2, 0);
		SELFTEST_ASSERT_CHANNEL(3, 0);
		SELFTEST_ASSERT_CHANNEL(4, 0);

		CMD_ExecuteCommand("setChannel 2 1", 0);

		SELFTEST_ASSERT_CHANNEL(1, 0);
		SELFTEST_ASSERT_CHANNEL(2, 1);
		SELFTEST_ASSERT_CHANNEL(3, 0);
		SELFTEST_ASSERT_CHANNEL(4, 0);

		Sim_RunMiliseconds(1500, false);
		SIM_SimulateUserClickOnPin(5);
		Sim_RunMiliseconds(1500, false);

		SELFTEST_ASSERT_CHANNEL(1, 0);
		SELFTEST_ASSERT_CHANNEL(2, 0);
		SELFTEST_ASSERT_CHANNEL(3, 0);
		SELFTEST_ASSERT_CHANNEL(4, 0);
	}

	// this check will fail obviously!
	//SELFTEST_ASSERT_CHANNEL(5, 666);
}


#endif
