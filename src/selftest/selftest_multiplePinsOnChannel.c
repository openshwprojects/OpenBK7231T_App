#ifdef WINDOWS

#include "selftest_local.h"

static int PIN_BUTTON = 10;
static int PIN_LED_n = 11;
static int PIN_RELAY = 9;
static int PIN_RELAY_n = 8;

static void Test_MultiplePins_SimulateButtonClick() {
	// Simulate user pressing a button
	// NOTE: respect BTN_DEBOUNCE_TICKS
	Sim_RunFrames(100, false);
	// user presses the button and shorts it to ground
	SELFTEST_ASSERT_PIN_BOOLEAN(PIN_BUTTON, true);
	SIM_SetSimulatedPinValue(PIN_BUTTON, false);
	SELFTEST_ASSERT_PIN_BOOLEAN(PIN_BUTTON, false);
	// wait multiple frames with button pressed
	Sim_RunFrames(25, false);
	// release button (goes back to default pull up state)
	SIM_SetSimulatedPinValue(PIN_BUTTON, true);
	SELFTEST_ASSERT_PIN_BOOLEAN(PIN_BUTTON, true);
	Sim_RunFrames(15, false);
	Sim_RunFrames(100, false);
}
void Test_MultiplePinsOnChannel() {
	// reset whole device
	SIM_ClearOBK(0);


	PIN_SetPinRoleForPinIndex(PIN_LED_n, IOR_LED_n);
	PIN_SetPinChannelForPinIndex(PIN_LED_n, 1);

	// by default, button has a pull up resistor
	SIM_SetSimulatedPinValue(PIN_BUTTON, true);
	PIN_SetPinRoleForPinIndex(PIN_BUTTON, IOR_Button);
	PIN_SetPinChannelForPinIndex(PIN_BUTTON, 1);

	PIN_SetPinRoleForPinIndex(PIN_RELAY, IOR_Relay);
	PIN_SetPinChannelForPinIndex(PIN_RELAY, 1);

	PIN_SetPinRoleForPinIndex(PIN_RELAY_n, IOR_Relay_n);
	PIN_SetPinChannelForPinIndex(PIN_RELAY_n, 1);


	SELFTEST_ASSERT_CHANNEL(1, 0);

	// Simulate user pressing a button
	Test_MultiplePins_SimulateButtonClick();

	SELFTEST_ASSERT_CHANNEL(1, 1);
	// Channel value is now 1
	SELFTEST_ASSERT_PIN_BOOLEAN(PIN_BUTTON, true);// button always keeps pull up state
	SELFTEST_ASSERT_PIN_BOOLEAN(PIN_LED_n, false);
	SELFTEST_ASSERT_PIN_BOOLEAN(PIN_RELAY, true);
	SELFTEST_ASSERT_PIN_BOOLEAN(PIN_RELAY_n, false);


	// Simulate user pressing a button
	Test_MultiplePins_SimulateButtonClick();

	SELFTEST_ASSERT_CHANNEL(1, 0);
	// Channel value is now 0
	SELFTEST_ASSERT_PIN_BOOLEAN(PIN_BUTTON, true); // button always keeps pull up state
	SELFTEST_ASSERT_PIN_BOOLEAN(PIN_LED_n, true);
	SELFTEST_ASSERT_PIN_BOOLEAN(PIN_RELAY, false);
	SELFTEST_ASSERT_PIN_BOOLEAN(PIN_RELAY_n, true);


	CMD_ExecuteCommand("setChannel 1 0", 0);

	SELFTEST_ASSERT_CHANNEL(1, 0);
	// Channel value is now 0
	SELFTEST_ASSERT_PIN_BOOLEAN(PIN_BUTTON, true); // button always keeps pull up state
	SELFTEST_ASSERT_PIN_BOOLEAN(PIN_LED_n, true);
	SELFTEST_ASSERT_PIN_BOOLEAN(PIN_RELAY, false);
	SELFTEST_ASSERT_PIN_BOOLEAN(PIN_RELAY_n, true);

	CMD_ExecuteCommand("setChannel 1 1", 0);

	SELFTEST_ASSERT_CHANNEL(1, 1);
	// Channel value is now 1
	SELFTEST_ASSERT_PIN_BOOLEAN(PIN_BUTTON, true);// button always keeps pull up state
	SELFTEST_ASSERT_PIN_BOOLEAN(PIN_LED_n, false);
	SELFTEST_ASSERT_PIN_BOOLEAN(PIN_RELAY, true);
	SELFTEST_ASSERT_PIN_BOOLEAN(PIN_RELAY_n, false);
}


#endif
