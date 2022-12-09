#ifdef WINDOWS

#include "selftest_local.h".

void Test_LFS() {
	char buffer[64];
	
	// reset whole device
	SIM_ClearOBK();
	CMD_ExecuteCommand("lfsformat", 0);

	// send file content as POST to REST interface
	Test_FakeHTTPClientPacket_POST("api/lfs/unitTestFile.txt", "This is a sample file made by unit test.");
	// get this file 
	Test_FakeHTTPClientPacket_GET("api/lfs/unitTestFile.txt");
	SELFTEST_ASSERT_HTML_REPLY("This is a sample file made by unit test.");

	// repeat this test with a random integer to make sure we're not somehow getting biased results from previous session
	sprintf(buffer, "Here are some random numbers %i %i %i", (rand() % 10), (rand() % 100), (rand() % 200));
	// send file content as POST to REST interface
	Test_FakeHTTPClientPacket_POST("api/lfs/unitTestFile.txt", buffer);
	// get this file 
	Test_FakeHTTPClientPacket_GET("api/lfs/unitTestFile.txt");
	SELFTEST_ASSERT_HTML_REPLY(buffer);

	// check to see if we can execute a file from within LFS
	Test_FakeHTTPClientPacket_POST("api/lfs/script1.txt", "setChannel 0 123");
	// exec will execute a script file in-place, all commands at once
	CMD_ExecuteCommand("exec script1.txt", 0);
	// is channel changed?
	SELFTEST_ASSERT_CHANNEL(0, 123);


	// check to see if we can execute a file from within LFS
	Test_FakeHTTPClientPacket_POST("api/lfs/script2.txt", "addChannel 0 102");
	// exec will execute a script file in-place, all commands at once
	CMD_ExecuteCommand("exec script2.txt", 0);
	// is channel changed?
	SELFTEST_ASSERT_CHANNEL(0, 225);

	SELFTEST_ASSERT_CHANNEL(10, 0);
	// check to see if we can execute a file from within LFS
	Test_FakeHTTPClientPacket_POST("api/lfs/script3.txt", "addChannel 10 2");
	// exec will execute a script file in-place, all commands at once
	CMD_ExecuteCommand("exec script3.txt", 0);
	SELFTEST_ASSERT_CHANNEL(10, 2);
	// see if we can stack several execs within backlog
	CMD_ExecuteCommand("backlog exec script3.txt; exec script3.txt; exec script3.txt; exec script3.txt; exec script3.txt", 0);
	SELFTEST_ASSERT_CHANNEL(10, 12);
	// see if we can alias it. It will add in total 10 value to ch 10
	CMD_ExecuteCommand("alias myexecs backlog exec script3.txt; exec script3.txt; exec script3.txt; exec script3.txt; exec script3.txt", 0);
	SELFTEST_ASSERT_CHANNEL(10, 12);
	// run command by alias
	CMD_ExecuteCommand("myexecs", 0);
	SELFTEST_ASSERT_CHANNEL(10, 22);
	// 3*10 = 30
	CMD_ExecuteCommand("alias myexecs_3 backlog myexecs; myexecs; myexecs", 0);
	CMD_ExecuteCommand("myexecs_3", 0);
	// should be increased by 30
	SELFTEST_ASSERT_CHANNEL(10, 52);
	CMD_ExecuteCommand("myexecs_3", 0);
	// should be increased by 30
	SELFTEST_ASSERT_CHANNEL(10, 82);

	//system("pause");
}

#endif
