#ifdef WINDOWS

#include "selftest_local.h"
#include "../httpserver/hass.h"

char *OLD_hass_generate_multiplyAndRound_template(int decimalPlacesForRounding, int decimalPointOffset, int divider) {
	static char g_hassBuffer[128];
	char tmp[8];
	int i;

	strcpy(g_hassBuffer, "{{ float(value)*");
	if (decimalPointOffset != 0) {
		strcat(g_hassBuffer, "0.");
		for (i = 1; i < decimalPointOffset; i++) {
			strcat(g_hassBuffer, "0");
		}
	}
	// usually it's 1
	sprintf(tmp, "%i", divider);
	strcat(g_hassBuffer, tmp);
	strcat(g_hassBuffer, "|round(");
	sprintf(tmp, "%i", decimalPlacesForRounding);
	strcat(g_hassBuffer, tmp);
	strcat(g_hassBuffer, ") }}");

	return g_hassBuffer;
}

void Test_HassDiscovery_Base() {
	char tmp[32];
	
	SELFTEST_ASSERT_STRING(OLD_hass_generate_multiplyAndRound_template(2, 1, 1), "{{ float(value)*0.1|round(2) }}");
	SELFTEST_ASSERT_STRING(OLD_hass_generate_multiplyAndRound_template(2, 1, 2), "{{ float(value)*0.2|round(2) }}");
	SELFTEST_ASSERT_STRING(OLD_hass_generate_multiplyAndRound_template(2, 1, 3), "{{ float(value)*0.3|round(2) }}");
	SELFTEST_ASSERT_STRING(OLD_hass_generate_multiplyAndRound_template(2, 1, 4), "{{ float(value)*0.4|round(2) }}");
	SELFTEST_ASSERT_STRING(OLD_hass_generate_multiplyAndRound_template(2, 1, 5), "{{ float(value)*0.5|round(2) }}");
	SELFTEST_ASSERT_STRING(OLD_hass_generate_multiplyAndRound_template(2, 2, 1), "{{ float(value)*0.01|round(2) }}");
	SELFTEST_ASSERT_STRING(OLD_hass_generate_multiplyAndRound_template(2, 3, 1), "{{ float(value)*0.001|round(2) }}");
	SELFTEST_ASSERT_STRING(OLD_hass_generate_multiplyAndRound_template(2, 4, 1), "{{ float(value)*0.0001|round(2) }}");
	SELFTEST_ASSERT_STRING(OLD_hass_generate_multiplyAndRound_template(2, 5, 1), "{{ float(value)*0.00001|round(2) }}");
	SELFTEST_ASSERT_STRING(OLD_hass_generate_multiplyAndRound_template(1, 0, 1), "{{ float(value)*1|round(1) }}");
	SELFTEST_ASSERT_STRING(OLD_hass_generate_multiplyAndRound_template(2, 0, 1), "{{ float(value)*1|round(2) }}");
	SELFTEST_ASSERT_STRING(OLD_hass_generate_multiplyAndRound_template(3, 0, 1), "{{ float(value)*1|round(3) }}");
	SELFTEST_ASSERT_STRING(OLD_hass_generate_multiplyAndRound_template(4, 0, 1), "{{ float(value)*1|round(4) }}");
	SELFTEST_ASSERT_STRING(OLD_hass_generate_multiplyAndRound_template(1, 5, 1), "{{ float(value)*0.00001|round(1) }}");
	SELFTEST_ASSERT_STRING(OLD_hass_generate_multiplyAndRound_template(2, 5, 1), "{{ float(value)*0.00001|round(2) }}");
	SELFTEST_ASSERT_STRING(OLD_hass_generate_multiplyAndRound_template(3, 5, 1), "{{ float(value)*0.00001|round(3) }}");
	SELFTEST_ASSERT_STRING(OLD_hass_generate_multiplyAndRound_template(4, 5, 1), "{{ float(value)*0.00001|round(4) }}");

	// causes an error
	//SELFTEST_ASSERT_STRING(OLD_hass_generate_multiplyAndRound_template(3, 5), "{{ float(value)*0.00001|round(4) }}");

	strcpy(tmp, "123.456789");
	stripDecimalPlaces(tmp, 3);
	SELFTEST_ASSERT_STRING(tmp, "123.456");
	strcpy(tmp, "123.456789");
	stripDecimalPlaces(tmp, 2);
	SELFTEST_ASSERT_STRING(tmp, "123.45");
	stripDecimalPlaces(tmp, 1);
	SELFTEST_ASSERT_STRING(tmp, "123.4");
	stripDecimalPlaces(tmp, 0);
	SELFTEST_ASSERT_STRING(tmp, "123");

}


#endif
