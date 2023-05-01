#ifdef WINDOWS

#include "selftest_local.h"
#include "../httpserver/hass.h"

void Test_HassDiscovery_Base() {
	char tmp[32];
	
	SELFTEST_ASSERT_STRING(hass_generate_multiplyAndRound_template(2, 1), "{{ float(value)*0.1|round(2) }}");
	SELFTEST_ASSERT_STRING(hass_generate_multiplyAndRound_template(2, 2), "{{ float(value)*0.01|round(2) }}");
	SELFTEST_ASSERT_STRING(hass_generate_multiplyAndRound_template(2, 3), "{{ float(value)*0.001|round(2) }}");
	SELFTEST_ASSERT_STRING(hass_generate_multiplyAndRound_template(2, 4), "{{ float(value)*0.0001|round(2) }}");
	SELFTEST_ASSERT_STRING(hass_generate_multiplyAndRound_template(2, 5), "{{ float(value)*0.00001|round(2) }}");
	SELFTEST_ASSERT_STRING(hass_generate_multiplyAndRound_template(1, 0), "{{ float(value)*1|round(1) }}");
	SELFTEST_ASSERT_STRING(hass_generate_multiplyAndRound_template(2, 0), "{{ float(value)*1|round(2) }}");
	SELFTEST_ASSERT_STRING(hass_generate_multiplyAndRound_template(3, 0), "{{ float(value)*1|round(3) }}");
	SELFTEST_ASSERT_STRING(hass_generate_multiplyAndRound_template(4, 0), "{{ float(value)*1|round(4) }}");
	SELFTEST_ASSERT_STRING(hass_generate_multiplyAndRound_template(1, 5), "{{ float(value)*0.00001|round(1) }}");
	SELFTEST_ASSERT_STRING(hass_generate_multiplyAndRound_template(2, 5), "{{ float(value)*0.00001|round(2) }}");
	SELFTEST_ASSERT_STRING(hass_generate_multiplyAndRound_template(3, 5), "{{ float(value)*0.00001|round(3) }}");
	SELFTEST_ASSERT_STRING(hass_generate_multiplyAndRound_template(4, 5), "{{ float(value)*0.00001|round(4) }}");

	// causes an error
	//SELFTEST_ASSERT_STRING(hass_generate_multiplyAndRound_template(3, 5), "{{ float(value)*0.00001|round(4) }}");

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
