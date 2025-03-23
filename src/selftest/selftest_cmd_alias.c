#ifdef WINDOWS

#include "selftest_local.h"

void Test_Commands_Alias_Generic() {
	// reset whole device
	SIM_ClearOBK(0);

	CMD_ExecuteCommand("alias test1 addChannel 2 10", 0);
	SELFTEST_ASSERT_CHANNEL(2, 0);
	CMD_ExecuteCommand("test1", 0);
	SELFTEST_ASSERT_CHANNEL(2, 10);
	CMD_ExecuteCommand("test1", 0);
	SELFTEST_ASSERT_CHANNEL(2, 20);
	CMD_ExecuteCommand("test1", 0);
	SELFTEST_ASSERT_CHANNEL(2, 30);

	CMD_ExecuteCommand("alias test2 addChannel 3 15", 0);
	CMD_ExecuteCommand("test2", 0);
	SELFTEST_ASSERT_CHANNEL(3, 15);

	// recursive alias...
	CMD_ExecuteCommand("alias test3 if $CH10 then test1 else test2", 0);
	CMD_ExecuteCommand("setChannel 10 1", 0);
	// this should run test1 - add 10 to CH2
	SELFTEST_ASSERT_CHANNEL(2, 30);
	CMD_ExecuteCommand("test3", 0);
	SELFTEST_ASSERT_CHANNEL(2, 40);
	CMD_ExecuteCommand("test3", 0);
	SELFTEST_ASSERT_CHANNEL(2, 50);
	CMD_ExecuteCommand("setChannel 10 0", 0);
	SELFTEST_ASSERT_CHANNEL(3, 15);
	// this should run test2 - add 15 to CH3
	CMD_ExecuteCommand("test3", 0);
	SELFTEST_ASSERT_CHANNEL(3, 30);
	SELFTEST_ASSERT_CHANNEL(2, 50);
	// this should run test2 - add 15 to CH3
	CMD_ExecuteCommand("test3", 0);
	SELFTEST_ASSERT_CHANNEL(3, 45);
	SELFTEST_ASSERT_CHANNEL(2, 50);
	// this should run test2 - add 15 to CH3
	CMD_ExecuteCommand("test3", 0);
	SELFTEST_ASSERT_CHANNEL(3, 60);
	SELFTEST_ASSERT_CHANNEL(2, 50);

	CMD_ExecuteCommand("alias test4 backlog setChannel 4 11; addChannel 4 9; setChannel 5 50", 0);
	CMD_ExecuteCommand("test4", 0);
	SELFTEST_ASSERT_CHANNEL(4, (11+9));
	SELFTEST_ASSERT_CHANNEL(5, 50);

	CMD_ExecuteCommand("alias test5 backlog setChannel 4 11; addChannel 4 9; \
		setChannel 5 50; addChannel 4 8; addChannel 4 7; addChannel 4 6", 0);
	CMD_ExecuteCommand("test5", 0);
	SELFTEST_ASSERT_CHANNEL(4, (11 + 9 + 8 + 7 + 6));
	SELFTEST_ASSERT_CHANNEL(5, 50);

	CMD_ExecuteCommand("alias test6 backlog setChannel 4 11; addChannel 4 9; \
		setChannel 5 50; addChannel 4 8; addChannel 4 7; addChannel 4 6; \
		addChannel 4 5; addChannel 4 4; addChannel 4 3; \
		addChannel 4 2; addChannel 4 1; addChannel 4 100", 0);
	CMD_ExecuteCommand("test6", 0);
	SELFTEST_ASSERT_CHANNEL(4, (11 + 9 + 8 + 7 + 6 + 5 + 4 + 3 + 2 + 1 + 100));
	SELFTEST_ASSERT_CHANNEL(5, 50);

	CMD_ExecuteCommand("alias test7 backlog setChannel 4 11; setChannel 5 50;  \
		addChannel 4 9; addChannel 4 8; addChannel 4 7; addChannel 4 6; \
		addChannel 4 5; addChannel 4 4; addChannel 4 3; \
		addChannel 4 2; addChannel 4 1; addChannel 4 100; \
		addChannel 4 9; addChannel 4 8; addChannel 4 7; addChannel 4 6; \
		addChannel 4 5; addChannel 4 4; addChannel 4 3; \
		addChannel 4 2; addChannel 4 1; addChannel 4 100; \
		addChannel 4 9; addChannel 4 8; addChannel 4 7; addChannel 4 6; \
		addChannel 4 5; addChannel 4 4; addChannel 4 3; \
		addChannel 4 2; addChannel 4 1; addChannel 4 100; \
		addChannel 4 9; addChannel 4 8; addChannel 4 7; addChannel 4 6; \
		addChannel 4 5; addChannel 4 4; addChannel 4 3; \
		addChannel 4 2; addChannel 4 1; addChannel 4 100; \
		addChannel 4 9; addChannel 4 8; addChannel 4 7; addChannel 4 6; \
		addChannel 4 5; addChannel 4 4; addChannel 4 3; \
		addChannel 4 2; addChannel 4 1; addChannel 4 100", 0);
	CMD_ExecuteCommand("test7", 0);
	SELFTEST_ASSERT_CHANNEL(4, 11 + 5*(9 + 8 + 7 + 6 + 5 + 4 + 3 + 2 + 1 + 100));
	SELFTEST_ASSERT_CHANNEL(5, 50);


	CMD_ExecuteCommand("alias test8 backlog setChannel 4 11; setChannel 5 50;  \
		addChannel 4 9; addChannel 4 8; addChannel 4 7; addChannel 4 6; \
		addChannel 4 5; addChannel 4 4; addChannel 4 3; \
		addChannel 4 2; addChannel 4 1; addChannel 4 100; \
		addChannel 4 9; addChannel 4 8; addChannel 4 7; addChannel 4 6; \
		addChannel 4 5; addChannel 4 4; addChannel 4 3; \
		addChannel 4 2; addChannel 4 1; addChannel 4 100; \
		addChannel 4 9; addChannel 4 8; addChannel 4 7; addChannel 4 6; \
		addChannel 4 5; addChannel 4 4; addChannel 4 3; \
		addChannel 4 2; addChannel 4 1; addChannel 4 100; \
		addChannel 4 9; addChannel 4 8; addChannel 4 7; addChannel 4 6; \
		addChannel 4 5; addChannel 4 4; addChannel 4 3; \
		addChannel 4 2; addChannel 4 1; addChannel 4 100; \
		addChannel 4 9; addChannel 4 8; addChannel 4 7; addChannel 4 6; \
		addChannel 4 5; addChannel 4 4; addChannel 4 3; \
		addChannel 4 2; addChannel 4 1; addChannel 4 100;  \
		addChannel 4 9; addChannel 4 8; addChannel 4 7; addChannel 4 6; \
		addChannel 4 5; addChannel 4 4; addChannel 4 3; \
		addChannel 4 2; addChannel 4 1; addChannel 4 100; \
		addChannel 4 9; addChannel 4 8; addChannel 4 7; addChannel 4 6; \
		addChannel 4 5; addChannel 4 4; addChannel 4 3; \
		addChannel 4 2; addChannel 4 1; addChannel 4 100; \
		addChannel 4 9; addChannel 4 8; addChannel 4 7; addChannel 4 6; \
		addChannel 4 5; addChannel 4 4; addChannel 4 3; \
		addChannel 4 2; addChannel 4 1; addChannel 4 100; \
		addChannel 4 9; addChannel 4 8; addChannel 4 7; addChannel 4 6; \
		addChannel 4 5; addChannel 4 4; addChannel 4 3; \
		addChannel 4 2; addChannel 4 1; addChannel 4 100; \
		addChannel 4 9; addChannel 4 8; addChannel 4 7; addChannel 4 6; \
		addChannel 4 5; addChannel 4 4; addChannel 4 3; \
		addChannel 4 2; addChannel 4 1; addChannel 4 100", 0);
	CMD_ExecuteCommand("test8", 0);
	SELFTEST_ASSERT_CHANNEL(4, 11 + 10 * (9 + 8 + 7 + 6 + 5 + 4 + 3 + 2 + 1 + 100));
	SELFTEST_ASSERT_CHANNEL(5, 50);


	CMD_ExecuteCommand("alias test9 backlog setChannel 4 11; setChannel 5 50;  \
		addChannel 4 9; addChannel 4 8; addChannel 4 7; addChannel 4 6; \
		addChannel 4 5; addChannel 4 4; addChannel 4 3; \
		addChannel 4 2; addChannel 4 1; addChannel 4 100; \
		addChannel 4 9; addChannel 4 8; addChannel 4 7; addChannel 4 6; \
		addChannel 4 5; addChannel 4 4; addChannel 4 3; \
		addChannel 4 2; addChannel 4 1; addChannel 4 100; \
		addChannel 4 9; addChannel 4 8; addChannel 4 7; addChannel 4 6; \
		addChannel 4 5; addChannel 4 4; addChannel 4 3; \
		addChannel 4 2; addChannel 4 1; addChannel 4 100; \
		addChannel 4 9; addChannel 4 8; addChannel 4 7; addChannel 4 6; \
		addChannel 4 5; addChannel 4 4; addChannel 4 3; \
		addChannel 4 2; addChannel 4 1; addChannel 4 100; \
		addChannel 4 9; addChannel 4 8; addChannel 4 7; addChannel 4 6; \
		addChannel 4 5; addChannel 4 4; addChannel 4 3; \
		addChannel 4 2; addChannel 4 1; addChannel 4 100;  \
		addChannel 4 9; addChannel 4 8; addChannel 4 7; addChannel 4 6; \
		addChannel 4 5; addChannel 4 4; addChannel 4 3; \
		addChannel 4 2; addChannel 4 1; addChannel 4 100; \
		addChannel 4 9; addChannel 4 8; addChannel 4 7; addChannel 4 6; \
		addChannel 4 5; addChannel 4 4; addChannel 4 3; \
		addChannel 4 2; addChannel 4 1; addChannel 4 100; \
		addChannel 4 9; addChannel 4 8; addChannel 4 7; addChannel 4 6; \
		addChannel 4 5; addChannel 4 4; addChannel 4 3; \
		addChannel 4 2; addChannel 4 1; addChannel 4 100; \
		addChannel 4 9; addChannel 4 8; addChannel 4 7; addChannel 4 6; \
		addChannel 4 5; addChannel 4 4; addChannel 4 3; \
		addChannel 4 2; addChannel 4 1; addChannel 4 100; \
		addChannel 4 9; addChannel 4 8; addChannel 4 7; addChannel 4 6; \
		addChannel 4 5; addChannel 4 4; addChannel 4 3; \
		addChannel 4 2; addChannel 4 1; addChannel 4 100;\
		addChannel 4 9; addChannel 4 8; addChannel 4 7; addChannel 4 6; \
		addChannel 4 5; addChannel 4 4; addChannel 4 3; \
		addChannel 4 2; addChannel 4 1; addChannel 4 100; \
		addChannel 4 9; addChannel 4 8; addChannel 4 7; addChannel 4 6; \
		addChannel 4 5; addChannel 4 4; addChannel 4 3; \
		addChannel 4 2; addChannel 4 1; addChannel 4 100; \
		addChannel 4 9; addChannel 4 8; addChannel 4 7; addChannel 4 6; \
		addChannel 4 5; addChannel 4 4; addChannel 4 3; \
		addChannel 4 2; addChannel 4 1; addChannel 4 100; \
		addChannel 4 9; addChannel 4 8; addChannel 4 7; addChannel 4 6; \
		addChannel 4 5; addChannel 4 4; addChannel 4 3; \
		addChannel 4 2; addChannel 4 1; addChannel 4 100; \
		addChannel 4 9; addChannel 4 8; addChannel 4 7; addChannel 4 6; \
		addChannel 4 5; addChannel 4 4; addChannel 4 3; \
		addChannel 4 2; addChannel 4 1; addChannel 4 100;  \
		addChannel 4 9; addChannel 4 8; addChannel 4 7; addChannel 4 6; \
		addChannel 4 5; addChannel 4 4; addChannel 4 3; \
		addChannel 4 2; addChannel 4 1; addChannel 4 100; \
		addChannel 4 9; addChannel 4 8; addChannel 4 7; addChannel 4 6; \
		addChannel 4 5; addChannel 4 4; addChannel 4 3; \
		addChannel 4 2; addChannel 4 1; addChannel 4 100; \
		addChannel 4 9; addChannel 4 8; addChannel 4 7; addChannel 4 6; \
		addChannel 4 5; addChannel 4 4; addChannel 4 3; \
		addChannel 4 2; addChannel 4 1; addChannel 4 100; \
		addChannel 4 9; addChannel 4 8; addChannel 4 7; addChannel 4 6; \
		addChannel 4 5; addChannel 4 4; addChannel 4 3; \
		addChannel 4 2; addChannel 4 1; addChannel 4 100; \
		addChannel 4 9; addChannel 4 8; addChannel 4 7; addChannel 4 6; \
		addChannel 4 5; addChannel 4 4; addChannel 4 3; \
		addChannel 4 2; addChannel 4 1; addChannel 4 100;", 0);
	CMD_ExecuteCommand("test9", 0);
	SELFTEST_ASSERT_CHANNEL(4, 11 + 20 * (9 + 8 + 7 + 6 + 5 + 4 + 3 + 2 + 1 + 100));
	SELFTEST_ASSERT_CHANNEL(5, 50);
	// this check will fail obviously!
	//SELFTEST_ASSERT_CHANNEL(5, 666);
}
void Test_Commands_Alias_Chain() {
	// reset whole device
	SIM_ClearOBK(0);

	SELFTEST_ASSERT_CHANNEL(1, 0);
	SELFTEST_ASSERT_CHANNEL(2, 0);
	SELFTEST_ASSERT_CHANNEL(3, 0);
	SELFTEST_ASSERT_CHANNEL(4, 0);
	SELFTEST_ASSERT_CHANNEL(5, 0);
	SELFTEST_ASSERT_CHANNEL(6, 0);

	CMD_ExecuteCommand("alias test1 backlog addChannel 1 123", 0);
	CMD_ExecuteCommand("alias test2 backlog addChannel 2 234; test1", 0);
	CMD_ExecuteCommand("alias test3 backlog addChannel 3 333; test2", 0);
	CMD_ExecuteCommand("alias test4 backlog addChannel 4 404; test3", 0);
	CMD_ExecuteCommand("alias test5 backlog addChannel 5 500; test4", 0);
	CMD_ExecuteCommand("alias test6 backlog addChannel 6 666; test5", 0);
	CMD_ExecuteCommand("test6", 0);

	SELFTEST_ASSERT_CHANNEL(1, 123);
	SELFTEST_ASSERT_CHANNEL(2, 234);
	SELFTEST_ASSERT_CHANNEL(3, 333);
	SELFTEST_ASSERT_CHANNEL(4, 404);
	SELFTEST_ASSERT_CHANNEL(5, 500);
	SELFTEST_ASSERT_CHANNEL(6, 666);
}
void Test_Commands_Alias_Chain2() {
	// reset whole device
	SIM_ClearOBK(0);

	SELFTEST_ASSERT_CHANNEL(1, 0);
	SELFTEST_ASSERT_CHANNEL(2, 0);
	SELFTEST_ASSERT_CHANNEL(3, 0);
	SELFTEST_ASSERT_CHANNEL(4, 0);
	SELFTEST_ASSERT_CHANNEL(5, 0);
	SELFTEST_ASSERT_CHANNEL(6, 0);

	CMD_ExecuteCommand("alias test10 backlog addChannel 10 111", 0);

	CMD_ExecuteCommand("alias test1 backlog addChannel 1 123; test10", 0);
	CMD_ExecuteCommand("alias test2 backlog addChannel 2 234; test1", 0);
	CMD_ExecuteCommand("alias test3 backlog addChannel 3 333; test2; test1", 0);
	CMD_ExecuteCommand("alias test4 backlog addChannel 4 404; test3", 0);
	CMD_ExecuteCommand("alias test5 backlog addChannel 5 500; test4; test1; test1", 0);
	CMD_ExecuteCommand("alias test6 backlog addChannel 6 666; test5", 0);
	CMD_ExecuteCommand("test6", 0);

	SELFTEST_ASSERT_CHANNEL(1, 123*4);
	SELFTEST_ASSERT_CHANNEL(2, 234);
	SELFTEST_ASSERT_CHANNEL(3, 333);
	SELFTEST_ASSERT_CHANNEL(4, 404);
	SELFTEST_ASSERT_CHANNEL(5, 500);
	SELFTEST_ASSERT_CHANNEL(6, 666);
	SELFTEST_ASSERT_CHANNEL(10, 4*111);
}

void Test_Commands_Alias() {
	Test_Commands_Alias_Generic();
	Test_Commands_Alias_Chain();
	Test_Commands_Alias_Chain2();
}

#endif
