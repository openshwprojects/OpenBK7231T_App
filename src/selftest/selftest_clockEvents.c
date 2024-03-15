#ifdef WINDOWS

#include "selftest_local.h"
#include "../driver/drv_ntp.h"

static void ResetEventsAndChannels(int eventsCleared) {
	SELFTEST_ASSERT(NTP_ClearEvents() == eventsCleared);
	SELFTEST_ASSERT(NTP_PrintEventList() == 0);

	CMD_ExecuteCommand("setChannel 1 0", 0);
	CMD_ExecuteCommand("setChannel 2 0", 0);
	CMD_ExecuteCommand("setChannel 3 0", 0);
	SELFTEST_ASSERT_CHANNEL(1, 0);
	SELFTEST_ASSERT_CHANNEL(2, 0);
	SELFTEST_ASSERT_CHANNEL(3, 0);
}

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
	CMD_ExecuteCommand("addClockEvent 8:09:00 0xff 1007 POWER0 ON", 0);
	// now there is 2
	SELFTEST_ASSERT(NTP_PrintEventList() == 2);
	CMD_ExecuteCommand("addClockEvent 09:08:07 0xff 1008 POWER0 ON", 0);
	// now there is 3
	SELFTEST_ASSERT(NTP_PrintEventList() == 3);

	SELFTEST_ASSERT(NTP_GetEventTime(1006) == 15 * 3600 + 30 * 60);
	SELFTEST_ASSERT(NTP_GetEventTime(1007) == 8 * 3600 + 9 * 60);
	SELFTEST_ASSERT(NTP_GetEventTime(1008) == 9 * 3600 + 8 * 60 + 7);

	ResetEventsAndChannels(3);

	CMD_ExecuteCommand("addClockEvent 13:54:59 0xff 4 backlog setChannel 1 10; echo test event for 4", 0);
	CMD_ExecuteCommand("addClockEvent 13:55 0xff 5 backlog setChannel 2 20; echo test event for 5", 0);
	CMD_ExecuteCommand("addClockEvent 13:55:01 0xff 6 backlog setChannel 3 30; echo test event for 6", 0);

	unsigned int simTime = 1681998870;
	for (int i = 0; i < 100; i++) {
		NTP_RunEvents(simTime + i, true);
	}
	SELFTEST_ASSERT_CHANNEL(1, 10);
	SELFTEST_ASSERT_CHANNEL(2, 20);
	SELFTEST_ASSERT_CHANNEL(3, 30);


	for (int test = 0; test < 10; test++) {
		ResetEventsAndChannels(3);

		CMD_ExecuteCommand("addClockEvent 13:54:59 0xff 4 backlog addChannel 1 10; echo test event for 4", 0);
		CMD_ExecuteCommand("addClockEvent 13:55 0xff 5 backlog addChannel 2 20; echo test event for 5", 0);
		CMD_ExecuteCommand("addClockEvent 13:55:01 0xff 6 backlog addChannel 3 30; echo test event for 6", 0);

		// start 300 seconds earlier
		simTime = 1681998570;
		for (int i = 0; i < 500; i += abs(rand() % 40)) {
			NTP_RunEvents(simTime + i, true);
		}
		printf("Channel 1 is %i\n", CHANNEL_Get(1));
		printf("Channel 2 is %i\n", CHANNEL_Get(2));
		printf("Channel 3 is %i\n", CHANNEL_Get(3));
		SELFTEST_ASSERT_CHANNEL(1, 10);
		SELFTEST_ASSERT_CHANNEL(2, 20);
		SELFTEST_ASSERT_CHANNEL(3, 30);
	}

	// Test with calculated times
	ResetEventsAndChannels(3);

	CMD_ExecuteCommand("setChannel 4 13", 0);
	CMD_ExecuteCommand("setChannel 5 55", 0);
	CMD_ExecuteCommand("setChannel 6 $CH4*3600+54*60+59", 0);

	CMD_ExecuteCommand("setChannel 7 10", 0);

	CMD_ExecuteCommand("addClockEvent $CH6 0xff 4 backlog addChannel 1 $CH7; echo test event for 4", 0);
	CMD_ExecuteCommand("addClockEvent $CH4:$CH5 0xff 5 backlog addChannel 2 20; echo test event for 5", 0);
	CMD_ExecuteCommand("addClockEvent $CH4:$CH5:01 0xff 6 backlog addChannel 3 30; echo test event for 6", 0);
	CMD_ExecuteCommand("addClockEvent $CH4*3600+54*60+59 0xff 7 backlog addChannel 4 40; echo test event for 7", 0);

	CMD_ExecuteCommand("setChannel 7 25", 0);

	simTime = 1681998870;
	for (int i = 0; i < 100; i++) {
		NTP_RunEvents(simTime + i, true);
	}

	// Event handler command should expand constants when the handler is run.
	SELFTEST_ASSERT_CHANNEL(1, 25);
	SELFTEST_ASSERT_CHANNEL(2, 20);
	SELFTEST_ASSERT_CHANNEL(3, 30);
	SELFTEST_ASSERT_CHANNEL(4, 53);
}

#endif
