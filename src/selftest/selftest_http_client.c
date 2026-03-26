#if defined(_WIN32)

#include "selftest_local.h"
#include "../httpclient/http_client.h"
#include <windows.h>

static int SelfTest_RunPostRequestAllocFailure(int *outRet) {
	int crashed = 0;
	int ret = -999;
	HTTPClient_Test_ClearLastRequest();
	HTTPClient_Test_SetFailPoint(HTTPCLIENT_TEST_FAIL_SENDPOST_REQUEST_ALLOC);
	__try {
		ret = HTTPClient_Async_SendPost("http://127.0.0.1/test", 80, "text/plain", "abc", 0);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		crashed = 1;
	}
	HTTPClient_Test_SetFailPoint(HTTPCLIENT_TEST_FAIL_NONE);
	HTTPClient_Test_FreeLastRequest();
	if (outRet) {
		*outRet = ret;
	}
	return crashed;
}

static int SelfTest_RunPostBodyDupFailure(int *outRet) {
	int crashed = 0;
	int ret = -999;
	HTTPClient_Test_ClearLastRequest();
	HTTPClient_Test_SetFailPoint(HTTPCLIENT_TEST_FAIL_SENDPOST_POSTBUF_DUP);
	__try {
		ret = HTTPClient_Async_SendPost("http://127.0.0.1/test", 80, "text/plain", "abc", 0);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		crashed = 1;
	}
	HTTPClient_Test_SetFailPoint(HTTPCLIENT_TEST_FAIL_NONE);
	HTTPClient_Test_FreeLastRequest();
	if (outRet) {
		*outRet = ret;
	}
	return crashed;
}

void Test_HTTP_Client() {
	int ret;
	int crashed;
	httprequest_t *request;

	SIM_ClearOBK(0);

	PIN_SetPinRoleForPinIndex(9, IOR_Relay);
	PIN_SetPinChannelForPinIndex(9, 1);

	SELFTEST_ASSERT_CHANNEL(1, 0);

	// Also nice method of testing: addRepeatingEvent 2 -1 SendGet http://192.168.0.103/cm?cmnd=POWER%20TOGGLE
	///CMD_ExecuteCommand("SendGet http://192.168.0.103/cm?cmnd=POWER%20TOGGLE", 0);
	//CMD_ExecuteCommand("SendGet http://192.168.0.104/cm?cmnd=POWER%20TOGGLE", 0);


	// The following selftest may be problematic. It does a real TCP send to our loopback 127.0.0.1 address
	// It will also fail if port 80 is already in use (and not in use by us)
#if 0
	// send GET to ourselves (127.0.0.1 is a localhost)
	CMD_ExecuteCommand("SendGet http://127.0.0.1/cm?cmnd=POWER%20TOGGLE", 0);
	Sim_RunSeconds(5, true);
	SELFTEST_ASSERT_CHANNEL(1, 1);
	// send GET to ourselves (127.0.0.1 is a localhost)
	CMD_ExecuteCommand("SendGet http://127.0.0.1/cm?cmnd=POWER%20TOGGLE", 0);
	Sim_RunSeconds(5, true);
	SELFTEST_ASSERT_CHANNEL(1, 0);
	// send GET to ourselves (127.0.0.1 is a localhost)
	CMD_ExecuteCommand("SendGet http://127.0.0.1/cm?cmnd=POWER%20ON", 0);
	Sim_RunSeconds(5, true);
	SELFTEST_ASSERT_CHANNEL(1, 1);
#endif
	//CMD_ExecuteCommand("SendGet http://127.0.0.1/cm?cmnd=POWER%20TOGGLE", 0);
	//Sim_RunFrames(15, false);
	//SELFTEST_ASSERT_CHANNEL(1, 1);

	HTTPClient_Test_SetSkipAsyncThread(1);

	crashed = SelfTest_RunPostRequestAllocFailure(&ret);
	SELFTEST_ASSERT(crashed == 0);
	SELFTEST_ASSERT(ret != 0);

	crashed = SelfTest_RunPostBodyDupFailure(&ret);
	SELFTEST_ASSERT(crashed == 0);
	SELFTEST_ASSERT(ret != 0);

	HTTPClient_Test_ClearLastRequest();
	HTTPClient_Test_SetFailPoint(HTTPCLIENT_TEST_FAIL_SENDGET_RESPONSEBUF_ALLOC);
	ret = HTTPClient_Async_SendGet("http://127.0.0.1/test", "cmd", 0);
	request = HTTPClient_Test_GetLastRequest();
	SELFTEST_ASSERT(ret != 0 || request == 0 || request->client_data.response_buf != 0 || request->client_data.response_buf_len == 0);
	HTTPClient_Test_SetFailPoint(HTTPCLIENT_TEST_FAIL_NONE);
	HTTPClient_Test_FreeLastRequest();

	HTTPClient_Test_SetSkipAsyncThread(0);
}

#else

void Test_HTTP_Client() {
}

#endif
