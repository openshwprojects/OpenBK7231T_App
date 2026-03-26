#ifdef WINDOWS

#include "selftest_local.h"
#include "../driver/drv_openWeatherMap.h"

const char *owm_sample_reply =
"HTTP/1.1 200 OK\r\n"
"Date: Sat, 09 Nov 2025 12:00:00 GMT\r\n"
"Server: openweathermap.org\r\n"
"Content-Type: application/json; charset=utf-8\r\n"
"Content-Length: 123\r\n"
"Connection: close\r\n"
"\r\n"
"{"
"  \"coord\": {\"lon\": 10.0, \"lat\": 50.0},"
"  \"weather\": [{\"id\":800,\"main\":\"Clear\",\"description\":\"clear sky\",\"icon\":\"01d\"}],"
"  \"base\": \"stations\","
"  \"main\": {"
"      \"temp\": 23.0,"
"      \"pressure\": 998,"
"      \"humidity\": 85,"
"      \"temp_min\": 20.0,"
"      \"temp_max\": 25.0"
"  },"
"  \"visibility\": 10000,"
"  \"wind\": {\"speed\": 3.6,\"deg\": 180},"
"  \"clouds\": {\"all\": 0},"
"  \"dt\": 1604995200,"
"  \"sys\": {\"type\":1,\"id\":1234,\"country\":\"US\",\"sunrise\":1604965200,\"sunset\":1605001200},"
"  \"timezone\": -18000,"
"  \"id\": 123456,"
"  \"name\": \"Test City\","
"  \"cod\": 200"
"}";

const char *reply_no_weather =
"HTTP/1.1 200 OK\r\n\r\n"
"{\"coord\":{\"lon\":10,\"lat\":50},\"main\":{\"temp\":20,\"pressure\":900,\"humidity\":60}}";

const char *owm_rain_reply =
"HTTP/1.1 200 OK\r\n\r\n"
"{"
"  \"coord\": {\"lon\": 11.0, \"lat\": 51.0},"
"  \"weather\": [{\"id\":500,\"main\":\"Rain\",\"description\":\"light rain\",\"icon\":\"10d\"}],"
"  \"main\": {\"temp\": 18.0, \"pressure\": 901, \"humidity\": 70},"
"  \"timezone\": 3600,"
"  \"sys\": {\"sunrise\":1604960000,\"sunset\":1605000000}"
"}";

void Test_OpenWeatherMap_StaleWeatherStringsFollowLatestFullReply();

void Test_OpenWeatherMap() {
	weatherData_t *w;

	// HTTP header + json
	Weather_SetReply(owm_sample_reply);

	w = Weather_GetData();
	SELFTEST_ASSERT_FLOATCOMPARE(w->humidity, 85);
	SELFTEST_ASSERT_FLOATCOMPARE(w->pressure, 998);
	SELFTEST_ASSERT_FLOATCOMPARE(w->temp, 23);
	SELFTEST_ASSERT_STRING(w->main_weather, "Clear");
	SELFTEST_ASSERT_STRING(w->description, "clear sky");
	SELFTEST_ASSERT_FLOATCOMPARE(w->lat, 50);
	SELFTEST_ASSERT_FLOATCOMPARE(w->lon, 10);

	// Test with a response that has no "weather" array (edge case)
	Weather_SetReply(reply_no_weather);
	w = Weather_GetData();
	// Numeric fields from "main" and "coord" must still parse
	SELFTEST_ASSERT_FLOATCOMPARE(w->humidity, 60);
	SELFTEST_ASSERT_FLOATCOMPARE(w->pressure, 900);
	SELFTEST_ASSERT_FLOATCOMPARE(w->temp, 20);
	SELFTEST_ASSERT_FLOATCOMPARE(w->lat, 50);
	SELFTEST_ASSERT_FLOATCOMPARE(w->lon, 10);
	// Current implementation only updates these strings when weather[0] exists,
	// so a reply without a weather array keeps the previous parsed values.
	SELFTEST_ASSERT_STRING(w->main_weather, "Clear");
	SELFTEST_ASSERT_STRING(w->description, "clear sky");

	Test_OpenWeatherMap_StaleWeatherStringsFollowLatestFullReply();
}

void Test_OpenWeatherMap_StaleWeatherStringsFollowLatestFullReply() {
	weatherData_t *w;

	Weather_SetReply(owm_sample_reply);
	w = Weather_GetData();
	SELFTEST_ASSERT_STRING(w->main_weather, "Clear");
	SELFTEST_ASSERT_STRING(w->description, "clear sky");

	Weather_SetReply(owm_rain_reply);
	w = Weather_GetData();
	SELFTEST_ASSERT_FLOATCOMPARE(w->humidity, 70);
	SELFTEST_ASSERT_FLOATCOMPARE(w->pressure, 901);
	SELFTEST_ASSERT_FLOATCOMPARE(w->temp, 18);
	SELFTEST_ASSERT_FLOATCOMPARE(w->lat, 51);
	SELFTEST_ASSERT_FLOATCOMPARE(w->lon, 11);
	SELFTEST_ASSERT_STRING(w->main_weather, "Rain");
	SELFTEST_ASSERT_STRING(w->description, "light rain");

	Weather_SetReply(reply_no_weather);
	w = Weather_GetData();
	SELFTEST_ASSERT_FLOATCOMPARE(w->humidity, 60);
	SELFTEST_ASSERT_FLOATCOMPARE(w->pressure, 900);
	SELFTEST_ASSERT_FLOATCOMPARE(w->temp, 20);
	SELFTEST_ASSERT_FLOATCOMPARE(w->lat, 50);
	SELFTEST_ASSERT_FLOATCOMPARE(w->lon, 10);
	SELFTEST_ASSERT_STRING(w->main_weather, "Rain");
	SELFTEST_ASSERT_STRING(w->description, "light rain");
}



#endif
