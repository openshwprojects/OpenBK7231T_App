#ifdef WINDOWS

#include "selftest_local.h"
#include "../driver/drv_openWeatherMap.h"

const char *sample_reply =
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

void Test_OpenWeatherMap() {

	// HTTP header + json
	Weather_SetReply(sample_reply);

	weatherData_t *w = Weather_GetData();
	SELFTEST_ASSERT_FLOATCOMPARE(w->humidity, 85);
	SELFTEST_ASSERT_FLOATCOMPARE(w->pressure, 998);
	SELFTEST_ASSERT_FLOATCOMPARE(w->temp, 23);
	SELFTEST_ASSERT_STRING(w->main_weather, "Clear");
	SELFTEST_ASSERT_STRING(w->description, "clear sky");
	SELFTEST_ASSERT_FLOATCOMPARE(w->lat, 50);
	SELFTEST_ASSERT_FLOATCOMPARE(w->lon, 10);


}



#endif
