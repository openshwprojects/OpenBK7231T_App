#ifdef WINDOWS

#include "selftest_local.h"

static const char *g_scr = "\
\n// main relay\
\nsetChannelType 1 Toggle\
\nsetChannelLabel 1 Relay\
\n// working mode\
\nsetChannelType 2 Toggle\
\nsetChannelLabel 2 Automatic Mode\
\n// received power [W]\
\nsetChannelLabel 3 Power\
\nsetChannelType 3 Power\
\n// min power to enable cooling\
\nsetChannelType 4 TextField\
\nsetChannelLabel 4 Min Power To Cool\
\n\n\
\n// remember channels\
\nsetStartValue 1 -1\
\nsetStartValue 2 -1\
\nsetStartValue 3 -1\
\nsetStartValue 4 -1\
\n\n\
\nagain:\
\n\n\
\nif $CH2==1 then setChannel 1 $CH3>$CH4\
\ndelay_s 0.1\
\n\n\
\ngoto again\
\n";


void Test_Demo_ConditionalRelay() {

	SIM_ClearOBK(0);
	CMD_ExecuteCommand("lfs_format", 0);

	// put file in LittleFS
	Test_FakeHTTPClientPacket_POST("api/lfs/demo.txt", g_scr);
	// get this file 
	Test_FakeHTTPClientPacket_GET("api/lfs/demo.txt");
	SELFTEST_ASSERT_HTML_REPLY(g_scr);

	CMD_ExecuteCommand("startScript demo.txt", 0);
	SELFTEST_ASSERT_INTEGER(CMD_GetCountActiveScriptThreads(), 1);
	// min power - 50W
	CMD_ExecuteCommand("setChannel 4 50", 0);
	for (int tr = 0; tr < 3; tr++) {
		// manual mode
		CMD_ExecuteCommand("setChannel 2 0", 0);
		// test
		CMD_ExecuteCommand("setChannel 1 0", 0);
		for (int j = 0; j < 10; j++) {
			int rel = j % 2;
			CHANNEL_Set(1, rel, 0);
			for (int t = 0; t < 15; t++) {
				SELFTEST_ASSERT_INTEGER(CMD_GetCountActiveScriptThreads(), 1);
				int r = rand() % 100;
				// current power
				CHANNEL_Set(3, r, 0);
				// force update
				SVM_RunThreads(1000);
				SVM_RunThreads(1000);
				// didn't change
				SELFTEST_ASSERT_CHANNEL(1, rel);
				SELFTEST_ASSERT_CHANNEL(3, r);
			}
		}
		// auto mode
		CMD_ExecuteCommand("setChannel 2 1", 0);
		for (int j = 0; j < 10; j++) {
			int rel = j % 2;
			CHANNEL_Set(1, rel, 0);
			for (int t = 0; t < 15; t++) {
				SELFTEST_ASSERT_INTEGER(CMD_GetCountActiveScriptThreads(), 1);
				int r = rand() % 100;
				// current power
				CHANNEL_Set(3, r, 0);
				SVM_RunThreads(1000);
				SVM_RunThreads(1000);
				// is automation working?
				SELFTEST_ASSERT_CHANNEL(1, r > 50);
			}
		}
	}
}


#endif
