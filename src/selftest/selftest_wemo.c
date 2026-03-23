#ifdef WINDOWS

#include "selftest_local.h"

static const char *g_wemo_set_state_on =
"<?xml version=\"1.0\" encoding=\"utf-8\"?>"
"<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\">"
"<s:Body>"
"<u:SetBinaryState xmlns:u=\"urn:Belkin:service:basicevent:1\">"
"<BinaryState>1</BinaryState>"
"</u:SetBinaryState>"
"</s:Body>"
"</s:Envelope>";

static const char *g_wemo_set_state_off =
"<?xml version=\"1.0\" encoding=\"utf-8\"?>"
"<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\">"
"<s:Body>"
"<u:SetBinaryState xmlns:u=\"urn:Belkin:service:basicevent:1\">"
"<BinaryState>0</BinaryState>"
"</u:SetBinaryState>"
"</s:Body>"
"</s:Envelope>";

static const char *g_wemo_get_state =
"<?xml version=\"1.0\" encoding=\"utf-8\"?>"
"<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\">"
"<s:Body>"
"<u:GetBinaryState xmlns:u=\"urn:Belkin:service:basicevent:1\"/>"
"</s:Body>"
"</s:Envelope>";

static void Test_Wemo_RelaySetupAndSoap(void) {
	SIM_ClearOBK(0);

	CFG_SetDeviceName("Fallback Device Name");
	PIN_SetPinRoleForPinIndex(7, IOR_Relay);
	PIN_SetPinChannelForPinIndex(7, 2);
	CHANNEL_SetLabel(2, "Kitchen & <Plug> \"Main\"", 1);
	CHANNEL_SetType(1, ChType_Toggle);
	CHANNEL_SetLabel(1, "ShouldNotBeChosen", 1);
	CMD_ExecuteCommand("startDriver Wemo", 0);

	Test_FakeHTTPClientPacket_GET("setup.xml");
	SELFTEST_ASSERT_HTML_REPLY_CONTAINS("Kitchen &amp; &lt;Plug&gt; &quot;Main&quot;");
	SELFTEST_ASSERT_HTML_REPLY_NOT_CONTAINS("Fallback Device Name");
	SELFTEST_ASSERT_HTML_REPLY_NOT_CONTAINS("ShouldNotBeChosen");
	SELFTEST_ASSERT_HTML_REPLY_CONTAINS("<controlURL>/upnp/control/basicevent1</controlURL>");
	SELFTEST_ASSERT_HTML_REPLY_CONTAINS("<serviceType>urn:Belkin:service:metainfo:1</serviceType>");

	Test_FakeHTTPClientPacket_POST("upnp/control/basicevent1", g_wemo_set_state_on);
	SELFTEST_ASSERT_CHANNEL(2, 1);
	SELFTEST_ASSERT_CHANNEL(1, 0);
	SELFTEST_ASSERT_HTML_REPLY_CONTAINS("<BinaryState>1</BinaryState>");

	Test_FakeHTTPClientPacket_POST("upnp/control/basicevent1", g_wemo_get_state);
	SELFTEST_ASSERT_HTML_REPLY_CONTAINS("<u:GetBinaryStateResponse");
	SELFTEST_ASSERT_HTML_REPLY_CONTAINS("<BinaryState>1</BinaryState>");

	Test_FakeHTTPClientPacket_POST("upnp/control/basicevent1", g_wemo_set_state_off);
	SELFTEST_ASSERT_CHANNEL(2, 0);
	SELFTEST_ASSERT_CHANNEL(1, 0);
	SELFTEST_ASSERT_HTML_REPLY_CONTAINS("<BinaryState>0</BinaryState>");

	Test_FakeHTTPClientPacket_POST("upnp/control/metainfo1", g_wemo_get_state);
	SELFTEST_ASSERT_HTML_REPLY_CONTAINS("MetaInfoResponse");
	Test_FakeHTTPClientPacket_GET("eventservice.xml");
	SELFTEST_ASSERT_HTML_REPLY_CONTAINS("SetBinaryState");
	Test_FakeHTTPClientPacket_GET("metainfoservice.xml");
	SELFTEST_ASSERT_HTML_REPLY_CONTAINS("GetExtMetaInfo");
}

static void Test_Wemo_ToggleFallbackAndRestart(void) {
	SIM_ClearOBK(0);

	CFG_SetDeviceName("Toggle Device");
	CHANNEL_SetType(5, ChType_Toggle);
	CHANNEL_SetLabel(5, "Toggle Fallback", 1);
	CMD_ExecuteCommand("startDriver Wemo", 0);

	Test_FakeHTTPClientPacket_GET("setup.xml");
	SELFTEST_ASSERT_HTML_REPLY_CONTAINS("Toggle Fallback");

	Test_FakeHTTPClientPacket_POST("upnp/control/basicevent1", g_wemo_set_state_on);
	SELFTEST_ASSERT_CHANNEL(5, 1);
	SELFTEST_ASSERT_HTML_REPLY_CONTAINS("<BinaryState>1</BinaryState>");

	CMD_ExecuteCommand("stopDriver Wemo", 0);
	Test_FakeHTTPClientPacket_GET("setup.xml");
	SELFTEST_ASSERT_HTML_REPLY_CONTAINS("\"error\":404");

	CMD_ExecuteCommand("startDriver Wemo", 0);
	Test_FakeHTTPClientPacket_GET("setup.xml");
	SELFTEST_ASSERT_HTML_REPLY_CONTAINS("Toggle Fallback");

	Test_FakeHTTPClientPacket_POST("upnp/control/basicevent1", g_wemo_set_state_off);
	SELFTEST_ASSERT_CHANNEL(5, 0);
	SELFTEST_ASSERT_HTML_REPLY_CONTAINS("<BinaryState>0</BinaryState>");
}

void Test_Wemo() {
	Test_Wemo_RelaySetupAndSoap();
	Test_Wemo_ToggleFallbackAndRestart();
}

#endif
