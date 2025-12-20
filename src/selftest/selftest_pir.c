#ifdef WINDOWS

#include "selftest_local.h"


void Test_PIR() {
	SIM_ClearOBK(0);
	CMD_ExecuteCommand("lfs_format", 0);

	PIN_SetPinRoleForPinIndex(6, IOR_DigitalInput);
	PIN_SetPinChannelForPinIndex(6, 11);

	PIN_SetPinRoleForPinIndex(8, IOR_PWM);
	PIN_SetPinChannelForPinIndex(8, 4);

	PIN_SetPinRoleForPinIndex(9, IOR_PWM_n);
	PIN_SetPinChannelForPinIndex(9, 5);

	PIN_SetPinRoleForPinIndex(23, IOR_ADC);
	PIN_SetPinChannelForPinIndex(23, 12);

	PIN_SetPinRoleForPinIndex(26, IOR_PWM_ScriptOnly);
	PIN_SetPinChannelForPinIndex(26, 10);

	CMD_ExecuteCommand("startDriver PIR", 0);

	Test_FakeHTTPClientPacket_GET("index?pirMode=1");
	// time on
	Test_FakeHTTPClientPacket_GET("index?pirTime=5");
	// margin light value - 2000
	Test_FakeHTTPClientPacket_GET("index?light=2000");
	// simulate light value - 3000
	SIM_SetIntegerValueADCPin(23, 3000);
	// no light
	SELFTEST_ASSERT(LED_GetEnableAll() == 0);
	for (int i = 0; i < 3; i++) {
		// Motion channel off
		SIM_SetSimulatedPinValue(6, false);
		Sim_RunSeconds(1, false);
		// no light
		SELFTEST_ASSERT(LED_GetEnableAll() == 0);
	}
	// Motion channel goes to 1
	SIM_SetSimulatedPinValue(6, true);
	// tick
	Sim_RunSeconds(2, false);
	// light is on 
	SELFTEST_ASSERT(LED_GetEnableAll() == 1);
	// there is still motion, so no timer
	for (int i = 0; i < 3; i++) {
		// Motion channel on
		SIM_SetSimulatedPinValue(6, true);
		Sim_RunSeconds(1, false);
		// light is on 
		SELFTEST_ASSERT(LED_GetEnableAll() == 1);
	}
	// now motion goes away
	SIM_SetSimulatedPinValue(6, false);
	// I set timer to 5 seconds, so 7?
	Sim_RunSeconds(7, false);
	SELFTEST_ASSERT(LED_GetEnableAll() == 0);
	// now nothing happens for long time
	for (int i = 0; i < 10; i++) {
		// Motion channel off
		SIM_SetSimulatedPinValue(6, false);
		Sim_RunSeconds(1, false);
		// no light
		SELFTEST_ASSERT(LED_GetEnableAll() == 0);
	}
	// Motion channel goes to 1
	SIM_SetSimulatedPinValue(6, true);
	// tick
	Sim_RunSeconds(2, false);
	// light is on 
	SELFTEST_ASSERT(LED_GetEnableAll() == 1);
	// but now it's day and it's bright
	SIM_SetIntegerValueADCPin(23, 500);
	// times goes down
	// I set timer to 5 seconds, so 7?
	Sim_RunSeconds(7, false);
	SELFTEST_ASSERT(LED_GetEnableAll() == 0);
	// so now, motion can be present, but light is still off
	for (int i = 0; i < 10; i++) {
		// Motion can be present 
		SIM_SetSimulatedPinValue(6, true);
		Sim_RunSeconds(1, false);
		// no light
		SELFTEST_ASSERT(LED_GetEnableAll() == 0);
	}
}


#endif
