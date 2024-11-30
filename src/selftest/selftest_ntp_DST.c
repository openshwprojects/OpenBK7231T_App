#ifdef WINDOWS

#include "selftest_local.h"

void Test_ntp_DST() {
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
	SELFTEST_ASSERT_EXPRESSION("$hour", 2);
	SELFTEST_ASSERT_EXPRESSION("$minute",00);
	SELFTEST_ASSERT_EXPRESSION("$second", 01);
	// test, that we are _in_ DST now
	SELFTEST_ASSERT_EXPRESSION("$isDST", 1);
	// adjust NTP offset, it's 2 hours durin summer time
	CMD_ExecuteCommand("ntp_timeZoneOfs 2", 0);
	// check time
	SELFTEST_ASSERT_EXPRESSION("$hour", 3);
	SELFTEST_ASSERT_EXPRESSION("$minute",00);
	SELFTEST_ASSERT_EXPRESSION("$second", 01);

	// set Sunday, Oct 27 2024 02:59:55 CEST
	// remember: we set NTP offset to 2 before!
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

	// check time (again: strictly speaking hour would be still 2, since clock was 
	// turned back for 1 hour, but since we didn't alter offset yet ...
	SELFTEST_ASSERT_EXPRESSION("$hour", 3);
	SELFTEST_ASSERT_EXPRESSION("$minute",00);
	SELFTEST_ASSERT_EXPRESSION("$second", 01);
	// test, that we are _not_ in DST any more
	SELFTEST_ASSERT_EXPRESSION("$isDST", 0);
	// adjust NTP offset, it's 1 hours durin standard time
	CMD_ExecuteCommand("ntp_timeZoneOfs 1", 0);
	// check time
	SELFTEST_ASSERT_EXPRESSION("$hour", 2);
	SELFTEST_ASSERT_EXPRESSION("$minute",00);
	SELFTEST_ASSERT_EXPRESSION("$second", 01);

	// some more tests with directly setting the ntp time
	// just checking DST function ...

	// 1743296395 = So 30. Mär 01:59:55 CET 2025
	NTP_SetSimulatedTime(1743296395);
	SELFTEST_ASSERT_EXPRESSION("$isDST", 0);
	Sim_RunSeconds(6, false);
	SELFTEST_ASSERT_EXPRESSION("$isDST", 1);
	// 1761440395 = So 26. Okt 02:59:55 CEST 2025
	NTP_SetSimulatedTime(1761440395);
	SELFTEST_ASSERT_EXPRESSION("$isDST", 1);
	Sim_RunSeconds(6, false);
	SELFTEST_ASSERT_EXPRESSION("$isDST", 0);
	// 1774745995 = So 29. Mär 01:59:55 CET 2026
	NTP_SetSimulatedTime(1774745995);
	SELFTEST_ASSERT_EXPRESSION("$isDST", 0);
	Sim_RunSeconds(6, false);
	SELFTEST_ASSERT_EXPRESSION("$isDST", 1);
	// 1792889995 = So 25. Okt 02:59:55 CEST 2026
	NTP_SetSimulatedTime(1792889995);
	SELFTEST_ASSERT_EXPRESSION("$isDST", 1);
	Sim_RunSeconds(6, false);
	SELFTEST_ASSERT_EXPRESSION("$isDST", 0);
	// 1806195595 = So 28. Mär 01:59:55 CET 2027
	NTP_SetSimulatedTime(1806195595);
	SELFTEST_ASSERT_EXPRESSION("$isDST", 0);
	Sim_RunSeconds(6, false);
	SELFTEST_ASSERT_EXPRESSION("$isDST", 1);
	// 1824944395 = So 31. Okt 02:59:55 CEST 2027
	NTP_SetSimulatedTime(1824944395);
	SELFTEST_ASSERT_EXPRESSION("$isDST", 1);
	Sim_RunSeconds(6, false);
	SELFTEST_ASSERT_EXPRESSION("$isDST", 0);
	// 1837645195 = So 26. Mär 01:59:55 CET 2028
	NTP_SetSimulatedTime(1837645195);
	SELFTEST_ASSERT_EXPRESSION("$isDST", 0);
	Sim_RunSeconds(6, false);
	SELFTEST_ASSERT_EXPRESSION("$isDST", 1);
	// 1856393995 = So 29. Okt 02:59:55 CEST 2028
	NTP_SetSimulatedTime(1856393995);
	SELFTEST_ASSERT_EXPRESSION("$isDST", 1);
	Sim_RunSeconds(6, false);
	SELFTEST_ASSERT_EXPRESSION("$isDST", 0);
	// 1869094795 = So 25. Mär 01:59:55 CET 2029
	NTP_SetSimulatedTime(1869094795);
	SELFTEST_ASSERT_EXPRESSION("$isDST", 0);
	Sim_RunSeconds(6, false);
	SELFTEST_ASSERT_EXPRESSION("$isDST", 1);
	// 1887843595 = So 28. Okt 02:59:55 CEST 2029
	NTP_SetSimulatedTime(1887843595);
	SELFTEST_ASSERT_EXPRESSION("$isDST", 1);
	Sim_RunSeconds(6, false);
	SELFTEST_ASSERT_EXPRESSION("$isDST", 0);
	// 1901149195 = So 31. Mär 01:59:55 CET 2030
	NTP_SetSimulatedTime(1901149195);
	SELFTEST_ASSERT_EXPRESSION("$isDST", 0);
	Sim_RunSeconds(6, false);
	SELFTEST_ASSERT_EXPRESSION("$isDST", 1);
	// 1919293195 = So 27. Okt 02:59:55 CEST 2030
	NTP_SetSimulatedTime(1919293195);
	SELFTEST_ASSERT_EXPRESSION("$isDST", 1);
	Sim_RunSeconds(6, false);
	SELFTEST_ASSERT_EXPRESSION("$isDST", 0);

}


#endif
