#ifdef WINDOWS

#include "selftest_local.h"

bool SIM_BeginParsingMQTTJSON(const char *topic, bool bPrefixMode) {
	const char *data;
	data = SIM_GetMQTTHistoryString(topic, bPrefixMode);
	if (data == 0)
		return true; //error

	Test_GetJSONValue_Setup(data);

	return false; // ok
}




#endif
