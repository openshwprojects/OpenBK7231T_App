#ifdef WINDOWS

#include "selftest_local.h"

void Test_LFS() {
	char buffer[64];
	
	// reset whole device
	SIM_ClearOBK(0);
	CMD_ExecuteCommand("lfs_format", 0);

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

	// check file commands
	CMD_ExecuteCommand("lfs_write command_file_1.txt MY_FILE_CONTENT", 0);
	// get this file 
	Test_FakeHTTPClientPacket_GET("api/lfs/command_file_1.txt");
	SELFTEST_ASSERT_HTML_REPLY("MY_FILE_CONTENT");

	// check file commands
	CMD_ExecuteCommand("lfs_append command_file_1.txt _ADDON", 0);
	// get this file 
	Test_FakeHTTPClientPacket_GET("api/lfs/command_file_1.txt");
	SELFTEST_ASSERT_HTML_REPLY("MY_FILE_CONTENT_ADDON");


	// check file commands
	CMD_ExecuteCommand("lfs_write command_file_2.txt TEST1", 0);
	// get this file 
	Test_FakeHTTPClientPacket_GET("api/lfs/command_file_2.txt");
	SELFTEST_ASSERT_HTML_REPLY("TEST1");

	// check file commands
	CMD_ExecuteCommand("lfs_write command_file_2.txt ABBA", 0);
	// get this file 
	Test_FakeHTTPClientPacket_GET("api/lfs/command_file_2.txt");
	SELFTEST_ASSERT_HTML_REPLY("ABBA");

	// check file commands
	CMD_ExecuteCommand("lfs_write command_file_2.txt XY", 0);
	// get this file 
	Test_FakeHTTPClientPacket_GET("api/lfs/command_file_2.txt");
	SELFTEST_ASSERT_HTML_REPLY("XY");

	// check file commands
	CMD_ExecuteCommand("lfs_append command_file_2.txt ZW", 0);
	// get this file 
	Test_FakeHTTPClientPacket_GET("api/lfs/command_file_2.txt");
	SELFTEST_ASSERT_HTML_REPLY("XYZW");

	// check file commands
	CMD_ExecuteCommand("lfs_remove command_file_2.txt", 0);
	// get this file 
	Test_FakeHTTPClientPacket_GET("api/lfs/command_file_2.txt");
	SELFTEST_ASSERT_HTML_REPLY("{\"fname\":\"command_file_2.txt\",\"error\":-2}");


	// check file commands
	CMD_ExecuteCommand("lfs_append command_file_2.txt this string has spaces really", 0);
	// get this file 
	Test_FakeHTTPClientPacket_GET("api/lfs/command_file_2.txt");
	SELFTEST_ASSERT_HTML_REPLY("this string has spaces really");
	CMD_ExecuteCommand("lfs_append command_file_2.txt !!", 0);
	Test_FakeHTTPClientPacket_GET("api/lfs/command_file_2.txt");
	SELFTEST_ASSERT_HTML_REPLY("this string has spaces really!!");

	// check file commands
	CMD_ExecuteCommand("lfs_append numbers.txt value is ", 0);
	// get this file 
	Test_FakeHTTPClientPacket_GET("api/lfs/numbers.txt");
	SELFTEST_ASSERT_HTML_REPLY("value is ");
	CMD_ExecuteCommand("SetChannel 15 2023", 0);
	CMD_ExecuteCommand("lfs_appendInt numbers.txt $CH15", 0);
	Test_FakeHTTPClientPacket_GET("api/lfs/numbers.txt");
	SELFTEST_ASSERT_HTML_REPLY("value is 2023");
	CMD_ExecuteCommand("lfs_append numbers.txt , and ", 0);
	Test_FakeHTTPClientPacket_GET("api/lfs/numbers.txt");
	SELFTEST_ASSERT_HTML_REPLY("value is 2023, and ");
	CMD_ExecuteCommand("lfs_appendInt numbers.txt 15+16", 0);
	Test_FakeHTTPClientPacket_GET("api/lfs/numbers.txt");
	SELFTEST_ASSERT_HTML_REPLY("value is 2023, and 31");
}

#endif
