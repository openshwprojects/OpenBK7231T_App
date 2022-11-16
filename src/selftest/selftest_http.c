#ifdef WINDOWS

#include "selftest_local.h"
#include "../httpserver/new_http.h"

// "GET /index?tgl=1 HTTP/1.1\r\n"
const char *http_get_template1 = "GET /%s HTTP/1.1\r\n"
"Host: 127.0.0.1\r\n"
"Connection: keep - alive\r\n"
"sec-ch-ua: \"Google Chrome\";v=\"107\", \"Chromium\";v=\"107\", \"Not=A?Brand\";v=\"24\"\r\n"
"sec-ch-ua-mobile: ? 0\r\n"
"User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/107.0.0.0 Safari/537.36\r\n"
"sec-ch-ua-platform: \"Windows\"\r\n"
"Accept: */*\r\n"
"Sec-Fetch-Site: same-origin\r\n"
"Sec-Fetch-Mode: cors\r\n"
"Sec-Fetch-Dest: empty\r\n"
"Referer: http://127.0.0.1/index\r\n"
"Accept-Encoding: gzip, deflate, br\r\n"
"Accept-Language: pl-PL,pl;q=0.9,en-US;q=0.8,en;q=0.7,de;q=0.6\r\n"
"\r\n"
"\r\n";

const char *Helper_GetPastHTTPHeader(const char *s) {
	while (*s) {
		if (!strncmp(s, "\r\n\r\n",4)) {
			return s + 4;
		}
		s++;
	}
	return 0;
}
static char outbuf[8192];
static char buffer[8192];
void Test_FakeHTTPClientPacket(const char *tg) {
	int iResult;
	int len;
	const char *replyAt;

	http_request_t request;

	sprintf(buffer, http_get_template1, tg);

	iResult = strlen(buffer);

	memset(&request, 0, sizeof(request));


	request.fd = 0;
	request.received = buffer;
	request.receivedLen = iResult;
	outbuf[0] = '\0';
	request.reply = outbuf;
	request.replylen = 0;

	request.replymaxlen = sizeof(outbuf);

	printf("Test_FakeHTTPClientPacket fake bytes sent: %d \n", iResult);
 	len = HTTP_ProcessPacket(&request);
	printf("Test_FakeHTTPClientPacket fake bytes received: %d \n", len);

	replyAt = Helper_GetPastHTTPHeader(outbuf);

}
void Test_Http() {

	CMD_ExecuteCommand("clearAll", 0);

	Test_FakeHTTPClientPacket("index?tgl=1");

	Test_FakeHTTPClientPacket("cm?cmnd=POWER");

	Test_FakeHTTPClientPacket("cm?cmnd=STATUS");


}


#endif
