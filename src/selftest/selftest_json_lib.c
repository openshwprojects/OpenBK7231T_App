#ifdef WINDOWS

#include "selftest_local.h"
#include "../cJSON/cJSON.h"

void Test_JSON_Lib() {
	int i;
	cJSON* root;
	cJSON* stats;
	char *msg;
	float dailyStats[4] = { 00000095.44071197,00000171.84954833,00000181.58737182,00000331.35061645 };

	root = cJSON_CreateObject();
	{
		stats = cJSON_CreateArray();
		for (i = 0; i < 4; i++)
		{
			cJSON_AddItemToArray(stats, cJSON_CreateNumber(dailyStats[i]));
		}
		cJSON_AddItemToObject(root, "consumption_daily", stats);
	}

	msg = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);

	free(msg);
}


#endif
