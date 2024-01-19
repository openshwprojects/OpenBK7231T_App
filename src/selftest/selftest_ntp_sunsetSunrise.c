#ifdef WINDOWS

#include "selftest_local.h"
#include "../driver/drv_ntp.h"

void Test_NTP_SunsetSunrise() {
	byte hour, minute;
	int sunrise, sunset;

	// reset whole device
	SIM_ClearOBK(0);

	// setup test case - Serbia
	CMD_ExecuteCommand("startDriver NTP", 0);
	CMD_ExecuteCommand("ntp_timeZoneOfs 1", 0);
	CMD_ExecuteCommand("ntp_setLatlong 44.272563 19.900690", 0);

	// set Tue, 19 Dec 2023 20:14:52
	NTP_SetSimulatedTime(1703016892);

	sunrise = NTP_GetSunrise();
	NTP_CalculateSunrise(&hour, &minute);
	// Expect sunrise at 7:11
	SELFTEST_ASSERT(hour == 7);
	SELFTEST_ASSERT(minute == 11);
	SELFTEST_ASSERT(sunrise == 25860);

	sunset = NTP_GetSunset();
	NTP_CalculateSunset(&hour, &minute);
	// Expect sunset at 16:02
	SELFTEST_ASSERT(hour == 16);
	SELFTEST_ASSERT(minute == 2);
	SELFTEST_ASSERT(sunset == 57720);

	// setup test case - Los Angeles
	CMD_ExecuteCommand("ntp_timeZoneOfs -8", 0);
	CMD_ExecuteCommand("ntp_setLatlong 34.052235 -118.243683", 0); 

	// set Tue, 19 Dec 2023 20:14:52
	NTP_SetSimulatedTime(1703016892);
	
	sunrise = NTP_GetSunrise();
	NTP_CalculateSunrise(&hour, &minute);
	// Expect sunrise at 6:52
	SELFTEST_ASSERT(hour == 6);
	SELFTEST_ASSERT(minute == 52);
	SELFTEST_ASSERT(sunrise == 24720);
	
	sunset = NTP_GetSunset();
	NTP_CalculateSunset(&hour, &minute);
	// Expect sunset at 16:45
	SELFTEST_ASSERT(hour == 16);
	SELFTEST_ASSERT(minute == 45);
	SELFTEST_ASSERT(sunset == 60300);

	// setup test case - Austin
	CMD_ExecuteCommand("ntp_timeZoneOfs -5", 0);
	CMD_ExecuteCommand("ntp_setLatlong 30.266666 -97.73333", 0);

	// set Wed Jul 12 2023 13:47:13 GMT+0000
	NTP_SetSimulatedTime(1689169633);

	sunrise = NTP_GetSunrise();
	NTP_CalculateSunrise(&hour, &minute);
	// Expect sunrise at 6:37
	SELFTEST_ASSERT(hour == 6);
	SELFTEST_ASSERT(minute == 37);
	SELFTEST_ASSERT(sunrise == 23820);

	sunset = NTP_GetSunset();
	NTP_CalculateSunset(&hour, &minute);
	// Expect sunset at 20:34
	SELFTEST_ASSERT(hour == 20);
	SELFTEST_ASSERT(minute == 34);
	SELFTEST_ASSERT(sunset == 74040);

	// set Warsaw
	CMD_ExecuteCommand("ntp_setLatlong 52.237049 21.017532", 0);
	CMD_ExecuteCommand("ntp_timeZoneOfs 1", 0);
	NTP_SetSimulatedTime(1703081805);

	// 15:16 Poland Warsaw
	SELFTEST_ASSERT_EXPRESSION("$hour", 15);
	SELFTEST_ASSERT_EXPRESSION("$minute", 16);
	CMD_ExecuteCommand("setChannel 15 123", 0);
	SELFTEST_ASSERT_CHANNEL(15,123);

	// expect set at 15:23
	NTP_CalculateSunset(&hour, &minute);
	SELFTEST_ASSERT(hour == 15);
	SELFTEST_ASSERT(minute == 23);
	SELFTEST_ASSERT_EXPRESSION("$sunset", 55380);
	
	// expect rise at 7:41
	NTP_CalculateSunrise(&hour, &minute);
	SELFTEST_ASSERT(hour == 7);
	SELFTEST_ASSERT(minute == 41);
	SELFTEST_ASSERT_EXPRESSION("$sunrise", 27660);

	CMD_ExecuteCommand("addClockEvent sunset 0x7f 12345 setChannel 15 2020", 0);
	CMD_ExecuteCommand("addClockEvent sunrise 0x7f 12345 setChannel 15 4567", 0);

	// during next 10 minutes, the rise should occur
	int runSeconds = 10 * 60;
	for (int i = 0; i < runSeconds; i++) {
		NTP_OnEverySecond();
	}
	SELFTEST_ASSERT_CHANNEL(15, 2020);
	// now we should be at about 15:26
	SELFTEST_ASSERT_EXPRESSION("$hour", 15);
	SELFTEST_ASSERT_EXPRESSION("$minute", 26);

	g_ntpTime += (9+6)*(60 * 60);
	// 6:26 now
	SELFTEST_ASSERT_EXPRESSION("$hour", 6);
	SELFTEST_ASSERT_EXPRESSION("$minute", 26);
	g_ntpTime += (34 * 60);
	// 7:00 now
	SELFTEST_ASSERT_EXPRESSION("$hour", 7);
	SELFTEST_ASSERT_EXPRESSION("$minute", 0);
	g_ntpTime += (35 * 60);
	// 7:35 now
	SELFTEST_ASSERT_EXPRESSION("$hour", 7);
	SELFTEST_ASSERT_EXPRESSION("$minute", 35);
	SELFTEST_ASSERT_CHANNEL(15, 2020);
	// during next 10 minutes, the rise should occur
	runSeconds = 10 * 60;
	for (int i = 0; i < runSeconds; i++) {
		NTP_OnEverySecond();
	}
	// channel value should change
	SELFTEST_ASSERT_CHANNEL(15, 4567);

}


#endif
