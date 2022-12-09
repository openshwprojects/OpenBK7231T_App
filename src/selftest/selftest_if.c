#ifdef WINDOWS

#include "selftest_local.h".

void Test_Command_If() {
	// reset whole device
	SIM_ClearOBK();

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

	// cause error
	//SELFTEST_ASSERT_CHANNEL(1, 666);

}

void Test_Command_If_Else() {
	// reset whole device
	SIM_ClearOBK();

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

	// cause error
	//SELFTEST_ASSERT_CHANNEL(1, 666);

	//system("pause");

}

#endif
