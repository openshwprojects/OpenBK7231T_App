#ifdef WINDOWS

#include "selftest_local.h"

void Test_Commands_Channels() {
	// reset whole device
	SIM_ClearOBK(0);

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

	CMD_ExecuteCommand("setChannelType 0 temperature", 0);
	SELFTEST_ASSERT_CHANNELTYPE(0, ChType_Temperature);
	CMD_ExecuteCommand("setChannelType 1 humidity", 0);
	SELFTEST_ASSERT_CHANNELTYPE(1, ChType_Humidity);
	CMD_ExecuteCommand("setChannelType 2 Dimmer1000", 0);
	SELFTEST_ASSERT_CHANNELTYPE(2, ChType_Dimmer1000);
	CMD_ExecuteCommand("setChannelType 3 BatteryLevelPercent", 0);
	SELFTEST_ASSERT_CHANNELTYPE(3, ChType_BatteryLevelPercent);
	CMD_ExecuteCommand("setChannelType 4 Current_div1000", 0);
	SELFTEST_ASSERT_CHANNELTYPE(4, ChType_Current_div1000);
	CMD_ExecuteCommand("setChannelType 5 OpenClosed", 0);
	SELFTEST_ASSERT_CHANNELTYPE(5, ChType_OpenClosed);
	CMD_ExecuteCommand("setChannelType 6 EnergyTotal_kWh_div100", 0);
	SELFTEST_ASSERT_CHANNELTYPE(6, ChType_EnergyTotal_kWh_div100);
	CMD_ExecuteCommand("setChannelType 7 OpenClosed_Inv", 0);
	SELFTEST_ASSERT_CHANNELTYPE(7, ChType_OpenClosed_Inv);

	// Map command
	/*
// linear mapping function --> https://www.arduino.cc/reference/en/language/functions/math/map/
#define MAP(x, in_min, in_max, out_min, out_max) (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
	*/
	// target is channel 0
	CMD_ExecuteCommand("Map 0 5 0 10 0 100", 0);
	SELFTEST_ASSERT_CHANNEL(0, 50);
	CMD_ExecuteCommand("Map 0 7 0 10 0 100", 0);
	SELFTEST_ASSERT_CHANNEL(0, 70);
	CMD_ExecuteCommand("Map 0 10 0 10 0 100", 0);
	SELFTEST_ASSERT_CHANNEL(0, 100);

	CMD_ExecuteCommand("Map 0 5 0 10 1000 2000", 0);
	SELFTEST_ASSERT_CHANNEL(0, 1500);
	CMD_ExecuteCommand("Map 0 7 0 10 1000 2000", 0);
	SELFTEST_ASSERT_CHANNEL(0, 1700);
	CMD_ExecuteCommand("Map 0 10 0 10 1000 2000", 0);
	SELFTEST_ASSERT_CHANNEL(0, 2000);
	CMD_ExecuteCommand("Map 0 9 0 10 1000 2000", 0);
	SELFTEST_ASSERT_CHANNEL(0, 1900);
	CMD_ExecuteCommand("Map 0 9.5 0 10 1000 2000", 0);
	SELFTEST_ASSERT_CHANNEL(0, 1950);
	// is it able to take channel values?
	CMD_ExecuteCommand("setChannel 2 0", 0);
	CMD_ExecuteCommand("setChannel 3 10", 0);
	CMD_ExecuteCommand("setChannel 4 1000", 0);
	CMD_ExecuteCommand("setChannel 5 2000", 0);
	CMD_ExecuteCommand("Map 0 9 $CH2 $CH3 $CH4 $CH5", 0);
	SELFTEST_ASSERT_CHANNEL(0, 1900);
	CMD_ExecuteCommand("Map 0 8 $CH2 $CH3 $CH4 $CH5", 0);
	SELFTEST_ASSERT_CHANNEL(0, 1800);
	CMD_ExecuteCommand("Map 0 8.5 $CH2 $CH3 $CH4 $CH5", 0);
	SELFTEST_ASSERT_CHANNEL(0, 1850);
	CMD_ExecuteCommand("setChannel 6 6", 0);
	CMD_ExecuteCommand("Map 0 $CH6 $CH2 $CH3 $CH4 $CH5", 0);
	SELFTEST_ASSERT_CHANNEL(0, 1600);
	CMD_ExecuteCommand("setChannel 6 5", 0);
	CMD_ExecuteCommand("Map 0 $CH6 $CH2 $CH3 $CH4 $CH5", 0);
	SELFTEST_ASSERT_CHANNEL(0, 1500);
	CMD_ExecuteCommand("setChannel 6 4", 0);
	CMD_ExecuteCommand("Map 0 $CH6 $CH2 $CH3 $CH4 $CH5", 0);
	SELFTEST_ASSERT_CHANNEL(0, 1400);


	CMD_ExecuteCommand("setChannel 10 0", 0);
	SELFTEST_ASSERT_CHANNEL(10, 0);
	CMD_ExecuteCommand("ToggleChannel 10", 0);
	SELFTEST_ASSERT_CHANNEL(10, 1);
	CMD_ExecuteCommand("ToggleChannel 10", 0);
	SELFTEST_ASSERT_CHANNEL(10, 0);
	CMD_ExecuteCommand("ToggleChannel 10", 0);
	SELFTEST_ASSERT_CHANNEL(10, 1);
	CMD_ExecuteCommand("setChannel 10 567", 0);
	SELFTEST_ASSERT_CHANNEL(10, 567);
	CMD_ExecuteCommand("ToggleChannel 10", 0);
	SELFTEST_ASSERT_CHANNEL(10, 0);
	for (int i = 0; i < 100; i++) {
		CMD_ExecuteCommand("ToggleChannel 10", 0);
		SELFTEST_ASSERT_CHANNEL(10, 1);
		CMD_ExecuteCommand("ToggleChannel 10", 0);
		SELFTEST_ASSERT_CHANNEL(10, 0);

	}

	// test backlog
	CMD_ExecuteCommand("backlog setChannel 1 11; setChannel 2 22; setChannel 3 33;", 0);
	SELFTEST_ASSERT_CHANNEL(1, 11);
	SELFTEST_ASSERT_CHANNEL(2, 22);
	SELFTEST_ASSERT_CHANNEL(3, 33);
	CMD_ExecuteCommand("backlog setChannel 1 101 ; setChannel 2 202 ; setChannel 3 303 ;", 0);
	SELFTEST_ASSERT_CHANNEL(1, 101);
	SELFTEST_ASSERT_CHANNEL(2, 202);
	SELFTEST_ASSERT_CHANNEL(3, 303);

	for (int i = 0; i < 5; i++) {
		CMD_ExecuteCommand("setChannel 1 $rand01*5", 0);
		printf("Rand01 times 5 - channel 1 is %i\n", CHANNEL_Get(1));
	}
	for (int i = 0; i < 5; i++) {
		CMD_ExecuteCommand("setChannel 1 $rand", 0);
		printf("Rand - channel 1 is %i\n", CHANNEL_Get(1));
	}

	// cause error
	//SELFTEST_ASSERT_CHANNEL(3, 666);
}


#endif
