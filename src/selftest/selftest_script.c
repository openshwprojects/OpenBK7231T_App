#ifdef WINDOWS

#include "selftest_local.h".

const char *demo_loop_1 =
"setChannel 10 0\r\n"
"again:\r\n"
"    addChannel 10 1\r\n"
"    if $CH10<3 then goto again\r\n"
"    setChannel 11 234\r\n";

void Test_Scripting() {
	char buffer[64];
	
	// reset whole device
	CMD_ExecuteCommand("clearAll", 0);
	CMD_ExecuteCommand("lfsformat", 0);

	// put file in LittleFS
	Test_FakeHTTPClientPacket_POST("api/lfs/demo_loop_1.txt", demo_loop_1);
	// get this file 
	Test_FakeHTTPClientPacket_GET("api/lfs/demo_loop_1.txt");
	SELFTEST_ASSERT_HTML_REPLY(demo_loop_1);

	CMD_ExecuteCommand("startScript demo_loop_1.txt", 0);
	SELFTEST_ASSERT_INTEGER(CMD_GetCountActiveScriptThreads(), 1);
	// should execute and quit within few frames
	// (we have a limit of scripting executed per frame)
	Sim_RunFrames(15);
	SELFTEST_ASSERT_INTEGER(CMD_GetCountActiveScriptThreads(), 0);
	SELFTEST_ASSERT_CHANNEL(10, 3);
	SELFTEST_ASSERT_CHANNEL(11, 234);
	//system("pause");
}

#endif
