#ifdef WINDOWS

#include "selftest_local.h"

void Test_HTTP_Client() {
	// reset whole device
	SIM_ClearOBK(0);


	PIN_SetPinRoleForPinIndex(9, IOR_Relay);
	PIN_SetPinChannelForPinIndex(9, 1);

	SELFTEST_ASSERT_CHANNEL(1, 0);

	// Also nice method of testing: addRepeatingEvent 2 -1 SendGet http://192.168.0.103/cm?cmnd=POWER%20TOGGLE
	///CMD_ExecuteCommand("SendGet http://192.168.0.103/cm?cmnd=POWER%20TOGGLE", 0);
	//CMD_ExecuteCommand("SendGet http://192.168.0.104/cm?cmnd=POWER%20TOGGLE", 0);


	// The following selftest may be problematic. It does a real TCP send to our loopback 127.0.0.1 address
	// It will also fail if port 80 is already in use (and not in use by us)
#if 0
	// send GET to ourselves (127.0.0.1 is a localhost)
	CMD_ExecuteCommand("SendGet http://127.0.0.1/cm?cmnd=POWER%20TOGGLE", 0);
	Sim_RunSeconds(5, true);
	SELFTEST_ASSERT_CHANNEL(1, 1);
	// send GET to ourselves (127.0.0.1 is a localhost)
	CMD_ExecuteCommand("SendGet http://127.0.0.1/cm?cmnd=POWER%20TOGGLE", 0);
	Sim_RunSeconds(5, true);
	SELFTEST_ASSERT_CHANNEL(1, 0);
	// send GET to ourselves (127.0.0.1 is a localhost)
	CMD_ExecuteCommand("SendGet http://127.0.0.1/cm?cmnd=POWER%20ON", 0);
	Sim_RunSeconds(5, true);
	SELFTEST_ASSERT_CHANNEL(1, 1);
#endif
}


#endif
