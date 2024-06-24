#ifdef WINDOWS

#include "selftest_local.h"
#include "../cmnds/cmd_newEnums.h"


void Test_Enums() {
	char tmp[1024];

	// reset whole device
	SIM_ClearOBK(0);

	SELFTEST_ASSERT(g_enums == 0);

	CMD_ExecuteCommand("SetChannelEnum 4 1:Ok 0:Bad", 0);
	SELFTEST_ASSERT(g_enums);
	for (int i = 0; i < CHANNEL_MAX; i++) {
		if (i != 4) {
			SELFTEST_ASSERT(!g_enums[i]);
		}
	}
	SELFTEST_ASSERT(g_enums[4]);

	SELFTEST_ASSERT(g_enums[4]->numOptions == 2);
	SELFTEST_ASSERT(g_enums[4]->options[0].value == 1);
	SELFTEST_ASSERT(!strcmp(g_enums[4]->options[0].label,"Ok"));
	SELFTEST_ASSERT(g_enums[4]->options[1].value == 0);
	SELFTEST_ASSERT(!strcmp(g_enums[4]->options[1].label, "Bad"));

	CMD_FormatEnumTemplate(g_enums[4], tmp, sizeof(tmp));

	CMD_ExecuteCommand("SetChannelEnum 14 1:One 0:Zero 3:Three", 0);
	SELFTEST_ASSERT(g_enums);
	SELFTEST_ASSERT(g_enums[14]);
	SELFTEST_ASSERT(!g_enums[13]);
	SELFTEST_ASSERT(!g_enums[15]);

	SELFTEST_ASSERT(g_enums[14]->numOptions == 3);
	SELFTEST_ASSERT(g_enums[14]->options[0].value == 1);
	SELFTEST_ASSERT(!strcmp(g_enums[14]->options[0].label, "One"));
	SELFTEST_ASSERT(g_enums[14]->options[1].value == 0);
	SELFTEST_ASSERT(!strcmp(g_enums[14]->options[1].label, "Zero"));
	SELFTEST_ASSERT(g_enums[14]->options[2].value == 3);
	SELFTEST_ASSERT(!strcmp(g_enums[14]->options[2].label, "Three"));
}


#endif
