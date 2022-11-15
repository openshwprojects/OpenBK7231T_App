#ifdef WINDOWS

#include "selftest_local.h"

void SelfTest_Failed(const char *file, const char *function, int line, const char *exp) {
	printf("SelfTest failed for %s\n", exp);
	printf("Check %s - %s - line %i\n", file, function, line);
	system("pause");
}


#endif
