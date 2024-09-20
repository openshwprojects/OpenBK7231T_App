#ifdef WINDOWS

#include "selftest_local.h"

static int g_selfTestErrors = 0;
int g_selfTestsMode = 0;

void SelfTest_Failed(const char *file, const char *function, int line, const char *exp) {
	printf("ERROR: SelfTest failed for %s\n", exp);
	printf("Check %s - %s - line %i\n", file, function, line);
	g_selfTestErrors++;

	system("pause");
}
int SelfTest_GetNumErrors() {
	return g_selfTestErrors;
}

#endif
