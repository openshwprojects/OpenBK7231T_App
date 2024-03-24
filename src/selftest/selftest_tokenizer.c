#ifdef WINDOWS

#include "selftest_local.h"

void Test_Tokenizer() {
	// reset whole device
	SIM_ClearOBK(0);

	Tokenizer_TokenizeString("Hello", 0);
	SELFTEST_ASSERT_ARGUMENTS_COUNT(1);
	SELFTEST_ASSERT_ARGUMENT(0, "Hello");

	Tokenizer_TokenizeString("Hello you too!", 0);
	SELFTEST_ASSERT_ARGUMENTS_COUNT(3);
	SELFTEST_ASSERT_ARGUMENT(0, "Hello");
	SELFTEST_ASSERT_ARGUMENT(1, "you");
	SELFTEST_ASSERT_ARGUMENT(2, "too!");
	Tokenizer_TokenizeString("1 2 3 4 5", 0);
	SELFTEST_ASSERT_ARGUMENTS_COUNT(5);
	SELFTEST_ASSERT_ARGUMENT(0, "1");
	SELFTEST_ASSERT_ARGUMENT_INTEGER(0, 1);
	SELFTEST_ASSERT_ARGUMENT(1, "2");
	SELFTEST_ASSERT_ARGUMENT_INTEGER(1, 2);
	SELFTEST_ASSERT_ARGUMENT(2, "3");
	SELFTEST_ASSERT_ARGUMENT_INTEGER(2, 3);
	SELFTEST_ASSERT_ARGUMENT(3, "4");
	SELFTEST_ASSERT_ARGUMENT_INTEGER(3, 4);
	SELFTEST_ASSERT_ARGUMENT(4, "5");
	SELFTEST_ASSERT_ARGUMENT_INTEGER(4, 5);

	// ability to skip multiple whitespaces
	Tokenizer_TokenizeString("   Hello     you    too!   ", 0);
	SELFTEST_ASSERT_ARGUMENTS_COUNT(3);
	SELFTEST_ASSERT_ARGUMENT(0, "Hello");
	SELFTEST_ASSERT_ARGUMENT(1, "you");
	SELFTEST_ASSERT_ARGUMENT(2, "too!");

	// Test the quoted strings behaviour
	Tokenizer_TokenizeString("   Hello     \"you too!\"   ", TOKENIZER_ALLOW_QUOTES);
	SELFTEST_ASSERT_ARGUMENTS_COUNT(2);
	SELFTEST_ASSERT_ARGUMENT(0, "Hello");
	SELFTEST_ASSERT_ARGUMENT(1, "you too!");

	Tokenizer_TokenizeString("   Print     \"you too!\" 15 620 400  ", TOKENIZER_ALLOW_QUOTES);
	SELFTEST_ASSERT_ARGUMENTS_COUNT(5);
	SELFTEST_ASSERT_ARGUMENT(0, "Print");
	SELFTEST_ASSERT_ARGUMENT(1, "you too!");
	SELFTEST_ASSERT_ARGUMENT_INTEGER(2, 15);
	SELFTEST_ASSERT_ARGUMENT_INTEGER(3, 620);
	SELFTEST_ASSERT_ARGUMENT_INTEGER(4, 400);

	Tokenizer_TokenizeString("   4567   ", 0);
	SELFTEST_ASSERT_ARGUMENTS_COUNT(1);
	SELFTEST_ASSERT_ARGUMENT(0, "4567");

	// ability to skip multiple whitespaces and quotes
	Tokenizer_TokenizeString("   \"Hello\"     \"you\"    \"too!\"   ", TOKENIZER_ALLOW_QUOTES);
	SELFTEST_ASSERT_ARGUMENTS_COUNT(3);
	SELFTEST_ASSERT_ARGUMENT(0, "Hello");
	SELFTEST_ASSERT_ARGUMENT(1, "you");
	SELFTEST_ASSERT_ARGUMENT(2, "too!");

	// test empty string
	Tokenizer_TokenizeString("", TOKENIZER_ALLOW_QUOTES);
	SELFTEST_ASSERT_ARGUMENTS_COUNT(0);

	Tokenizer_TokenizeString("\"Hello\" \"you\" \"too!\"", TOKENIZER_ALLOW_QUOTES);
	SELFTEST_ASSERT_ARGUMENTS_COUNT(3);
	SELFTEST_ASSERT_ARGUMENT(0, "Hello");
	SELFTEST_ASSERT_ARGUMENT(1, "you");
	SELFTEST_ASSERT_ARGUMENT(2, "too!");

	// test empty string
	Tokenizer_TokenizeString("  ", 0);
	SELFTEST_ASSERT_ARGUMENTS_COUNT(0);

	// there are TWO spaces
	Tokenizer_TokenizeString("\"Hello\" \"you  too!\"", TOKENIZER_ALLOW_QUOTES);
	SELFTEST_ASSERT_ARGUMENTS_COUNT(2);
	SELFTEST_ASSERT_ARGUMENT(0, "Hello");
	SELFTEST_ASSERT_ARGUMENT(1, "you  too!"); // there are TWO spaces


	// block of code to cause assertion (failed test)
#if 0
	Tokenizer_TokenizeString("   Hello     you    too!   ", 0);
	SELFTEST_ASSERT_ARGUMENTS_COUNT(3);
	SELFTEST_ASSERT_ARGUMENT(0, "Hello");
	// this will trigger an assert
	SELFTEST_ASSERT_ARGUMENT(1, "us");
	SELFTEST_ASSERT_ARGUMENT(2, "too!");

#endif
	// check constant expansion
	CMD_ExecuteCommand("setChannel 1 55",0);
	CMD_ExecuteCommand("setChannel 3 66",0);
	CMD_ExecuteCommand("setChannel 5 77",0);

	Tokenizer_TokenizeString("$CH1 2 $CH3 4 $CH5", 0);
	SELFTEST_ASSERT_ARGUMENTS_COUNT(5);
	SELFTEST_ASSERT_ARGUMENT_INTEGER(0, 55); // $CH1
	SELFTEST_ASSERT_ARGUMENT(1, "2");
	SELFTEST_ASSERT_ARGUMENT_INTEGER(1, 2);
	SELFTEST_ASSERT_ARGUMENT_INTEGER(2, 66); // $CH2
	SELFTEST_ASSERT_ARGUMENT(3, "4");
	SELFTEST_ASSERT_ARGUMENT_INTEGER(3, 4);
	SELFTEST_ASSERT_ARGUMENT_INTEGER(4, 77);// $CH3


	Tokenizer_TokenizeString("SendPOST http://localhost:3000/ 3000 \"application/json\" \"{ \\\"a\\\":123, \\\"b\\\":77 }\" 0",
		TOKENIZER_ALLOW_QUOTES | TOKENIZER_ALLOW_ESCAPING_QUOTATIONS);

	SELFTEST_ASSERT_ARGUMENTS_COUNT(6);
	SELFTEST_ASSERT_ARGUMENT(0, "SendPOST");
	SELFTEST_ASSERT_ARGUMENT(1, "http://localhost:3000/");
	SELFTEST_ASSERT_ARGUMENT_INTEGER(2, 3000);
	SELFTEST_ASSERT_ARGUMENT(3, "application/json");
	SELFTEST_ASSERT_ARGUMENT(4, "{ \"a\":123, \"b\":77 }");
	SELFTEST_ASSERT_ARGUMENT_INTEGER(5, 0);
	

	CMD_ExecuteCommand("setChannel 1 12", 0);
	CMD_ExecuteCommand("setChannel 3 55", 0);
	CMD_ExecuteCommand("setChannel 5 77", 0);

	Tokenizer_TokenizeString("Turn on $CH1:$CH3 level=$CH5 $CH6",
		TOKENIZER_ALTERNATE_EXPAND_AT_START | TOKENIZER_FORCE_SINGLE_ARGUMENT_MODE);

	SELFTEST_ASSERT_ARGUMENT(0, "Turn on 12:55 level=77 0");

	Tokenizer_TokenizeString("TurnOn $CH1:$CH3 level=$CH5 $CH6", TOKENIZER_ALTERNATE_EXPAND_AT_START);

	SELFTEST_ASSERT_ARGUMENT(0, "TurnOn");
	SELFTEST_ASSERT_ARGUMENT(1, "12:55");
	SELFTEST_ASSERT_ARGUMENT(2, "level=77");
	SELFTEST_ASSERT_ARGUMENT(3, "0");
	SELFTEST_ASSERT_STRING(Tokenizer_GetArgFrom(2), "level=$CH5 $CH6")
}

#endif
