#ifdef WINDOWS

#include "selftest_local.h"

void Test_Commands_Startup() {
	// reset whole device
	SIM_ClearOBK(0);

	CMD_ExecuteCommand("StartupCommand Test123", 0);

	SELFTEST_ASSERT_STRING(CFG_GetShortStartupCommand(), "Test123");
	CMD_ExecuteCommand("StartupCommand \"Space, space!\"", 0);
	SELFTEST_ASSERT_STRING(CFG_GetShortStartupCommand(), "Space, space!");
	CMD_ExecuteCommand("StartupCommand \"Space, space! Very long string with whitespaces in quotation marks!\"", 0);
	SELFTEST_ASSERT_STRING(CFG_GetShortStartupCommand(), "Space, space! Very long string with whitespaces in quotation marks!");
	CMD_ExecuteCommand("StartupCommand \"\"", 0);
	SELFTEST_ASSERT_STRING(CFG_GetShortStartupCommand(), "");

	CMD_ExecuteCommand("setChannel 0 0", 0);
	SELFTEST_ASSERT_CHANNEL(0, 0);

	// set, but do not execute!
	CMD_ExecuteCommand("StartupCommand \"setChannel 0 123\"", 0);
	SELFTEST_ASSERT_STRING(CFG_GetShortStartupCommand(), "setChannel 0 123");
	// still 0
	SELFTEST_ASSERT_CHANNEL(0, 0);
	// set, and execute
	CMD_ExecuteCommand("StartupCommand \"setChannel 0 234\" 1", 0);
	SELFTEST_ASSERT_STRING(CFG_GetShortStartupCommand(), "setChannel 0 234");
	SELFTEST_ASSERT_CHANNEL(0, 234);

	const char *loremIpsum = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum. Phasellus pretium felis vel metus dictum, non lacinia elit interdum. Proin vehicula sem vel orci convallis pharetra. Vivamus scelerisque, erat id tincidunt fringilla, felis enim vehicula ipsum, in condimentum risus enim eget mi. Curabitur nec felis libero. Cras mattis justo a nulla malesuada, sit amet gravida urna dictum. Integer et nisi sed tortor dictum tincidunt. Donec a lectus sit amet sapien suscipit fringilla. Nulla facilisi. Morbi ac facilisis eros. Suspendisse facilisis convallis eros, in sagittis nisi eleifend vitae.";
	CFG_SetShortStartupCommand(loremIpsum);
	CMD_ExecuteCommand(CFG_GetShortStartupCommand(), 0);


	const char *loremIpsum2 = "startDriver Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum. Phasellus pretium felis vel metus dictum, non lacinia elit interdum. Proin vehicula sem vel orci convallis pharetra. Vivamus scelerisque, erat id tincidunt fringilla, felis enim vehicula ipsum, in condimentum risus enim eget mi. Curabitur nec felis libero. Cras mattis justo a nulla malesuada, sit amet gravida urna dictum. Integer et nisi sed tortor dictum tincidunt. Donec a lectus sit amet sapien suscipit fringilla. Nulla facilisi. Morbi ac facilisis eros. Suspendisse facilisis convallis eros, in sagittis nisi eleifend vitae.";
	CFG_SetShortStartupCommand(loremIpsum2);
	CMD_ExecuteCommand(CFG_GetShortStartupCommand(), 0);

	const char *loremIpsum3 = "startDriver DS1820startDriver BMP280 25 24 9 10 11 236// wifi waitwaitFor WiFiState 4startDriver OpenWeatherMap// owm_setup lat long APIkeyowm_setup 40.7128 -74.0060 d6fae53c4278ffb3fe4c17c23fc6a7c6 setChannelType 6 Temperature_div10setChannelType 7 HumiditysetChannelType 8 Pressure_div100// owm_channels temperature humidity pressureowm_channels 6 7 8// this will send a HTTP GET onceowm_requestsetChannel 30 0again:addChannel 30 1echo Hello $CH30delay_s 1goto again";
	CFG_SetShortStartupCommand(loremIpsum3);
	CMD_ExecuteCommand(CFG_GetShortStartupCommand(), 0);

	{
		SIM_ShutdownOBK();
		const char *t = "startDriver IR\n"
			"setFlag 1 1\n"
			"addEventHandler2 IR_NEC 0xC7EA 0x17 backlog IRSend NEC 0x0 0xA8; IRSend NEC 0x0 0xA8; IRSend NEC 0x0 0xA8\n"
			"addEventHandler2 IR_NEC 0xC7EA 0xF backlog IRSend NEC 0x0 0x8C; IRSend NEC 0x0 0x8C; IRSend NEC 0x0 0x8C; IRSend NEC 0x0 0x8C; IRSend NEC 0x0 0x8C\n"
			"addEventHandler2 IR_NEC 0xC7EA 0x10 backlog IRSend NEC 0x0 0x9C; IRSend NEC 0x0 0x9C; IRSend NEC 0x0 0x9C; IRSend NEC 0x0 0x9C\n";

		CFG_SetShortStartupCommand(t);
		CFG_Save_IfThereArePendingChanges();
		// NOTE: THIS WILL RUN STARTUP COMMAND!!!!
		SIM_StartOBK(0);

		printf("TEST 0x17: %s\n",
			EventHandlers_GetHandlerCommand2(CMD_EVENT_IR_NEC, 0xC7EA, 0x17));
		printf("TEST 0xF %s\n",
			EventHandlers_GetHandlerCommand2(CMD_EVENT_IR_NEC, 0xC7EA, 0xF));
		printf("TEST 0x10 %s\n",
			EventHandlers_GetHandlerCommand2(CMD_EVENT_IR_NEC, 0xC7EA, 0x10));
		SELFTEST_ASSERT(1 == EventHandlers_FireEvent2(CMD_EVENT_IR_NEC, 0xC7EA, 0x17));
		SELFTEST_ASSERT(1 == EventHandlers_FireEvent2(CMD_EVENT_IR_NEC, 0xC7EA, 0xF));
		SELFTEST_ASSERT(1 == EventHandlers_FireEvent2(CMD_EVENT_IR_NEC, 0xC7EA, 0x10));

		SELFTEST_ASSERT(1 == EventHandlers_FireEvent3(CMD_EVENT_IR_NEC, 0xC7EA, 0x17, 0));
		SELFTEST_ASSERT(1 == EventHandlers_FireEvent3(CMD_EVENT_IR_NEC, 0xC7EA, 0xF, 0));
		SELFTEST_ASSERT(1 == EventHandlers_FireEvent3(CMD_EVENT_IR_NEC, 0xC7EA, 0x10, 0));
	}
	{
		SIM_ShutdownOBK();
		const char *t = "startDriver IR\n"
			"setFlag 1 1\n"
			"addEventHandler2 IR_NEC 0xC7EA 0x17 backlog IRSend NEC 0x0 0xA8; IRSend NEC 0x0 0xA8; IRSend NEC 0x0 0xA8\n"
			"addEventHandler2 IR_NEC 0xC7EA 0xF backlog IRSend NEC 0x0 0x8C; IRSend NEC 0x0 0x8C; IRSend NEC 0x0 0x8C; IRSend NEC 0x0 0x8C; IRSend NEC 0x0 0x8C\n"
			"addEventHandler2 IR_NEC 0xC7EA 0x10 backlog IRSend NEC 0x0 0x9C; IRSend NEC 0x0 0x9C; IRSend NEC 0x0 0x9C; IRSend NEC 0x0 0x9C\n"
			"addEventHandler2 IR_NEC 0xC7EA 0x11 backlog IRSend NEC 0x0 0x9D; IRSend NEC 0x0 0x9D; IRSend NEC 0x0 0x9D; IRSend NEC 0x0 0x9D\n";

		CFG_SetShortStartupCommand(t);
		CFG_Save_IfThereArePendingChanges();
		// NOTE: THIS WILL RUN STARTUP COMMAND!!!!
		SIM_StartOBK(0);

		printf("TEST 0x17: %s\n",
			EventHandlers_GetHandlerCommand2(CMD_EVENT_IR_NEC, 0xC7EA, 0x17));
		printf("TEST 0xF %s\n",
			EventHandlers_GetHandlerCommand2(CMD_EVENT_IR_NEC, 0xC7EA, 0xF));
		printf("TEST 0x10 %s\n",
			EventHandlers_GetHandlerCommand2(CMD_EVENT_IR_NEC, 0xC7EA, 0x10));
		printf("TEST 0x11 %s\n",
			EventHandlers_GetHandlerCommand2(CMD_EVENT_IR_NEC, 0xC7EA, 0x11));
		SELFTEST_ASSERT(1 == EventHandlers_FireEvent2(CMD_EVENT_IR_NEC, 0xC7EA, 0x17));
		SELFTEST_ASSERT(1 == EventHandlers_FireEvent2(CMD_EVENT_IR_NEC, 0xC7EA, 0xF));
		SELFTEST_ASSERT(1 == EventHandlers_FireEvent2(CMD_EVENT_IR_NEC, 0xC7EA, 0x10));
		SELFTEST_ASSERT(1 == EventHandlers_FireEvent2(CMD_EVENT_IR_NEC, 0xC7EA, 0x11));

	}

	{
		SIM_ShutdownOBK();
		const char *t = "startDriver IR\n"
			"setFlag 1 1\n"
			"addEventHandler2 IR_NEC 0xC7EA 0x17 backlog IRSend NEC 0x0 0xA8; delay_ms 50; IRSend NEC 0x0 0xA8; IRSend NEC 0x0 0xA8; addChannel 1 1\n"
			"addEventHandler2 IR_NEC 0xC7EA 0xF backlog IRSend NEC 0x0 0x8C; delay_ms 50; IRSend NEC 0x0 0x8C; IRSend NEC 0x0 0x8C; IRSend NEC 0x0 0x8C; IRSend NEC 0x0 0x8C; addChannel 2 1\n"
			"addEventHandler2 IR_NEC 0xC7EA 0x10 backlog IRSend NEC 0x0 0x9C; delay_ms 50; IRSend NEC 0x0 0x9C; IRSend NEC 0x0 0x9C; IRSend NEC 0x0 0x9C; addChannel 3 1\n"
			"addEventHandler2 IR_NEC 0xC7EA 0x11 backlog IRSend NEC 0x0 0x9D; delay_ms 50; IRSend NEC 0x0 0x9D; IRSend NEC 0x0 0x9D; IRSend NEC 0x0 0x9D; addChannel 4 1\n";

		CFG_SetShortStartupCommand(t);
		CFG_Save_IfThereArePendingChanges();
		// NOTE: THIS WILL RUN STARTUP COMMAND!!!!
		SIM_StartOBK(0);

		CMD_ExecuteCommand("setChannel 1 0", 0);
		SELFTEST_ASSERT_CHANNEL(1, 0);
		CMD_ExecuteCommand("setChannel 2 0", 0);
		SELFTEST_ASSERT_CHANNEL(2, 0);
		CMD_ExecuteCommand("setChannel 3 0", 0);
		SELFTEST_ASSERT_CHANNEL(3, 0);
		CMD_ExecuteCommand("setChannel 4 0", 0);
		SELFTEST_ASSERT_CHANNEL(4, 0);

		printf("TEST 0x17: %s\n",
			EventHandlers_GetHandlerCommand2(CMD_EVENT_IR_NEC, 0xC7EA, 0x17));
		printf("TEST 0xF %s\n",
			EventHandlers_GetHandlerCommand2(CMD_EVENT_IR_NEC, 0xC7EA, 0xF));
		printf("TEST 0x10 %s\n",
			EventHandlers_GetHandlerCommand2(CMD_EVENT_IR_NEC, 0xC7EA, 0x10));
		printf("TEST 0x11 %s\n",
			EventHandlers_GetHandlerCommand2(CMD_EVENT_IR_NEC, 0xC7EA, 0x11));
		SELFTEST_ASSERT(1 == EventHandlers_FireEvent2(CMD_EVENT_IR_NEC, 0xC7EA, 0x17));
		SELFTEST_ASSERT(1 == EventHandlers_FireEvent2(CMD_EVENT_IR_NEC, 0xC7EA, 0xF));
		SELFTEST_ASSERT(1 == EventHandlers_FireEvent2(CMD_EVENT_IR_NEC, 0xC7EA, 0x10));
		SELFTEST_ASSERT(1 == EventHandlers_FireEvent2(CMD_EVENT_IR_NEC, 0xC7EA, 0x11));
		
		// didn't get past delay_ms 50 yet
		SELFTEST_ASSERT_CHANNEL(1, 0);
		SELFTEST_ASSERT_CHANNEL(2, 0);
		SELFTEST_ASSERT_CHANNEL(3, 0);
		SELFTEST_ASSERT_CHANNEL(4, 0);
		// get past delay_ms 50
		Sim_RunMiliseconds(100, false);
		SELFTEST_ASSERT_CHANNEL(1, 1);
		SELFTEST_ASSERT_CHANNEL(2, 1);
		SELFTEST_ASSERT_CHANNEL(3, 1);
		SELFTEST_ASSERT_CHANNEL(4, 1);
	}

}


#endif
