#ifdef WINDOWS

#include "selftest_local.h"

void Test_NTP_DST() {
	// reset whole device
	SIM_ClearOBK(0);

	CMD_ExecuteCommand("startDriver NTP", 0);
	CMD_ExecuteCommand("ntp_timeZoneOfs 1", 0);
	// set Sunday, Mar 31 2024 01:59:55 CET
	// that's 5 seconds before DST starts in Europe
	NTP_SetSimulatedTime(1711846795);

	SELFTEST_ASSERT_EXPRESSION("$minute",59);
	SELFTEST_ASSERT_EXPRESSION("$hour", 1);
	SELFTEST_ASSERT_EXPRESSION("$second", 55);
	SELFTEST_ASSERT_EXPRESSION("$day", 0);
	SELFTEST_ASSERT_EXPRESSION("$mday", 31);
	SELFTEST_ASSERT_EXPRESSION("$month", 3);
	SELFTEST_ASSERT_EXPRESSION("$year", 2024);



	// now set DST information for Europe:
	// DST ends last Sunday of October at 3 local (summer) time
	// DST starts last Sunday of March at 2 local (standard) time
	// arguments are: nthWeekEnd, monthEnd, dayEnd, hourEnd, nthWeekStart, monthStart, dayStart, hourStart
	// 
	// 0 10 1 3 	means "End of DST on 	last(=0) Octobers(=10) Sunday(=1) at 3"
	// 0 3 1 2 	means "Start of DST on 	last(=0) Marchs(=3) Sunday(=1) at 2"
	CMD_ExecuteCommand("CLOCK_calcDST 0 10 1 3 0 3 1 2", 0);
	// test - we are (slightly) "before" DST
	SELFTEST_ASSERT_EXPRESSION("$isDST", 0);

	// advace 6 seconds, so we are 1 second after 2:00, hence summertime started
	// in fact the actual time could never happen, since clock
	// advances from 2:00:00 to 3:00:00 clock can't be 2:00:01 on this day
	// --> NTP offset should be 2 now, we will set that later
	Sim_RunSeconds(6, false);

	// check time
	// DST switch should have been done by system

	SELFTEST_ASSERT_EXPRESSION("$hour", 3);
	SELFTEST_ASSERT_EXPRESSION("$minute",00);
	SELFTEST_ASSERT_EXPRESSION("$second", 01);
	// test, that we are _in_ DST now
	SELFTEST_ASSERT_EXPRESSION("$isDST", 1);

	// set Sunday, Oct 27 2024 02:59:55 CEST
	// it's summertime now (for another 5 seconds)
	// because that's 5 seconds before DST ends in Europe
	NTP_SetSimulatedTime(1729990795);

	SELFTEST_ASSERT_EXPRESSION("$minute",59);
	SELFTEST_ASSERT_EXPRESSION("$hour", 2);
	SELFTEST_ASSERT_EXPRESSION("$second", 55);
	SELFTEST_ASSERT_EXPRESSION("$day", 0);
	SELFTEST_ASSERT_EXPRESSION("$mday", 27);
	SELFTEST_ASSERT_EXPRESSION("$month", 10);
	SELFTEST_ASSERT_EXPRESSION("$year", 2024);
	// test, that we are still _in_ DST now
	SELFTEST_ASSERT_EXPRESSION("$isDST", 1);

	// advace 6 seconds, so we are 1 second after 3:00, hence summertime ended
	Sim_RunSeconds(6, false);

	// check time - DST switch should have hapened, so clock was 
	// turned back for 1 hour
	SELFTEST_ASSERT_EXPRESSION("$hour", 2);
	SELFTEST_ASSERT_EXPRESSION("$minute",00);
	SELFTEST_ASSERT_EXPRESSION("$second", 01);
	// test, that we are _not_ in DST any more
	SELFTEST_ASSERT_EXPRESSION("$isDST", 0);

	// some more tests with directly setting the ntp time
	// just checking DST function ...

	// 1743296395 = Sun, Mar 30 2025 01:59:55 CET
	CMD_ExecuteCommand("ntp_timeZoneOfs 1", 0);
	NTP_SetSimulatedTime(1743296395);
	SELFTEST_ASSERT_EXPRESSION("$isDST", 0);
	Sim_RunSeconds(6, false);
	SELFTEST_ASSERT_EXPRESSION("$isDST", 1);
	// 1761440395 = Sun, Oct 26 2025 02:59:55 CEST
//	CMD_ExecuteCommand("ntp_timeZoneOfs 2", 0);
	NTP_SetSimulatedTime(1761440395);
	SELFTEST_ASSERT_EXPRESSION("$isDST", 1);
	Sim_RunSeconds(6, false);
	SELFTEST_ASSERT_EXPRESSION("$isDST", 0);
	
	// 1774745995 = Sun, Mar 29 2026 01:59:55 CET
	CMD_ExecuteCommand("ntp_timeZoneOfs 1", 0);
	NTP_SetSimulatedTime(1774745995);
	SELFTEST_ASSERT_EXPRESSION("$isDST", 0);
	Sim_RunSeconds(6, false);
	SELFTEST_ASSERT_EXPRESSION("$isDST", 1);
	// 1792889995 = Sun, Oct 25 2026 02:59:55 CEST
//	CMD_ExecuteCommand("ntp_timeZoneOfs 2", 0);
	NTP_SetSimulatedTime(1792889995);
	SELFTEST_ASSERT_EXPRESSION("$isDST", 1);
	Sim_RunSeconds(6, false);
	SELFTEST_ASSERT_EXPRESSION("$isDST", 0);
	
	// 1806195595 = Sun, Mar 28 2027 01:59:55 CET
	CMD_ExecuteCommand("ntp_timeZoneOfs 1", 0);
	NTP_SetSimulatedTime(1806195595);
	SELFTEST_ASSERT_EXPRESSION("$isDST", 0);
	Sim_RunSeconds(6, false);
	SELFTEST_ASSERT_EXPRESSION("$isDST", 1);
	// 1824944395 = Sun, Oct 31 2027 02:59:55 CEST
//	CMD_ExecuteCommand("ntp_timeZoneOfs 2", 0);
	NTP_SetSimulatedTime(1824944395);
	SELFTEST_ASSERT_EXPRESSION("$isDST", 1);
	Sim_RunSeconds(6, false);
	SELFTEST_ASSERT_EXPRESSION("$isDST", 0);

	// 1837645195 = Sun, Mar 26 2028 01:59:55 CET
	CMD_ExecuteCommand("ntp_timeZoneOfs 1", 0);
	NTP_SetSimulatedTime(1837645195);
	SELFTEST_ASSERT_EXPRESSION("$isDST", 0);
	Sim_RunSeconds(6, false);
	SELFTEST_ASSERT_EXPRESSION("$isDST", 1);
	// 1856393995 = Sun, Oct 29 2028 02:59:55 CEST
//	CMD_ExecuteCommand("ntp_timeZoneOfs 2", 0);
	NTP_SetSimulatedTime(1856393995);
	SELFTEST_ASSERT_EXPRESSION("$isDST", 1);
	Sim_RunSeconds(6, false);
	SELFTEST_ASSERT_EXPRESSION("$isDST", 0);

	// 1869094795 = Sun, Mar 25 2029 01:59:55 CET
	CMD_ExecuteCommand("ntp_timeZoneOfs 1", 0);
	NTP_SetSimulatedTime(1869094795);
	SELFTEST_ASSERT_EXPRESSION("$isDST", 0);
	Sim_RunSeconds(6, false);
	SELFTEST_ASSERT_EXPRESSION("$isDST", 1);
	// 1887843595 = Sun, Oct 28 2029 02:59:55 CEST
//	CMD_ExecuteCommand("ntp_timeZoneOfs 2", 0);
	NTP_SetSimulatedTime(1887843595);
	SELFTEST_ASSERT_EXPRESSION("$isDST", 1);
	Sim_RunSeconds(6, false);
	SELFTEST_ASSERT_EXPRESSION("$isDST", 0);

	// 1901149195 = Sun, Mar 31 2030 01:59:55 CET
	CMD_ExecuteCommand("ntp_timeZoneOfs 1", 0);
	NTP_SetSimulatedTime(1901149195);
	SELFTEST_ASSERT_EXPRESSION("$isDST", 0);
	Sim_RunSeconds(6, false);
	SELFTEST_ASSERT_EXPRESSION("$isDST", 1);
	// 1919293195 = Sun, Oct 27 2030 02:59:55 CEST
//	CMD_ExecuteCommand("ntp_timeZoneOfs 2", 0);
	NTP_SetSimulatedTime(1919293195);
	SELFTEST_ASSERT_EXPRESSION("$isDST", 1);
	Sim_RunSeconds(6, false);
	SELFTEST_ASSERT_EXPRESSION("$isDST", 0);

}


#endif
