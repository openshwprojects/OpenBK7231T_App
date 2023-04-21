#ifdef WINDOWS

#include "selftest_local.h"

/*
// Another shutter idea

// Channel 10 - Relay 1
// Channel 11 - Relay 2

alias Set_Up setChannel 10 0; setChannel 11 1
alias Set_Down setChannel 10 1; setChannel 11 0
alias Set_None setChannel 10 0; setChannel 11 0

// command "Start_Opening" will first kill other script threads, and start with clear open
alias Start_Opening backlog stopAllScripts; startScript autoexec.bat openShutter
// command "Start_Closing" will first kill other script threads, and start with clear close
alias Start_Closing backlog stopAllScripts; startScript autoexec.bat closeShutter
// command "Start_Closing" will first kill other script threads, and then stop
alias Stop_All backlog stopAllScripts; Set_None

// create GUI buttons for HTTP panel
startDriver httpButtons
setButtonEnabled 0 1
setButtonLabel 0 "Open"
setButtonCommand 0 Start_Opening

setButtonEnabled 1 1
setButtonLabel 1 "Close"
setButtonCommand 1 Start_Closing

setButtonEnabled 2 1
setButtonLabel 2 "Stop"
setButtonCommand 2 Stop_All

// link the same commands to physical button on GPIO pins
// 8 and 9 are GPIO indices, like P9, etc
addEventHandler OnClick 8 Start_Opening
addEventHandler OnClick 9 Start_Closing
addEventHandler OnClick 10 Stop_All

// do not proceed
return

// Script thread for opening
openShutter:
// setup none
Set_None
// wait 0.1 seconds
delay_s 0.1
// setup up
Set_Up
// wait 10 seconds
delay_s 10
Set_None
// done
return

// Script thread for closing
closeShutter:
// setup none
Set_None
// wait 0.1 seconds
delay_s 0.1
// setup down
Set_Down
// wait 10 seconds
delay_s 10
Set_None
// done
return

*/
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
