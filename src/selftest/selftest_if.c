#ifdef WINDOWS

#include "selftest_local.h"

void Test_Command_If() {
	// reset whole device
	SIM_ClearOBK(0);

	CMD_ExecuteCommand("if 1 then \"setChannel 1 1521\"", 0);
	SELFTEST_ASSERT_CHANNEL(1, 1521);
	CMD_ExecuteCommand("if 0 then \"setChannel 1 9876\"", 0);
	SELFTEST_ASSERT_CHANNEL(1, 1521);
	CMD_ExecuteCommand("if 1 then \"setChannel 1 1500+67\"", 0);
	SELFTEST_ASSERT_CHANNEL(1, 1567);
	CMD_ExecuteCommand("if !1 then \"setChannel 1 3333+1111\"", 0);
	SELFTEST_ASSERT_CHANNEL(1, 1567);
	CMD_ExecuteCommand("if !0 then \"setChannel 1 3333+1111\"", 0);
	SELFTEST_ASSERT_CHANNEL(1, 4444);

	CMD_ExecuteCommand("setChannel 10 0", 0);
	CMD_ExecuteCommand("setChannel 11 1", 0);

	CMD_ExecuteCommand("if $CH11 then \"setChannel 1 4000\"", 0);
	SELFTEST_ASSERT_CHANNEL(1, 4000);

	CMD_ExecuteCommand("if $CH10 then \"setChannel 1 5000\"", 0);
	SELFTEST_ASSERT_CHANNEL(1, 4000);

	CMD_ExecuteCommand("if $CH11>$CH10 then \"setChannel 1 6000\"", 0);
	SELFTEST_ASSERT_CHANNEL(1, 6000);

	// test OR and AND
	CMD_ExecuteCommand("setChannel 20 0", 0);
	CMD_ExecuteCommand("setChannel 21 0", 0);
	CMD_ExecuteCommand("setChannel 23 0", 0);
	SELFTEST_ASSERT_CHANNEL(23, 0);
	CMD_ExecuteCommand("if $CH20&&$CH21 then \"setChannel 23 123\"", 0);
	SELFTEST_ASSERT_CHANNEL(23, 0);
	CMD_ExecuteCommand("if $CH20||$CH21 then \"setChannel 23 123\"", 0);
	SELFTEST_ASSERT_CHANNEL(23, 0);
	CMD_ExecuteCommand("setChannel 20 1", 0);
	CMD_ExecuteCommand("if $CH20||$CH21 then \"setChannel 23 123\"", 0);
	SELFTEST_ASSERT_CHANNEL(23, 123);
	CMD_ExecuteCommand("if $CH20&&$CH21 then \"setChannel 23 234\"", 0);
	SELFTEST_ASSERT_CHANNEL(23, 123);
	CMD_ExecuteCommand("if $CH20&&$CH21 then \"setChannel 23 234\"", 0);
	SELFTEST_ASSERT_CHANNEL(23, 123);
	CMD_ExecuteCommand("setChannel 21 1", 0);
	CMD_ExecuteCommand("if $CH20&&$CH21 then \"setChannel 23 234\"", 0);
	SELFTEST_ASSERT_CHANNEL(23, 234);


	CMD_ExecuteCommand("setChannel 7 0", 0);
	CMD_ExecuteCommand("if $CH7!=0 then \"setChannel 23 8765\"", 0);
	SELFTEST_ASSERT_CHANNEL(23, 234);
	CMD_ExecuteCommand("setChannel 7 1", 0);
	CMD_ExecuteCommand("if $CH7==0 then \"setChannel 23 8765\"", 0);
	SELFTEST_ASSERT_CHANNEL(23, 234);
	CMD_ExecuteCommand("setChannel 7 1", 0);
	CMD_ExecuteCommand("if $CH7==1 then \"setChannel 23 8765\"", 0);
	SELFTEST_ASSERT_CHANNEL(23, 8765);
	CMD_ExecuteCommand("setChannel 7 0", 0);
	CMD_ExecuteCommand("if $CH7==0 then \"setChannel 23 7654\"", 0);
	SELFTEST_ASSERT_CHANNEL(23, 7654);
	CMD_ExecuteCommand("setChannel 23 0", 0);
	SELFTEST_ASSERT_CHANNEL(23, 0);
	CMD_ExecuteCommand("if $CH7==0 then setChannel 23 7654", 0);
	SELFTEST_ASSERT_CHANNEL(23, 7654);


	CMD_ExecuteCommand("setChannel 7 1", 0);
	CMD_ExecuteCommand("if $CH7!=0 then \"setChannel 23 1234\"", 0);
	SELFTEST_ASSERT_CHANNEL(23, 1234);

	CMD_ExecuteCommand("setChannel 23 0", 0);
	SELFTEST_ASSERT_CHANNEL(23, 0);
	CMD_ExecuteCommand("if $CH7!=0 then setChannel 23 1234", 0);
	SELFTEST_ASSERT_CHANNEL(23, 1234);

	CMD_ExecuteCommand("setChannel 23 0", 0);
	CMD_ExecuteCommand("setChannel 24 0", 0);
	CMD_ExecuteCommand("setChannel 25 0", 0);
	SELFTEST_ASSERT_CHANNEL(23, 0);
	SELFTEST_ASSERT_CHANNEL(24, 0);
	SELFTEST_ASSERT_CHANNEL(25, 0);
	CMD_ExecuteCommand("if $CH7!=0 then backlog setChannel 23 1234; setChannel 24 4567; setChannel 25 6789", 0);
	SELFTEST_ASSERT_CHANNEL(23, 1234);
	SELFTEST_ASSERT_CHANNEL(24, 4567);
	SELFTEST_ASSERT_CHANNEL(25, 6789);
	// cause error
	//SELFTEST_ASSERT_CHANNEL(1, 666);
}

void Test_Command_If_Else() {
	// reset whole device
	SIM_ClearOBK(0);

	CMD_ExecuteCommand("if 1 then \"setChannel 1 1111\" else \"setChannel 2 2222\"", 0);
	SELFTEST_ASSERT_CHANNEL(1, 1111);
	SELFTEST_ASSERT_CHANNEL(2, 0);
	CMD_ExecuteCommand("if 0 then \"setChannel 1 3333\" else \"setChannel 2 4444\"", 0);
	SELFTEST_ASSERT_CHANNEL(1, 1111);
	SELFTEST_ASSERT_CHANNEL(2, 4444);
	// CH1 is 1111, it's not larger than ch2 - 4444
	CMD_ExecuteCommand("if $CH1>$CH2 then \"setChannel 3 5555\" else \"setChannel 4 6666\"", 0);
	SELFTEST_ASSERT_CHANNEL(3, 0);
	SELFTEST_ASSERT_CHANNEL(4, 6666);

	SELFTEST_ASSERT_CHANNEL(1, 1111);
	SELFTEST_ASSERT_CHANNEL(2, 4444);

	// they are the same, 4*1111 = 4444
	///CMD_ExecuteCommand("if 4*$CH1>=$CH2 then \"setChannel 3 1234\" else \"setChannel 6 1234\"", 0);
	//SELFTEST_ASSERT_CHANNEL(3, 1234);
	//SELFTEST_ASSERT_CHANNEL(6, 0);
	//SELFTEST_ASSERT_CHANNEL(4, 6666);

	CMD_ExecuteCommand("setChannel 11 1", 0);

	CMD_ExecuteCommand("if $CH11 then \"setChannel 12 1111\" else \"setChannel 13 2222\"", 0);
	SELFTEST_ASSERT_CHANNEL(11, 1);
	SELFTEST_ASSERT_CHANNEL(12, 1111);
	SELFTEST_ASSERT_CHANNEL(13, 0);

	CMD_ExecuteCommand("setChannel 11 0", 0);
	CMD_ExecuteCommand("if $CH11 then \"setChannel 12 3333\" else \"setChannel 13 4444\"", 0);
	SELFTEST_ASSERT_CHANNEL(11, 0);
	SELFTEST_ASSERT_CHANNEL(12, 1111);//keeps old val
	SELFTEST_ASSERT_CHANNEL(13, 4444);

	CMD_ExecuteCommand("setChannel 5 0", 0);
	CMD_ExecuteCommand("addEventHandler OnChannelChange 5 if $CH11 then \"setChannel 12 2024\" else \"setChannel 12 5000\"", 0);
	SELFTEST_ASSERT_CHANNEL(5, 0);
	SELFTEST_ASSERT_CHANNEL(11, 0);
	SELFTEST_ASSERT_CHANNEL(12, 1111);//keeps old val
	SELFTEST_ASSERT_CHANNEL(13, 4444);

	CMD_ExecuteCommand("setChannel 11 1", 0);
	CMD_ExecuteCommand("setChannel 5 1", 0);
	// change will fire if and since CH11 is 1 it will set 12 to 2024
	SELFTEST_ASSERT_CHANNEL(5, 1);
	SELFTEST_ASSERT_CHANNEL(11, 1);
	SELFTEST_ASSERT_CHANNEL(12, 2024);
	SELFTEST_ASSERT_CHANNEL(13, 4444);

	// clear 12 for test
	CMD_ExecuteCommand("setChannel 12 0", 0);
	SELFTEST_ASSERT_CHANNEL(12, 0);

	// change will fire if and since CH11 is 1 it will set 12 to 2024
	CMD_ExecuteCommand("addChannel 5 1", 0);
	SELFTEST_ASSERT_CHANNEL(12, 2024);

	// clear 12 for test
	CMD_ExecuteCommand("setChannel 12 0", 0);
	SELFTEST_ASSERT_CHANNEL(12, 0);
	// set 11 so IF will behave differently
	CMD_ExecuteCommand("setChannel 11 0", 0);
	SELFTEST_ASSERT_CHANNEL(11, 0);
	// 'if' didnt fire yet, so it should still have prev val:
	SELFTEST_ASSERT_CHANNEL(12, 0);
	// change will fire if and since CH11 is 1 it will set 12 to 5000
	CMD_ExecuteCommand("addChannel 5 1", 0);
	SELFTEST_ASSERT_CHANNEL(12, 5000);

	// cause error
	//SELFTEST_ASSERT_CHANNEL(1, 666);

	//system("pause");

}

#endif
