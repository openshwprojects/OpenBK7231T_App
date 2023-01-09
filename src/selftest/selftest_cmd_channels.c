#ifdef WINDOWS

#include "selftest_local.h".

void Test_Commands_Channels() {
	// reset whole device
	SIM_ClearOBK();

	CMD_ExecuteCommand("setChannel 5 12", 0);

	SELFTEST_ASSERT_CHANNEL(4, 0);
	SELFTEST_ASSERT_CHANNEL(5, 12);
	SELFTEST_ASSERT_CHANNEL(6, 0);

	CMD_ExecuteCommand("addChannel 5 1", 0);
	SELFTEST_ASSERT_CHANNEL(5, 13);
	CMD_ExecuteCommand("addChannel 5 -1", 0);
	SELFTEST_ASSERT_CHANNEL(5, 12);

	// this check will fail obviously!
	//SELFTEST_ASSERT_CHANNEL(5, 666);

	CMD_ExecuteCommand("setChannel 15 100", 0);
	SELFTEST_ASSERT_CHANNEL(15, 100);

	// Add value of channel 15 to channel 5.
	// 5 is 12, so 12+100 = 112
	SELFTEST_ASSERT_CHANNEL(5, 12);
	SELFTEST_ASSERT_CHANNEL(15, 100);
	CMD_ExecuteCommand("addChannel 5 $CH15", 0);
	SELFTEST_ASSERT_CHANNEL(5, 112);
	CMD_ExecuteCommand("addChannel 5 $CH15", 0);
	SELFTEST_ASSERT_CHANNEL(5, 212);

	// system should be capable of handling - sign
	CMD_ExecuteCommand("addChannel 5 -$CH15", 0);
	SELFTEST_ASSERT_CHANNEL(5, 112);

	// It can also do simple math expressions
	CMD_ExecuteCommand("setChannel 2 10", 0);
	// $CH2 is now 10
	// 10 times 2 is 20
	// but we subtract here
	// So 112-20 = 92
	CMD_ExecuteCommand("addChannel 5 -$CH2*2", 0);
	SELFTEST_ASSERT_CHANNEL(5, 92);

	// test clamping
	// addChannel can take a max and min value
	// So...
	// let's start with 19, add 91 and see if it gets clamped to 100
	CMD_ExecuteCommand("setChannel 11 19", 0);
	SELFTEST_ASSERT_CHANNEL(11, 19);

	CMD_ExecuteCommand("addChannel 11 91 0 100", 0);
	SELFTEST_ASSERT_CHANNEL(11, 100);

	CMD_ExecuteCommand("addChannel 11 91 0 100", 0);
	SELFTEST_ASSERT_CHANNEL(11, 100);

	// clamp to 200
	CMD_ExecuteCommand("addChannel 11 291 0 200", 0);
	SELFTEST_ASSERT_CHANNEL(11, 200);

	// this check will fail obviously!
	//SELFTEST_ASSERT_CHANNEL(5, 666);


	//
	// Check for long commands support! HTTP stage.
	//
	const char *logCmd = "backlog%20setChannel%200%2012;%20setChannel%201%203;%20setChannel%202%204;%20setChannel%203%2045;%20setChannel%204%205;%20setChannel%205%206;%20setChannel%206%2078;%20setChannel%207%20999;%20setChannel%208%2010;%20setChannel%209%20234;%20setChannel%2010%20567;%20setChannel%2011%20-1;%20setChannel%2012%201212;%20setChannel%2013%20131313";

	const char *logCmd2 = ";setChannel%2014%205678;%20setChannel%2015%2012345%20;%20setChannel%2016%207890;%20setChannel%2017%209898;%20setChannel%2018%208765%20;%20setChannel%2019%201191;%20setChannel%2020%204%20";

	char tmp[8192];
	strcpy(tmp, "cm?cmnd=");
	strcat(tmp, logCmd);
	strcat(tmp, logCmd2);
	Test_FakeHTTPClientPacket_GET(tmp);

	SELFTEST_ASSERT_CHANNEL(0, 12);
	SELFTEST_ASSERT_CHANNEL(1, 3);
	SELFTEST_ASSERT_CHANNEL(2, 4);
	SELFTEST_ASSERT_CHANNEL(3, 45);
	SELFTEST_ASSERT_CHANNEL(4, 5);
	SELFTEST_ASSERT_CHANNEL(5, 6);
	SELFTEST_ASSERT_CHANNEL(6, 78);
	SELFTEST_ASSERT_CHANNEL(7, 999);
	SELFTEST_ASSERT_CHANNEL(8, 10);
	SELFTEST_ASSERT_CHANNEL(9, 234);
	SELFTEST_ASSERT_CHANNEL(10, 567);
	SELFTEST_ASSERT_CHANNEL(11, -1);
	SELFTEST_ASSERT_CHANNEL(12, 1212);
	SELFTEST_ASSERT_CHANNEL(13, 131313);
	SELFTEST_ASSERT_CHANNEL(14, 5678);
	SELFTEST_ASSERT_CHANNEL(15, 12345);
	SELFTEST_ASSERT_CHANNEL(16, 7890);
	SELFTEST_ASSERT_CHANNEL(17, 9898);
	SELFTEST_ASSERT_CHANNEL(18, 8765);
	SELFTEST_ASSERT_CHANNEL(19, 1191);
	SELFTEST_ASSERT_CHANNEL(20, 4);

	for (int i = 0; i < CHANNEL_MAX; i++) {
		sprintf(tmp, "cm?cmnd=setChannel%%20%i%%200", i);
		Test_FakeHTTPClientPacket_GET(tmp);
		SELFTEST_ASSERT_CHANNEL(i, 0);
	}
	const char *mqtt_logCmd = "setChannel 0 12; setChannel 1 3; setChannel 2 4; setChannel 3 45; setChannel 4 5; setChannel 5 6; setChannel 6 78; setChannel 7 999; setChannel 8 10; setChannel 9 234; setChannel 10 567; setChannel 11 -1; setChannel 12 1212; setChannel 13 131313";
	const char *mqtt_logCmd2 = ";setChannel 14 5678; setChannel 15 12345 ; setChannel 16 7890; setChannel 17 9898; setChannel 18 8765 ; setChannel 19 1191; setChannel 20 4 ";

	strcpy(tmp, mqtt_logCmd);
	strcat(tmp, mqtt_logCmd2);
	SIM_SendFakeMQTTAndRunSimFrame_CMND("backlog", tmp);
	//
	// Check for long commands support! MQTT stage.
	//
	SELFTEST_ASSERT_CHANNEL(0, 12);
	SELFTEST_ASSERT_CHANNEL(1, 3);
	SELFTEST_ASSERT_CHANNEL(2, 4);
	SELFTEST_ASSERT_CHANNEL(3, 45);
	SELFTEST_ASSERT_CHANNEL(4, 5);
	SELFTEST_ASSERT_CHANNEL(5, 6);
	SELFTEST_ASSERT_CHANNEL(6, 78);
	SELFTEST_ASSERT_CHANNEL(7, 999);
	SELFTEST_ASSERT_CHANNEL(8, 10);
	SELFTEST_ASSERT_CHANNEL(9, 234);
	SELFTEST_ASSERT_CHANNEL(10, 567);
	SELFTEST_ASSERT_CHANNEL(11, -1);
	SELFTEST_ASSERT_CHANNEL(12, 1212);
	SELFTEST_ASSERT_CHANNEL(13, 131313);
	SELFTEST_ASSERT_CHANNEL(14, 5678);
	SELFTEST_ASSERT_CHANNEL(15, 12345);
	SELFTEST_ASSERT_CHANNEL(16, 7890);
	SELFTEST_ASSERT_CHANNEL(17, 9898);
	SELFTEST_ASSERT_CHANNEL(18, 8765);
	SELFTEST_ASSERT_CHANNEL(19, 1191);
	SELFTEST_ASSERT_CHANNEL(20, 4);

}


#endif
