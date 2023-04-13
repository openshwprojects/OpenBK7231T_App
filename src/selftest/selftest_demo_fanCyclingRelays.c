#ifdef WINDOWS

#include "selftest_local.h"

void SIM_SimulateUserClickOnPin(int pin) {
	SIM_SetSimulatedPinValue(pin, false);
	Sim_RunMiliseconds(150, false);
	SIM_SetSimulatedPinValue(pin, true);
	Sim_RunMiliseconds(450, false);
}
// Another script demo for user
/*
// display on gui
setChannelType 10 OffLowMidHigh
setChannelLabel 1 "Relay 1"
setChannelLabel 2 "Relay 2"
setChannelLabel 3 "Relay 3"
setChannelLabel 10 "Fan Speed"

// hide raw relay channel buttons from gui
setChannelVisible 1 0
setChannelVisible 2 0
setChannelVisible 3 0

// optional gui buttons
startDriver httpButtons

setButtonLabel 0 "Toggle Fan"
setButtonLabel 1 "Next Fan Speed"
setButtonCommand 0 Do_Power_Press
setButtonCommand 1 Do_Cycle_Press
setButtonEnabled 0 1
setButtonEnabled 1 1

// button colour
addChangeHandler Channel10 == 0 setButtonColor 0 red
addChangeHandler Channel10 != 0 setButtonColor 0 green
// off by default
setButtonColor 0 red

// translate fan state (0-4) to relay states
addChangeHandler Channel10 == 0 backlog SetChannel 1 0; SetChannel 2 0; SetChannel 3 0
// Fan on Low - R2 on
addChangeHandler Channel10 == 1 backlog SetChannel 1 1; SetChannel 2 0; SetChannel 3 0
// Fan on Medium - R2, R3 on
addChangeHandler Channel10 == 2 backlog SetChannel 1 1; SetChannel 2 1; SetChannel 3 0
// Fan on High - R2, R3, R4 on
addChangeHandler Channel10 == 3 backlog SetChannel 1 1; SetChannel 2 1; SetChannel 3 1
// change to OnClick if you want?
addEventHandler OnPress 11 Do_Power_Press
addEventHandler OnPress 9 Do_Cycle_Press
// power press will just toggle channel 10 - so if non zero it goes to 0, if zero, then goes to 1
alias Do_Power_Press toggleChannel 10
// cycle press will cycle values 0, 1, 2 and 3 for channel 10?
// AddChannel [ChannelIndex][ValueToAdd][ClampMin][ClampMax][bWrapInsteadOfClamp]
alias Do_Cycle_Press addChannel 10 1 1 3 1


*/
void Test_Demo_FanCyclingRelays() {
	// reset whole device
	SIM_ClearOBK(0);

	// Pins 3 and 4 are buttons
	PIN_SetPinRoleForPinIndex(3, IOR_Button_ScriptOnly);
	SIM_SetSimulatedPinValue(3, true);
	PIN_SetPinRoleForPinIndex(4, IOR_Button_ScriptOnly);
	SIM_SetSimulatedPinValue(4, true);

	Sim_RunMiliseconds(500, false);

	// Channels 1, 2, 3 are relays
	PIN_SetPinRoleForPinIndex(9, IOR_Relay);
	PIN_SetPinChannelForPinIndex(9, 1);
	PIN_SetPinRoleForPinIndex(10, IOR_Relay);
	PIN_SetPinChannelForPinIndex(10, 2);
	PIN_SetPinRoleForPinIndex(11, IOR_Relay);
	PIN_SetPinChannelForPinIndex(11, 3);

	// Channel 10 is fan speed - 0, 1, 2 or 3
	// Setting channel 10 to 0, 1, or 2 or 3 will propagate to relays
	// Fan on Off - all relays off
	CMD_ExecuteCommand("addChangeHandler Channel10 == 0 backlog SetChannel 1 0; SetChannel 2 0; SetChannel 3 0", 0);
	// Fan on Low - R2 on
	CMD_ExecuteCommand("addChangeHandler Channel10 == 1 backlog SetChannel 1 1; SetChannel 2 0; SetChannel 3 0", 0);
	// Fan on Medium - R2, R3 on
	CMD_ExecuteCommand("addChangeHandler Channel10 == 2 backlog SetChannel 1 1; SetChannel 2 1; SetChannel 3 0", 0);
	// Fan on High - R2, R3, R4 on
	CMD_ExecuteCommand("addChangeHandler Channel10 == 3 backlog SetChannel 1 1; SetChannel 2 1; SetChannel 3 1", 0);

	// change to OnClick if you want?
	CMD_ExecuteCommand("addEventHandler OnPress 3 Do_Power_Press", 0);
	CMD_ExecuteCommand("addEventHandler OnPress 4 Do_Cycle_Press", 0);

	// power press will just toggle channel 10 - so if non zero it goes to 0, if zero, then goes to 1
	CMD_ExecuteCommand("alias Do_Power_Press toggleChannel 10", 0);
	// cycle press will cycle values 0, 1, 2 and 3 for channel 10?
	// AddChannel [ChannelIndex][ValueToAdd][ClampMin][ClampMax][bWrapInsteadOfClamp]
	CMD_ExecuteCommand("alias Do_Cycle_Press addChannel 10 1 1 3 1", 0);

	// start with empty channels
	SELFTEST_ASSERT_CHANNEL(1, 0); // relay
	SELFTEST_ASSERT_CHANNEL(2, 0); // relay
	SELFTEST_ASSERT_CHANNEL(3, 0); // relay

	SELFTEST_ASSERT_CHANNEL(10, 0); // Fan state - 0 to 4 values

	// user clicks POWER
	SIM_SimulateUserClickOnPin(3);
	// Now lowest speed is set
	SELFTEST_ASSERT_CHANNEL(1, 1); // relay
	SELFTEST_ASSERT_CHANNEL(2, 0); // relay
	SELFTEST_ASSERT_CHANNEL(3, 0); // relay

	SELFTEST_ASSERT_CHANNEL(10, 1); // Fan state - 0 to 4 values

	// user clicks POWER
	SIM_SimulateUserClickOnPin(3);
	// Now device is again off
	SELFTEST_ASSERT_CHANNEL(1, 0); // relay
	SELFTEST_ASSERT_CHANNEL(2, 0); // relay
	SELFTEST_ASSERT_CHANNEL(3, 0); // relay

	SELFTEST_ASSERT_CHANNEL(10, 0); // Fan state - 0 to 4 values

	// user clicks POWER
	SIM_SimulateUserClickOnPin(3);
	// Now lowest speed is set
	SELFTEST_ASSERT_CHANNEL(1, 1); // relay
	SELFTEST_ASSERT_CHANNEL(2, 0); // relay
	SELFTEST_ASSERT_CHANNEL(3, 0); // relay

	SELFTEST_ASSERT_CHANNEL(10, 1); // Fan state - 0 to 4 values


	// user clicks SPEED
	SIM_SimulateUserClickOnPin(4);
	// second speed is set (because it was lowest previously)
	SELFTEST_ASSERT_CHANNEL(1, 1); // relay
	SELFTEST_ASSERT_CHANNEL(2, 1); // relay
	SELFTEST_ASSERT_CHANNEL(3, 0); // relay

	SELFTEST_ASSERT_CHANNEL(10, 2); // Fan state - 0 to 4 values


	// user clicks SPEED
	SIM_SimulateUserClickOnPin(4);
	// second speed is set (because it was lowest previously)
	SELFTEST_ASSERT_CHANNEL(1, 1); // relay
	SELFTEST_ASSERT_CHANNEL(2, 1); // relay
	SELFTEST_ASSERT_CHANNEL(3, 1); // relay

	SELFTEST_ASSERT_CHANNEL(10, 3); // Fan state - 0 to 4 values

	// speed button is clamped to 1 to 3 range, so it's not possible to turn off by speed button
	// From highest (3) it clamps to lowest (1)
	// user clicks SPEED
	SIM_SimulateUserClickOnPin(4);
	// second speed is set (because it was lowest previously)
	SELFTEST_ASSERT_CHANNEL(1, 1); // relay
	SELFTEST_ASSERT_CHANNEL(2, 0); // relay
	SELFTEST_ASSERT_CHANNEL(3, 0); // relay

	SELFTEST_ASSERT_CHANNEL(10, 1); // Fan state - 0 to 4 values
}


#endif
