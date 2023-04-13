#ifdef WINDOWS

#include "selftest_local.h"
#include "../driver/drv_ntp.h"

void Test_ClockEvents() {
	// reset whole device
	SIM_ClearOBK(0);

	CMD_ExecuteCommand("startDriver NTP", 0);
	

	SELFTEST_ASSERT(NTP_PrintEventList() == 0);
	CMD_ExecuteCommand("addClockEvent 15:30:00 0xff 123 POWER0 ON", 0);
	// now there is 1
	SELFTEST_ASSERT(NTP_PrintEventList() == 1);
	// none removed
	SELFTEST_ASSERT(NTP_RemoveClockEvent(1245) == 0);
	// one removed
	SELFTEST_ASSERT(NTP_RemoveClockEvent(123) == 1);
	// now there is 0
	SELFTEST_ASSERT(NTP_PrintEventList() == 0);
	CMD_ExecuteCommand("addClockEvent 15:30:00 0xff 1001 POWER0 ON", 0);
	// now there is 1
	SELFTEST_ASSERT(NTP_PrintEventList() == 1);
	CMD_ExecuteCommand("addClockEvent 15:30:00 0xff 1002 POWER0 ON", 0);
	// now there is 2
	SELFTEST_ASSERT(NTP_PrintEventList() == 2);
	CMD_ExecuteCommand("addClockEvent 15:30:00 0xff 1003 POWER0 ON", 0);
	// now there is 3
	SELFTEST_ASSERT(NTP_PrintEventList() == 3);
	CMD_ExecuteCommand("addClockEvent 15:30:00 0xff 1004 POWER0 ON", 0);
	// now there is 4
	SELFTEST_ASSERT(NTP_PrintEventList() == 4);
	// one removed
	SELFTEST_ASSERT(NTP_RemoveClockEvent(1004) == 1);
	// now there is 3
	SELFTEST_ASSERT(NTP_PrintEventList() == 3);
	// one removed
	SELFTEST_ASSERT(NTP_RemoveClockEvent(1003) == 1);
	// now there is 2
	SELFTEST_ASSERT(NTP_PrintEventList() == 2);
	CMD_ExecuteCommand("addClockEvent 15:30:00 0xff 1005 POWER0 ON", 0);
	// now there is 3
	SELFTEST_ASSERT(NTP_PrintEventList() == 3);
	CMD_ExecuteCommand("addClockEvent 15:30:00 0xff 1006 POWER0 ON", 0);
	// now there is 4
	SELFTEST_ASSERT(NTP_PrintEventList() == 4);
	// one removed
	SELFTEST_ASSERT(NTP_RemoveClockEvent(1005) == 1);
	// now there is 3
	SELFTEST_ASSERT(NTP_PrintEventList() == 3);
	// one removed
	SELFTEST_ASSERT(NTP_RemoveClockEvent(1001) == 1);
	// now there is 2
	SELFTEST_ASSERT(NTP_PrintEventList() == 2);
	// one removed
	SELFTEST_ASSERT(NTP_RemoveClockEvent(1002) == 1);
	// now there is 1
	SELFTEST_ASSERT(NTP_PrintEventList() == 1);
	CMD_ExecuteCommand("addClockEvent 15:30:00 0xff 1007 POWER0 ON", 0);
	// now there is 2
	SELFTEST_ASSERT(NTP_PrintEventList() == 2);
	CMD_ExecuteCommand("addClockEvent 15:30:00 0xff 1008 POWER0 ON", 0);
	// now there is 3
	SELFTEST_ASSERT(NTP_PrintEventList() == 3);
	// removed all 3
	SELFTEST_ASSERT(NTP_ClearEvents() == 3);
	// now there is 0
	SELFTEST_ASSERT(NTP_PrintEventList() == 0);


}


#endif
