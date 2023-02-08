#include "../new_common.h"
#include "http_fns.h"
#include "../new_pins.h"
#include "../new_cfg.h"
#include "../ota/ota.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../driver/drv_tuyaMCU.h"
#include "../driver/drv_public.h"
#include "../hal/hal_wifi.h"
#include "../hal/hal_pins.h"
#include "../hal/hal_flashConfig.h"
#include "../logging/logging.h"
#include "../devicegroups/deviceGroups_public.h"
#include "../mqtt/new_mqtt.h"
#include "hass.h"
#include "../cJSON/cJSON.h"
#include <time.h>
#include "../driver/drv_ntp.h"
#include "../driver/drv_local.h"



static int http_tasmota_json_Dimmer(void* request, jsonCb_t printer) {
	int dimmer;
	dimmer = LED_GetDimmer();
	printer(request, "\"Dimmer\":%i", dimmer);
	return 0;
}
static int http_tasmota_json_CT(void* request, jsonCb_t printer) {
	int temperature;
	// 154 to 500 range
	temperature = LED_GetTemperature();
	// Temperature 
	printer(request, "\"CT\":%i", temperature);
	return 0;
}
// https://tasmota.github.io/docs/Commands/#with-mqtt
/*
http://<ip>/cm?cmnd=Power%20TOGGLE
http://<ip>/cm?cmnd=Power%20On
http://<ip>/cm?cmnd=Power%20off
http://<ip>/cm?user=admin&password=joker&cmnd=Power%20Toggle
*/
// https://www.elektroda.com/rtvforum/viewtopic.php?p=19330027#19330027
// Web browser sends: GET /cm?cmnd=POWER1
// System responds with state
static int http_tasmota_json_power(void* request, jsonCb_t printer) {
	int numRelays;
	int numPWMs;
	int i;
	int lastRelayState;
	bool bRelayIndexingStartsWithZero;
	int relayIndexingOffset;

	bRelayIndexingStartsWithZero = CHANNEL_HasChannelPinWithRoleOrRole(0, IOR_Relay, IOR_Relay_n);
	if (bRelayIndexingStartsWithZero) {
		relayIndexingOffset = 0;
	}
	else {
		relayIndexingOffset = 1;
	}

	// try to return status
	numPWMs = PIN_CountPinsWithRoleOrRole(IOR_PWM, IOR_PWM_n);
	numRelays = 0;

	// LED driver (if has PWMs)
	if (LED_IsLEDRunning()) {
		http_tasmota_json_Dimmer(request, printer);
		printer(request, ",");
		printer(request, "\"Fade\":\"OFF\",");
		printer(request, "\"Speed\":1,");
		printer(request, "\"LedTable\":\"ON\",");
		if (LED_IsLedDriverChipRunning() || numPWMs >= 3) {
			/*
			{
			POWER: "OFF",
			Dimmer: 100,
			Color: "255,0,157",
			HSBColor: "323,100,100",
			Channel: [
			100,
			0,
			62
			]
			}*/
			// Eg: Color: "255,0,157",
			byte rgbcw[5];
			int hsv[3];
			byte channels[5];

			LED_GetFinalRGBCW(rgbcw);
			LED_GetFinalHSV(hsv);
			LED_GetFinalChannels100(channels);

			// it looks like they include C and W in color
			if (LED_IsLedDriverChipRunning() || numPWMs == 5) {
				printer(request, "\"Color\":\"%i,%i,%i,%i,%i\",",
					(int)rgbcw[0], (int)rgbcw[1], (int)rgbcw[2], (int)rgbcw[3], (int)rgbcw[4]);
			}
			else {
				printer(request, "\"Color\":\"%i,%i,%i\",", (int)rgbcw[0], (int)rgbcw[1], (int)rgbcw[2]);
			}
			printer(request, "\"HSBColor\":\"%i,%i,%i\",", hsv[0], hsv[1], hsv[2]);
			printer(request, "\"Channel\":[%i,%i,%i],", (int)channels[0], (int)channels[1], (int)channels[2]);

		}
		if (LED_IsLedDriverChipRunning() || numPWMs == 5 || numPWMs == 2) {
			http_tasmota_json_CT(request, printer);
			printer(request, ",");
		}
		if (LED_GetEnableAll() == 0) {
			printer(request, "\"POWER\":\"OFF\"");
		}
		else {
			printer(request, "\"POWER\":\"ON\"");
		}
	}
	else {
		// relays driver
		for (i = 0; i < CHANNEL_MAX; i++) {
			if (h_isChannelRelay(i) || CHANNEL_GetType(i) == ChType_Toggle) {
				numRelays++;
				lastRelayState = CHANNEL_Get(i);
			}
		}
		if (numRelays == 1) {
			if (lastRelayState) {
				printer(request, "\"POWER\":\"ON\"");
			}
			else {
				printer(request, "\"POWER\":\"OFF\"");
			}
		}
		else {
			int c_posted = 0;
			for (i = 0; i < CHANNEL_MAX; i++) {
				if (h_isChannelRelay(i) || CHANNEL_GetType(i) == ChType_Toggle) {
					int indexStartingFrom1;

					if (bRelayIndexingStartsWithZero) {
						indexStartingFrom1 = i + 1;
					}
					else {
						indexStartingFrom1 = i;
					}
					lastRelayState = CHANNEL_Get(i);
					if (c_posted) {
						printer(request, ",");
					}
					if (lastRelayState) {
						printer(request, "\"POWER%i\":\"ON\"", indexStartingFrom1);
					}
					else {
						printer(request, "\"POWER%i\":\"OFF\"", indexStartingFrom1);
					}
					c_posted++;
				}
			}
		}

	}
	return 0;
}
/*
{"StatusSNS":{"Time":"2022-07-30T10:11:26","ENERGY":{"TotalStartTime":"2022-05-12T10:56:31","Total":0.003,"Yesterday":0.003,"Today":0.000,"Power": 0,"ApparentPower": 0,"ReactivePower": 0,"Factor":0.00,"Voltage":236,"Current":0.000}}}
*/


static int http_tasmota_json_ENERGY(void* request, jsonCb_t printer) {
	float power, factor, voltage, current;
	float energy, energy_hour;

	factor = 0; // TODO
	voltage = DRV_GetReading(OBK_VOLTAGE);
	current = DRV_GetReading(OBK_CURRENT);
	power = DRV_GetReading(OBK_POWER);
	energy = DRV_GetReading(OBK_CONSUMPTION_TOTAL);
	energy_hour = DRV_GetReading(OBK_CONSUMPTION_LAST_HOUR);

	// following check will clear NaN values
	if (OBK_IS_NAN(energy)) {
		energy = 0;
	}
	if (OBK_IS_NAN(energy_hour)) {
		energy_hour = 0;
	}
	printer(request, "{");
	printer(request, "\"Power\": %f,", power);
	printer(request, "\"ApparentPower\": 0,\"ReactivePower\": 0,\"Factor\":%f,", factor);
	printer(request, "\"Voltage\":%f,", voltage);
	printer(request, "\"Current\":%f,", current);
	printer(request, "\"ConsumptionTotal\":%f,", energy);
	printer(request, "\"ConsumptionLastHour\":%f", energy_hour);
	// close ENERGY block
	printer(request, "}");
	return 0;
}

// Topic: tele/tasmota_48E7F3/SENSOR at 3:06 AM:
// Sample:
/*
{
	"Time": "2022-12-30T03:06:36",
	"ENERGY": {
		"TotalStartTime": "2022-05-12T10:56:31",
		"Total": 0.007,
		"Yesterday": 0,
		"Today": 0,
		"Period": 0,
		"Power": 0,
		"ApparentPower": 0,
		"ReactivePower": 0,
		"Factor": 0,
		"Voltage": 241,
		"Current": 0
	}
}
*/
static int http_tasmota_json_status_SNS(void* request, jsonCb_t printer, bool bAppendHeader) {
	char buff[20];

	if (bAppendHeader) {
		printer(request, "\"StatusSNS\":");
	}
	printer(request, "{");

	time_t localTime = (time_t)NTP_GetCurrentTime();
	strftime(buff, sizeof(buff), "%Y-%m-%dT%H:%M:%S", localtime(&localTime));
	printer(request, "\"Time\":\"%s\"", buff);

#ifndef OBK_DISABLE_ALL_DRIVERS
	if (DRV_IsMeasuringPower()) {

		// begin ENERGY block
		printer(request, ",");
		printer(request, "\"ENERGY\":");
		http_tasmota_json_ENERGY(request, printer);
	}
#endif

	printer(request, "}");

	return 0;
}

#ifdef PLATFORM_XR809
//XR809 does not support drivers but its build script compiles many drivers including ntp.

#else
#ifndef ENABLE_BASIC_DRIVERS
unsigned int NTP_GetCurrentTime() {
	return 0;
}
unsigned int NTP_GetCurrentTimeWithoutOffset() {
	return 0;
}
#endif
#endif

// Topic:  tele/tasmota_48E7F3/STATE
// Sample:
/*
{
	"Time": "2022-12-30T03:06:36",
	"Uptime": "0T06:16:14",
	"UptimeSec": 22574,
	"Heap": 26,
	"SleepMode": "Dynamic",
	"Sleep": 50,
	"LoadAvg": 19,
	"MqttCount": 1,
	"POWER": "ON",
	"Wifi": {
		"AP": 1,
		"SSId": "ASUS_25G_WIFI",
		"BSSId": "32:21:BA:10:F6:6D",
		"Channel": 3,
		"Mode": "11n",
		"RSSI": 62,
		"Signal": -69,
		"LinkCount": 1,
		"Downtime": "0T00:00:04"
	}
}
*/
static int http_tasmota_json_status_STS(void* request, jsonCb_t printer, bool bAppendHeader) {
	char buff[20];
	time_t localTime = (time_t)NTP_GetCurrentTime();

	if (bAppendHeader) {
		printer(request, "\"StatusSTS\":");
	}
	printer(request, "{");
	strftime(buff, sizeof(buff), "%Y-%m-%dT%H:%M:%S", localtime(&localTime));
	printer(request, "\"Time\":\"%s\",", buff);
	printer(request, "\"Uptime\":\"30T02:59:30\",");
	printer(request, "\"UptimeSec\":%i,", Time_getUpTimeSeconds());
	printer(request, "\"Heap\":25,");
	printer(request, "\"SleepMode\":\"Dynamic\",");
	printer(request, "\"Sleep\":10,");
	printer(request, "\"LoadAvg\":99,");
	printer(request, "\"MqttCount\":23,");

	http_tasmota_json_power(request, printer);
	printer(request, ",");
	printer(request, "\"Wifi\":{"); // open WiFi
	printer(request, "\"AP\":1,");
	printer(request, "\"SSId\":\"%s\",", CFG_GetWiFiSSID());
	printer(request, "\"BSSId\":\"30:B5:C2:5D:70:72\",");
	printer(request, "\"Channel\":11,");
	printer(request, "\"Mode\":\"11n\",");
	printer(request, "\"RSSI\":78,");
	printer(request, "\"Signal\":%i,", HAL_GetWifiStrength());
	printer(request, "\"LinkCount\":21,");
	printer(request, "\"Downtime\":\"0T06:13:34\"");
	printer(request, "}"); // close WiFi
	printer(request, "}");
	return 0;
}
static int http_tasmota_json_status_TIM(void* request, jsonCb_t printer) {
	char buff[20];

	time_t localTime = (time_t)NTP_GetCurrentTime();
	time_t localUTC = (time_t)NTP_GetCurrentTimeWithoutOffset();
	printer(request, "\"StatusTIM\":{");
	strftime(buff, sizeof(buff), "%Y-%m-%dT%H:%M:%S", localtime(&localUTC));
	printer(request, "\"UTC\":\"%s\",", buff);
	strftime(buff, sizeof(buff), "%Y-%m-%dT%H:%M:%S", localtime(&localTime));
	printer(request, "\"Local\":\"%s\",", buff);
	printer(request, "\"StartDST\":\"2022-03-27T02:00:00\",");
	printer(request, "\"EndDST\":\"2022-10-30T03:00:00\",");
	printer(request, "\"Timezone\":\"+01:00\",");
	printer(request, "\"Sunrise\":\"07:50\",");
	printer(request, "\"Sunset\":\"17:17\"");
	printer(request, "}");
	return 0;
}
static int http_tasmota_json_status_FWR(void* request, jsonCb_t printer) {

	printer(request, "\"StatusFWR\":{");
	printer(request, "\"Version\":\"%s\",", DEVICENAME_PREFIX_FULL"_"USER_SW_VER);
	printer(request, "\"BuildDateTime\":\"%s\",", __DATE__ " " __TIME__);
	printer(request, "\"Boot\":7,");
	printer(request, "\"Core\":\"%s\",", "0.0");
	printer(request, "\"SDK\":\"\",", "obk");
	printer(request, "\"CpuFrequency\":80,");
	printer(request, "\"Hardware\":\"%s\",", PLATFORM_MCU_NAME);
	printer(request, "\"CR\":\"465/699\"");
	printer(request, "}");
	return 0;
}
static int http_tasmota_json_status_MEM(void* request, jsonCb_t printer) {
	printer(request, "\"StatusMEM\":{");
	printer(request, "\"ProgramSize\":616,");
	printer(request, "\"Free\":384,");
	printer(request, "\"Heap\":25,");
	printer(request, "\"ProgramFlashSize\":1024,");
	printer(request, "\"FlashSize\":2048,");
	printer(request, "\"FlashChipId\":\"1540A1\",");
	printer(request, "\"FlashFrequency\":40,");
	printer(request, "\"FlashMode\":3,");
	printer(request, "\"Features\":[");
	printer(request, "\"00000809\",");
	printer(request, "\"8FDAC787\",");
	printer(request, "\"04368001\",");
	printer(request, "\"000000CF\",");
	printer(request, "\"010013C0\",");
	printer(request, "\"C000F981\",");
	printer(request, "\"00004004\",");
	printer(request, "\"00001000\",");
	printer(request, "\"00000020\"");
	printer(request, "],");
	printer(request, "\"Drivers\":\"1,2,3,4,5,6,7,8,9,10,12,16,18,19,20,21,22,24,26,27,29,30,35,37,45\",");
	printer(request, "\"Sensors\":\"1,2,3,4,5,6\"");
	printer(request, "}");
	return 0;
}
static int http_tasmota_json_status_NET(void* request, jsonCb_t printer) {

	printer(request, "\"StatusNET\":{");
	printer(request, "\"Hostname\":\"%s\",", CFG_GetShortDeviceName());
	printer(request, "\"IPAddress\":\"%s\",", HAL_GetMyIPString());
	printer(request, "\"Gateway\":\"192.168.0.1\",");
	printer(request, "\"Subnetmask\":\"255.255.255.0\",");
	printer(request, "\"DNSServer1\":\"192.168.0.1\",");
	printer(request, "\"DNSServer2\":\"0.0.0.0\",");
	printer(request, "\"Mac\":\"10:52:1C:D7:9E:2C\",");
	printer(request, "\"Webserver\":2,");
	printer(request, "\"HTTP_API\":1,");
	printer(request, "\"WifiConfig\":4,");
	printer(request, "\"WifiPower\":17.0");
	printer(request, "}");
	return 0;
}
static int http_tasmota_json_status_MQT(void* request, jsonCb_t printer) {

	printer(request, "\"StatusMQT\":{");
	printer(request, "\"MqttHost\":\"%s\",", CFG_GetMQTTHost());
	printer(request, "\"MqttPort\":%i,", CFG_GetMQTTPort());
	printer(request, "\"MqttClientMask\":\"core-mosquitto\",");
	printer(request, "\"MqttClient\":\"%s\",", CFG_GetMQTTClientId());
	printer(request, "\"MqttUser\":\"%s\",", CFG_GetMQTTUserName());
	printer(request, "\"MqttCount\":23,");
	printer(request, "\"MAX_PACKET_SIZE\":1200,");
	printer(request, "\"KEEPALIVE\":30,");
	printer(request, "\"SOCKET_TIMEOUT\":4");
	printer(request, "}");
	return 0;
}
/*
{"Status":{"Module":0,"DeviceName":"Tasmota","FriendlyName":["Tasmota"],"Topic":"tasmota_48E7F3","ButtonTopic":"0","Power":1,"PowerOnState":3,"LedState":1,"LedMask":"FFFF","SaveData":1,"SaveState":1,"SwitchTopic":"0","SwitchMode":[0,0,0,0,0,0,0,0],"ButtonRetain":0,"SwitchRetain":0,"SensorRetain":0,"PowerRetain":0,"InfoRetain":0,"StateRetain":0}}
*/
static int http_tasmota_json_status_generic(void* request, jsonCb_t printer) {
	const char* deviceName;
	const char* friendlyName;
	const char* clientId;
	int powerCode;
	int relayCount, pwmCount, dInputCount, i;
	bool bRelayIndexingStartsWithZero;

	deviceName = CFG_GetShortDeviceName();
	friendlyName = CFG_GetDeviceName();
	clientId = CFG_GetMQTTClientId();

	//deviceName = "Tasmota";
	//friendlyName - "Tasmota";

	bRelayIndexingStartsWithZero = CHANNEL_HasChannelPinWithRoleOrRole(0, IOR_Relay, IOR_Relay_n);

	get_Relay_PWM_Count(&relayCount, &pwmCount, &dInputCount);

	if (LED_IsLEDRunning()) {
		powerCode = LED_GetEnableAll();
	}
	else {
		powerCode = 0;
		for (i = 0; i < CHANNEL_MAX; i++) {
			bool bRelay;
			int useIdx;
			int iValue;
			if (bRelayIndexingStartsWithZero) {
				useIdx = i;
			}
			else {
				useIdx = i + 1;
			}
			bRelay = CHANNEL_HasChannelPinWithRoleOrRole(useIdx, IOR_Relay, IOR_Relay_n);
			if (bRelay) {
				iValue = CHANNEL_Get(useIdx);
				if (iValue)
					BIT_SET(powerCode, i);
				else
					BIT_CLEAR(powerCode, i);
			}
		}
	}

	printer(request, "{");
	// Status section
	printer(request, "\"Status\":{\"Module\":0,\"DeviceName\":\"%s\"", deviceName);
	printer(request, ",\"FriendlyName\":[");
	if (relayCount == 0) {
		printer(request, "\"%s\"", deviceName);
	}
	else {
		int c_printed = 0;
		for (i = 0; i < CHANNEL_MAX; i++) {
			bool bRelay;
			bRelay = CHANNEL_HasChannelPinWithRoleOrRole(i, IOR_Relay, IOR_Relay_n);
			if (bRelay) {
				int useIdx;
				if (bRelayIndexingStartsWithZero) {
					useIdx = i + 1;
				}
				else {
					useIdx = i;
				}
				if (c_printed) {
					printer(request, ",");
				}
				printer(request, "\"%s_%i\"", deviceName, useIdx);
				c_printed++;
			}
		}
	}
	printer(request, "]");
	printer(request, ",\"Topic\":\"%s\",\"ButtonTopic\":\"0\"", clientId);
	printer(request, ",\"Power\":%i,\"PowerOnState\":3,\"LedState\":1", powerCode);
	printer(request, ",\"LedMask\":\"FFFF\",\"SaveData\":1,\"SaveState\":1");
	printer(request, ",\"SwitchTopic\":\"0\",\"SwitchMode\":[0,0,0,0,0,0,0,0]");
	printer(request, ",\"ButtonRetain\":0,\"SwitchRetain\":0,\"SensorRetain\":0");
	printer(request, ",\"PowerRetain\":0,\"InfoRetain\":0,\"StateRetain\":0");
	printer(request, "}");

	printer(request, ",");


	printer(request, "\"StatusPRM\":{");
	printer(request, "\"Baudrate\":115200,");
	printer(request, "\"SerialConfig\":\"8N1\",");
	printer(request, "\"GroupTopic\":\"tasmotas\",");
	printer(request, "\"OtaUrl\":\"http://ota.tasmota.com/tasmota/release/tasmota.bin.gz\",");
	printer(request, "\"RestartReason\":\"HardwareWatchdog\",");
	printer(request, "\"Uptime\":\"30T02:59:30\",");
	printer(request, "\"StartupUTC\":\"2022-10-10T16:09:41\",");
	printer(request, "\"Sleep\":50,");
	printer(request, "\"CfgHolder\":4617,");
	printer(request, "\"BootCount\":22,");
	printer(request, "\"BCResetTime\":\"2022-01-27T16:10:56\",");
	printer(request, "\"SaveCount\":1235,");
	printer(request, "\"SaveAddress\":\"F9000\"");
	printer(request, "}");

	printer(request, ",");

	http_tasmota_json_status_FWR(request, printer);

	printer(request, ",");



	printer(request, "\"StatusLOG\":{");
	printer(request, "\"SerialLog\":2,");
	printer(request, "\"WebLog\":2,");
	printer(request, "\"MqttLog\":0,");
	printer(request, "\"SysLog\":0,");
	printer(request, "\"LogHost\":\"\",");
	printer(request, "\"LogPort\":514,");
	printer(request, "\"SSId\":[");
	printer(request, "\"%s\",", CFG_GetWiFiSSID());
	printer(request, "\"\"");
	printer(request, "],");
	printer(request, "\"TelePeriod\":300,");
	printer(request, "\"Resolution\":\"558180C0\",");
	printer(request, "\"SetOption\":[");
	printer(request, "\"000A8009\",");
	printer(request, "\"2805C80001000600003C5A0A000000000000\",");
	printer(request, "\"00000280\",");
	printer(request, "\"00006008\",");
	printer(request, "\"00004000\"");
	printer(request, "]");
	printer(request, "}");

	printer(request, ",");



	http_tasmota_json_status_MEM(request, printer);

	printer(request, ",");

	http_tasmota_json_status_NET(request, printer);

	printer(request, ",");


	http_tasmota_json_status_MQT(request, printer);


	printer(request, ",");

	http_tasmota_json_status_TIM(request, printer);

	printer(request, ",");


	http_tasmota_json_status_SNS(request, printer, true);

	printer(request, ",");


	http_tasmota_json_status_STS(request, printer, true);

	// end
	printer(request, "}");



	return 0;
}
int JSON_ProcessCommandReply(const char *cmd, const char *arg, void *request, jsonCb_t printer, int flags) {

	if (!wal_strnicmp(cmd, "POWER", 5)) {

		printer(request, "{");
		http_tasmota_json_power(request, printer);
		printer(request, "}");
		if (flags == COMMAND_FLAG_SOURCE_MQTT) {
			MQTT_PublishPrinterContentsToStat((struct obk_mqtt_publishReplyPrinter_s *)request, "RESULT");
		}
	}
	else if (!wal_strnicmp(cmd, "CT", 2)) {
		printer(request, "{");
		if (*arg == 0) {
			http_tasmota_json_CT(request, printer);
		}
		else {
			http_tasmota_json_power(request, printer);
		}
		printer(request, "}");
		if (flags == COMMAND_FLAG_SOURCE_MQTT) {
			MQTT_PublishPrinterContentsToStat((struct obk_mqtt_publishReplyPrinter_s *)request, "RESULT");
		}
	}
	else if (!wal_strnicmp(cmd, "Dimmer", 6)) {
		printer(request, "{");
		if (*arg == 0) {
			http_tasmota_json_Dimmer(request, printer);
		} 
		else {
			http_tasmota_json_power(request, printer);
		}
		printer(request, "}");
		if (flags == COMMAND_FLAG_SOURCE_MQTT) {
			MQTT_PublishPrinterContentsToStat((struct obk_mqtt_publishReplyPrinter_s *)request, "RESULT");
		}
	}
	else if (!wal_strnicmp(cmd, "STATE", 5)) {
		http_tasmota_json_status_STS(request, printer, false);
		if (flags == COMMAND_FLAG_SOURCE_MQTT) {
			MQTT_PublishPrinterContentsToStat((struct obk_mqtt_publishReplyPrinter_s *)request, "RESULT");
		}
		if (flags == COMMAND_FLAG_SOURCE_TELESENDER) {
			MQTT_PublishPrinterContentsToTele((struct obk_mqtt_publishReplyPrinter_s *)request, "STATE");
		}
	}
	else if (!wal_strnicmp(cmd, "SENSOR", 5)) {
		// not a Tasmota command, but still required for us
		http_tasmota_json_status_SNS(request, printer, false);
		if (flags == COMMAND_FLAG_SOURCE_TELESENDER) {
			MQTT_PublishPrinterContentsToTele((struct obk_mqtt_publishReplyPrinter_s *)request, "SENSOR");
		}
	}
	else if (!wal_strnicmp(cmd, "STATUS", 6)) {
		if (!stricmp(arg, "8") || !stricmp(arg, "10")) {
			printer(request, "{");
			http_tasmota_json_status_SNS(request, printer, true);
			printer(request, "}");
			if (flags == COMMAND_FLAG_SOURCE_MQTT) {
				if (arg[0] == '8') {
					MQTT_PublishPrinterContentsToStat((struct obk_mqtt_publishReplyPrinter_s *)request, "STATUS8");
				}
				else {
					MQTT_PublishPrinterContentsToStat((struct obk_mqtt_publishReplyPrinter_s *)request, "STATUS10");
				}
			}
		} else if (!stricmp(arg, "6") ) {
			printer(request, "{");
			http_tasmota_json_status_MQT(request, printer);
			printer(request, "}");
			if (flags == COMMAND_FLAG_SOURCE_MQTT) {
				MQTT_PublishPrinterContentsToStat((struct obk_mqtt_publishReplyPrinter_s *)request, "STATUS6");
			}
		}
		else if (!stricmp(arg, "7")) {
			printer(request, "{");
			http_tasmota_json_status_TIM(request, printer);
			printer(request, "}");
			if (flags == COMMAND_FLAG_SOURCE_MQTT) {
				MQTT_PublishPrinterContentsToStat((struct obk_mqtt_publishReplyPrinter_s *)request, "STATUS7");
			}
		}
		else if (!stricmp(arg, "5")) {
			printer(request, "{");
			http_tasmota_json_status_NET(request, printer);
			printer(request, "}");
			if (flags == COMMAND_FLAG_SOURCE_MQTT) {
				MQTT_PublishPrinterContentsToStat((struct obk_mqtt_publishReplyPrinter_s *)request, "STATUS5");
			}
		}
		else if (!stricmp(arg, "4")) {
			printer(request, "{");
			http_tasmota_json_status_MEM(request, printer);
			printer(request, "}");
			if (flags == COMMAND_FLAG_SOURCE_MQTT) {
				MQTT_PublishPrinterContentsToStat((struct obk_mqtt_publishReplyPrinter_s *)request, "STATUS4");
			}
		}
		else if (!stricmp(arg, "2")) {
			printer(request, "{");
			http_tasmota_json_status_FWR(request, printer);
			printer(request, "}");
			if (flags == COMMAND_FLAG_SOURCE_MQTT) {
				MQTT_PublishPrinterContentsToStat((struct obk_mqtt_publishReplyPrinter_s *)request, "STATUS2");
			}
		}
		else {
			http_tasmota_json_status_generic(request, printer);
			if (flags == COMMAND_FLAG_SOURCE_MQTT) {
				MQTT_PublishPrinterContentsToStat((struct obk_mqtt_publishReplyPrinter_s *)request, "STATUS");
			}
		}
	}

	return 0;
}



