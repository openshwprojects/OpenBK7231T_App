#ifdef WINDOWS

#include "selftest_local.h".

void Test_LEDDriver_CW() {
	// reset whole device
	CMD_ExecuteCommand("clearAll", 0);

	// CW bulb requires two PWMs - first on channel 1, second on channel 2
	// NOTE: we also support case where first PWM is on channel 0, and second on 1
	PIN_SetPinRoleForPinIndex(24, IOR_PWM);
	PIN_SetPinChannelForPinIndex(24, 1);

	PIN_SetPinRoleForPinIndex(26, IOR_PWM);
	PIN_SetPinChannelForPinIndex(26, 2);

	CMD_ExecuteCommand("led_enableAll 1", 0);
	// Set 100% Warm
	CMD_ExecuteCommand("led_temperature 500", 0);

	printf("Channel C is %i, channel W is %i\n", CHANNEL_Get(1), CHANNEL_Get(2));
	SELFTEST_ASSERT_CHANNEL(1, 0);
	SELFTEST_ASSERT_CHANNEL(2, 100);

	// Set 100% Cold
	CMD_ExecuteCommand("led_temperature 0", 0);

	SELFTEST_ASSERT_CHANNEL(1, 100);
	SELFTEST_ASSERT_CHANNEL(2, 0);


}

void Test_LEDDriver() {

	Test_LEDDriver_CW();
}

#endif
