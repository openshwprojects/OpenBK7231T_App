#ifdef WINDOWS

#include "selftest_local.h"

void Test_NTP() {
	// reset whole device
	SIM_ClearOBK(0);

	CMD_ExecuteCommand("startDriver NTP", 0);

	SELFTEST_ASSERT_INTCOMPARE(NTP_GetTimesZoneOfsSeconds(), 0);

	// 1 hour
	CMD_ExecuteCommand("ntp_timeZoneOfs 1", 0);
	SELFTEST_ASSERT_INTCOMPARE(NTP_GetTimesZoneOfsSeconds(), 1*60*60);

	// 2 hour
	CMD_ExecuteCommand("ntp_timeZoneOfs 2", 0);
	SELFTEST_ASSERT_INTCOMPARE(NTP_GetTimesZoneOfsSeconds(), 2 * 60 * 60);

	// -2 hour
	CMD_ExecuteCommand("ntp_timeZoneOfs -2", 0);
	SELFTEST_ASSERT_INTCOMPARE(NTP_GetTimesZoneOfsSeconds(), -2 * 60 * 60);

	// 2 hour 30 minutes
	CMD_ExecuteCommand("ntp_timeZoneOfs 2:30", 0);
	SELFTEST_ASSERT_INTCOMPARE(NTP_GetTimesZoneOfsSeconds(), 2 * 60 * 60 + 30 * 60);

	// 3 hour 15 minutes
	CMD_ExecuteCommand("ntp_timeZoneOfs 03:15", 0);
	SELFTEST_ASSERT_INTCOMPARE(NTP_GetTimesZoneOfsSeconds(), 3 * 60 * 60 + 15 * 60);

	// - 2 hour 30 minutes
	CMD_ExecuteCommand("ntp_timeZoneOfs -2:30", 0);
	SELFTEST_ASSERT_INTCOMPARE(NTP_GetTimesZoneOfsSeconds(), -(2 * 60 * 60 + 30 * 60));

	// - 12 hour 5 minutes
	CMD_ExecuteCommand("ntp_timeZoneOfs -12:05", 0);
	SELFTEST_ASSERT_INTCOMPARE(NTP_GetTimesZoneOfsSeconds(), -(12 * 60 * 60 + 5 * 60));



}


#endif
