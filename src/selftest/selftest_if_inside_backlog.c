#ifdef WINDOWS

#include "selftest_local.h"

void Test_IF_Inside_Backlog() {
	// reset whole device
	SIM_ClearOBK(0);

	// CW bulb requires two PWMs - first on channel 1, second on channel 2
	// NOTE: we also support case where first PWM is on channel 0, and second on 1
	PIN_SetPinRoleForPinIndex(24, IOR_PWM);
	PIN_SetPinChannelForPinIndex(24, 1);

	PIN_SetPinRoleForPinIndex(26, IOR_PWM);
	PIN_SetPinChannelForPinIndex(26, 2);

	CMD_ExecuteCommand("led_enableAll 1", 0);

	CMD_ExecuteCommand("alias day_lights backlog led_temperature 200", 0);
	CMD_ExecuteCommand("alias night_lights backlog led_temperature 500", 0);
	// same can be done with if else
	CMD_ExecuteCommand("alias set_lights backlog if $CH10>7&&$CH10<19 then day_lights; if $CH10<=7||$CH10>=19 then night_lights;", 0);

	// simulate hour 10
	CMD_ExecuteCommand("setChannel 10 10", 0);
	CMD_ExecuteCommand("set_lights", 0);

	SELFTEST_ASSERT_EXPRESSION("$led_temperature", 200);


	// simulate hour 3
	CMD_ExecuteCommand("setChannel 10 3", 0);
	CMD_ExecuteCommand("set_lights", 0);

	SELFTEST_ASSERT_EXPRESSION("$led_temperature", 500);

	// simulate hour 12
	CMD_ExecuteCommand("setChannel 10 12", 0);
	CMD_ExecuteCommand("set_lights", 0);

	SELFTEST_ASSERT_EXPRESSION("$led_temperature", 200);

	// simulate hour 22
	CMD_ExecuteCommand("setChannel 10 22", 0);
	CMD_ExecuteCommand("set_lights", 0);

	SELFTEST_ASSERT_EXPRESSION("$led_temperature", 500);
}


#endif
