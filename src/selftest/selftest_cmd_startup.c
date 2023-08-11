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


}


#endif
