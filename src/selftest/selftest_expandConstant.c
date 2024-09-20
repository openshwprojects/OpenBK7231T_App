#ifdef WINDOWS

#include "selftest_local.h"

void Test_ExpandConstant() {
	char buffer[512];
	char *ptr;
	char smallBuffer[8];

	// reset whole device
	SIM_ClearOBK(0);

	CMD_ExpandConstantsWithinString("Hello", buffer, sizeof(buffer));
	SELFTEST_ASSERT_STRING(buffer, "Hello");


	CHANNEL_Set(1, 123, 0);
	CMD_ExpandConstantsWithinString("$CH1", buffer,sizeof(buffer));
	SELFTEST_ASSERT_STRING(buffer, "123");

	CHANNEL_Set(1, 456, 0);
	CMD_ExpandConstantsWithinString("$CH1", buffer, sizeof(buffer));;
	SELFTEST_ASSERT_STRING(buffer, "456");

	CHANNEL_Set(11, 2022, 0);
	// must be able to tell whether it's $CH11 or a $CH1
	CMD_ExpandConstantsWithinString("$CH11", buffer, sizeof(buffer));
	SELFTEST_ASSERT_STRING(buffer, "2022");

	CMD_ExpandConstantsWithinString("$CH1", buffer, sizeof(buffer));
	SELFTEST_ASSERT_STRING(buffer, "456");

	// must be able to tell whether it's $CH11 or a $CH1 - with a suffix
	CMD_ExpandConstantsWithinString("$CH11ba", buffer, sizeof(buffer));
	SELFTEST_ASSERT_STRING(buffer, "2022ba");

	CMD_ExpandConstantsWithinString("$CH1ba", buffer, sizeof(buffer));
	SELFTEST_ASSERT_STRING(buffer, "456ba");

	// must be able to tell whether it's $CH11 or a $CH1 - with a prefix
	CMD_ExpandConstantsWithinString("ba$CH11", buffer, sizeof(buffer));
	SELFTEST_ASSERT_STRING(buffer, "ba2022");

	CMD_ExpandConstantsWithinString("ba$CH1", buffer, sizeof(buffer));
	SELFTEST_ASSERT_STRING(buffer, "ba456");

	// must be able to tell whether it's $CH11 or a $CH1 - with a prefix and a suffix
	CMD_ExpandConstantsWithinString("ba$CH11ha", buffer, sizeof(buffer));
	SELFTEST_ASSERT_STRING(buffer, "ba2022ha");

	CMD_ExpandConstantsWithinString("ba$CH1ha", buffer, sizeof(buffer));
	SELFTEST_ASSERT_STRING(buffer, "ba456ha");

	CMD_ExpandConstantsWithinString("ba$CH1$CH1ha", buffer, sizeof(buffer));
	SELFTEST_ASSERT_STRING(buffer, "ba456456ha");

	CMD_ExpandConstantsWithinString("$CH1$CH1ha", buffer, sizeof(buffer));
	SELFTEST_ASSERT_STRING(buffer, "456456ha");

	CMD_ExpandConstantsWithinString("$CH1$CH1", buffer, sizeof(buffer));
	SELFTEST_ASSERT_STRING(buffer, "456456");

	// check buffer len truncating
	CMD_ExpandConstantsWithinString("Hello long one!", smallBuffer, sizeof(smallBuffer));
	// Buffer was too short - text truncated!
	SELFTEST_ASSERT_STRING(smallBuffer, "Hello l");


	//CMD_ExpandConstantsWithinString("Hello $CH1", smallBuffer, sizeof(smallBuffer));
	// Buffer was too short - text truncated!
	//SELFTEST_ASSERT_STRING(smallBuffer, "Hello 4");
	// NOTE: it won't work like that because of the sprintf behaviour....

	// repeat above tests with an allocing version
	// must be able to tell whether it's $CH11 or a $CH1 - with a prefix and a suffix
	ptr = CMD_ExpandingStrdup("ba$CH11ha");
	SELFTEST_ASSERT_STRING(ptr, "ba2022ha");
	free(ptr);

	ptr = CMD_ExpandingStrdup("ba$CH1ha");
	SELFTEST_ASSERT_STRING(ptr, "ba456ha");
	free(ptr);

	ptr = CMD_ExpandingStrdup("ba$CH1$CH1ha");
	SELFTEST_ASSERT_STRING(ptr, "ba456456ha");
	free(ptr);

	ptr = CMD_ExpandingStrdup("$CH1$CH1ha");
	SELFTEST_ASSERT_STRING(ptr, "456456ha");
	free(ptr);

	ptr = CMD_ExpandingStrdup("$CH1$CH1");
	SELFTEST_ASSERT_STRING(ptr, "456456");
	free(ptr);

	ptr = CMD_ExpandingStrdup("$CH11ba");
	SELFTEST_ASSERT_STRING(ptr, "2022ba");
	free(ptr);

	ptr = CMD_ExpandingStrdup("$CH1ba");
	SELFTEST_ASSERT_STRING(ptr, "456ba");
	free(ptr);

	// must be able to tell whether it's $CH11 or a $CH1 - with a prefix
	ptr = CMD_ExpandingStrdup("ba$CH11");
	SELFTEST_ASSERT_STRING(ptr, "ba2022");
	free(ptr);

	ptr = CMD_ExpandingStrdup("ba$CH1");
	SELFTEST_ASSERT_STRING(ptr, "ba456");
	free(ptr);

	ptr = CMD_ExpandingStrdup("$CH1+$CH11");
	SELFTEST_ASSERT_STRING(ptr, "456+2022");
	free(ptr);
	//system("pause");

	CFG_SetFlag(OBK_FLAG_HTTP_PINMONITOR, 0);
	ptr = CMD_ExpandingStrdup("$FLAG13");
	SELFTEST_ASSERT_STRING(ptr, "0");
	free(ptr);

	CFG_SetFlag(OBK_FLAG_HTTP_PINMONITOR, 1);
	ptr = CMD_ExpandingStrdup("$FLAG13");
	SELFTEST_ASSERT_STRING(ptr, "1");
	free(ptr);

	CFG_SetFlag(OBK_FLAG_HTTP_PINMONITOR, 0);
	ptr = CMD_ExpandingStrdup("$FLAG13");
	SELFTEST_ASSERT_STRING(ptr, "0");
	free(ptr);

	CFG_SetFlag(OBK_FLAG_HTTP_PINMONITOR, 1);
	ptr = CMD_ExpandingStrdup("$FLAG13");
	SELFTEST_ASSERT_STRING(ptr, "1");
	free(ptr);

	CFG_SetFlag(OBK_FLAG_HTTP_PINMONITOR, 0);
}

#endif
