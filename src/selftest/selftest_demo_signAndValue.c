#ifdef WINDOWS

#include "selftest_local.h"


const char *CMD_FindOperator(const char *s, const char *stop, byte *oCode);

void Test_Demo_SignAndValue() {
	// reset whole device
	SIM_ClearOBK(0);


	const char *op;
	byte dummy;
	op = CMD_FindOperator("1*1", 0, &dummy);
	SELFTEST_ASSERT(*op == '*');
	op = CMD_FindOperator("1-1", 0, &dummy);
	SELFTEST_ASSERT(*op == '-');
	op = CMD_FindOperator("-1+1", 0, &dummy);
	SELFTEST_ASSERT(*op == '+');
	op = CMD_FindOperator("-1/1", 0, &dummy);
	SELFTEST_ASSERT(*op == '/');
	op = CMD_FindOperator("1+2*3", 0, &dummy);
	SELFTEST_ASSERT(*op == '+');
	op = CMD_FindOperator("1*2-3", 0, &dummy);
	SELFTEST_ASSERT(*op == '-');
	op = CMD_FindOperator("-1/-1", 0, &dummy);
	SELFTEST_ASSERT(*op == '/');
	op = CMD_FindOperator("-1/-12345", 0, &dummy);
	SELFTEST_ASSERT(*op == '/');
	op = CMD_FindOperator("-1*-12345", 0, &dummy);
	SELFTEST_ASSERT(*op == '*');
	op = CMD_FindOperator("-1==-12345", 0, &dummy);
	SELFTEST_ASSERT(*op == '=');


	Sim_RunMiliseconds(500, false);

	CMD_ExecuteCommand("setChannel 4 0", 0);
	CMD_ExecuteCommand("setChannel 3 14", 0);
	SELFTEST_ASSERT_CHANNEL(3, 14);
	SELFTEST_ASSERT_EXPRESSION("$CH3", 14);
	SELFTEST_ASSERT_EXPRESSION("$CH3*-1", -14);

	// change to OnClick if you want?
	CMD_ExecuteCommand("alias qpositive setChannel 19 $CH3", 0);
	CMD_ExecuteCommand("alias qnegative setChannel 19 $CH3*-1", 0);
	CMD_ExecuteCommand("alias mysetcomb if $CH4==0 then qpositive else qnegative", 0);

	CMD_ExecuteCommand("addEventHandler OnChannelChange 3 mysetcomb", 0);
	CMD_ExecuteCommand("addEventHandler OnChannelChange 4 mysetcomb ", 0);

	CMD_ExecuteCommand("setChannel 3 10", 0);
	SELFTEST_ASSERT_CHANNEL(3, 10);
	SELFTEST_ASSERT_CHANNEL(19, 10);


	CMD_ExecuteCommand("setChannel 3 15", 0);
	SELFTEST_ASSERT_CHANNEL(3, 15);
	SELFTEST_ASSERT_CHANNEL(19, 15);


	CMD_ExecuteCommand("setChannel 4 1", 0);
	SELFTEST_ASSERT_CHANNEL(4, 1);
	SELFTEST_ASSERT_CHANNEL(3, 15);
	SELFTEST_ASSERT_CHANNEL(19, -15);


	CMD_ExecuteCommand("setChannel 4 0", 0);
	SELFTEST_ASSERT_CHANNEL(4, 0);
	SELFTEST_ASSERT_CHANNEL(3, 15);
	SELFTEST_ASSERT_CHANNEL(19, 15);


	CMD_ExecuteCommand("setChannel 4 1", 0);
	SELFTEST_ASSERT_CHANNEL(4, 1);
	SELFTEST_ASSERT_CHANNEL(3, 15);
	SELFTEST_ASSERT_CHANNEL(19, -15);


	CMD_ExecuteCommand("setChannel 3 1234", 0);
	SELFTEST_ASSERT_CHANNEL(4, 1);
	SELFTEST_ASSERT_CHANNEL(3, 1234);
	SELFTEST_ASSERT_CHANNEL(19, -1234);

	CMD_ExecuteCommand("setChannel 4 0", 0);
	CMD_ExecuteCommand("setChannel 3 0", 0);
	CMD_ExecuteCommand("setChannel 19 0", 0);
	SELFTEST_ASSERT_CHANNEL(4, 0);
	SELFTEST_ASSERT_CHANNEL(3, 0);
	SELFTEST_ASSERT_CHANNEL(19, 0);
	CMD_ExecuteCommand("clearAllHandlers", 0);
}


#endif
