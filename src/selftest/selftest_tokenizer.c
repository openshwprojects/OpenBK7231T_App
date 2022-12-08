#ifdef WINDOWS

#include "selftest_local.h".

void Test_Tokenizer() {
	// reset whole device
	SIM_ClearOBK();

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


	//system("pause");
}


#endif
