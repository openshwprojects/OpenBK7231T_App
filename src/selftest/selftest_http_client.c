#ifdef WINDOWS

#include "selftest_local.h".

void Test_HTTP_Client() {
	// reset whole device
	CMD_ExecuteCommand("clearAll", 0);

	CMD_ExecuteCommand("SendGet http://192.168.0.103/cm?cmnd=POWER%20TOGGLE", 0);
	//CMD_ExecuteCommand("SendGet http://192.168.0.104/cm?cmnd=POWER%20TOGGLE", 0);
	//SELFTEST_ASSERT_CHANNEL(5, 666);
}


#endif
