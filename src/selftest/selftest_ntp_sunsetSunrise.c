#ifdef WINDOWS

#include "selftest_local.h"

void Test_NTP_SunsetSunrise() {
	byte hour, minute;

	// reset whole device
	SIM_ClearOBK(0);

	// setup test case - Serbia
	CMD_ExecuteCommand("startDriver NTP", 0);
	CMD_ExecuteCommand("ntp_timeZoneOfs 1", 0);
	CMD_ExecuteCommand("ntp_setLatlong 44.272563 19.900690", 0);
	// set Tue, 19 Dec 2023 20:14:52
	NTP_SetSimulatedTime(1703016892);
	NTP_CalculateSunrise(&hour, &minute);
	// Expect sunrise at 7:11
	SELFTEST_ASSERT(hour == 7);
	SELFTEST_ASSERT(minute == 11);
	NTP_CalculateSunset(&hour, &minute);
	// Expect sunset at 16:02
	SELFTEST_ASSERT(hour == 16);
	SELFTEST_ASSERT(minute == 2);


	// setup test case - Los Angeles
	CMD_ExecuteCommand("ntp_timeZoneOfs -8", 0);
	CMD_ExecuteCommand("ntp_setLatlong 34.052235 -118.243683", 0); 

	// set Tue, 19 Dec 2023 20:14:52
	NTP_SetSimulatedTime(1703016892);
	NTP_CalculateSunrise(&hour, &minute);
	// Expect sunrise at 6:52
	SELFTEST_ASSERT(hour == 6);
	SELFTEST_ASSERT(minute == 52);
	NTP_CalculateSunset(&hour, &minute);
	// Expect sunset at 16:462
	SELFTEST_ASSERT(hour == 16);
	SELFTEST_ASSERT(minute == 45);
}


#endif
