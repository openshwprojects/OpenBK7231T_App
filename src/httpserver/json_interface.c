#include "../new_common.h"
#include "http_fns.h"
#include "../new_pins.h"
#include "../new_cfg.h"
#include "../ota/ota.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../driver/drv_tuyaMCU.h"
#include "../driver/drv_public.h"
#include "../driver/drv_battery.h"
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
#include "../driver/drv_bl_shared.h"

#if ENABLE_TASMOTA_JSON

void JSON_PrintKeyValue_String(void* request, jsonCb_t printer, const char* key, const char* value, bool bComma) {
	printer(request, "\"%s\":\"%s\"", key, value);
	if (bComma) {
		printer(request, ",");
	}
}
void JSON_PrintKeyValue_Int(void* request, jsonCb_t printer, const char* key, int value, bool bComma) {
	printer(request, "\"%s\":%i", key, value);
	if (bComma) {
		printer(request, ",");
	}
}
void JSON_PrintKeyValue_Float(void* request, jsonCb_t printer, const char* key, float value, bool bComma) {
	printer(request, "\"%s\":%f", key, value);
	if (bComma) {
		printer(request, ",");
	}
}

static int http_tasmota_json_Dimmer(void* request, jsonCb_t printer) {
	int dimmer;
	dimmer = LED_GetDimmer();
	JSON_PrintKeyValue_Int(request, printer, "Dimmer", dimmer, false);
	return 0;
}
static int http_tasmota_json_CT(void* request, jsonCb_t printer) {
	int temperature;
	// 154 to 500 range
	temperature = LED_GetTemperature();
	// Temperature 
	JSON_PrintKeyValue_Int(request, printer, "CT", temperature, false);
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
	char buff32[32];

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
		// Temperature 
		JSON_PrintKeyValue_String(request, printer, "Fade", "OFF", true);
		JSON_PrintKeyValue_Int(request, printer, "Speed", 1, true);
		JSON_PrintKeyValue_String(request, printer, "LedTable", "ON", true);
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
			LED_GetTasmotaHSV(hsv);
			LED_GetFinalChannels100(channels);

			// it looks like they include C and W in color
			if (LED_IsLedDriverChipRunning() || numPWMs == 5) {
				sprintf(buff32, "%i,%i,%i,%i,%i", (int)rgbcw[0], (int)rgbcw[1], (int)rgbcw[2], (int)rgbcw[3], (int)rgbcw[4]);
			}
			else {
				sprintf(buff32, "%i,%i,%i", (int)rgbcw[0], (int)rgbcw[1], (int)rgbcw[2]);
			}
			JSON_PrintKeyValue_String(request, printer, "Color", buff32, true);
			sprintf(buff32, "%i,%i,%i", (int)hsv[0], (int)hsv[1], (int)hsv[2]);
			JSON_PrintKeyValue_String(request, printer, "HSBColor", buff32, true);
			printer(request, "\"Channel\":[%i,%i,%i],", (int)channels[0], (int)channels[1], (int)channels[2]);

		}
		if (LED_IsLedDriverChipRunning() || numPWMs == 5 || numPWMs == 2) {
			http_tasmota_json_CT(request, printer);
			printer(request, ",");
		}
		if (LED_GetEnableAll() == 0) {
			JSON_PrintKeyValue_String(request, printer, "POWER", "OFF", false);
		}
		else {
			JSON_PrintKeyValue_String(request, printer, "POWER", "ON", false);
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
		if (numRelays == 0) {
			JSON_PrintKeyValue_String(request, printer, "POWER", "ON", false);
		}
		else if (numRelays == 1) {
			if (lastRelayState) {
				JSON_PrintKeyValue_String(request, printer, "POWER", "ON", false);
			}
			else {
				JSON_PrintKeyValue_String(request, printer, "POWER", "OFF", false);
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
					sprintf(buff32, "POWER%i", indexStartingFrom1);
					if (lastRelayState) {
						JSON_PrintKeyValue_String(request, printer, buff32, "ON", false);
					}
					else {
						JSON_PrintKeyValue_String(request, printer, buff32, "OFF", false);
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
#ifdef ENABLE_DRIVER_BL0937
// returns NaN values as 0
static float _getReading_NanToZero(energySensor_t type) {
	float retval = DRV_GetReading(type);
	return OBK_IS_NAN(retval) ? 0 : retval;
}

static int http_tasmota_json_ENERGY(void* request, jsonCb_t printer) {
	float voltage, batterypercentage = 0;

	if (DRV_IsMeasuringBattery()) {
#ifdef ENABLE_DRIVER_BATTERY
		voltage = Battery_lastreading(OBK_BATT_VOLTAGE) / 1000.00;
		batterypercentage = Battery_lastreading(OBK_BATT_LEVEL);
#endif
		printer(request, "{");
		printer(request, "\"Voltage\":%.4f,", _getReading_NanToZero(OBK_VOLTAGE));
		printer(request, "\"Batterypercentage\":%.0f", batterypercentage);
		// close ENERGY block
		printer(request, "}");
	}
	else {
		printer(request, "{"); 
		printer(request, "\"Power\": %f,", _getReading_NanToZero(OBK_POWER));
		printer(request, "\"ApparentPower\": %f,", _getReading_NanToZero(OBK_POWER_APPARENT));
		printer(request, "\"ReactivePower\": %f,", _getReading_NanToZero(OBK_POWER_REACTIVE));
		printer(request, "\"Factor\":%f,", _getReading_NanToZero(OBK_POWER_FACTOR));
		printer(request, "\"Voltage\":%f,", _getReading_NanToZero(OBK_VOLTAGE));
		printer(request, "\"Current\":%f,", _getReading_NanToZero(OBK_CURRENT));
		printer(request, "\"ConsumptionTotal\":%f,", _getReading_NanToZero(OBK_CONSUMPTION_TOTAL));
		printer(request, "\"Yesterday\": %f,", _getReading_NanToZero(OBK_CONSUMPTION_YESTERDAY));
		printer(request, "\"ConsumptionLastHour\":%f", _getReading_NanToZero(OBK_CONSUMPTION_LAST_HOUR));
		// close ENERGY block
		printer(request, "}");
	}
	return 0;
}
#endif	// ENABLE_DRIVER_BL0937
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
static int http_tasmota_json_SENSOR(void* request, jsonCb_t printer) {
	float chan_val1, chan_val2;
	int channel_1, channel_2, g_pin_1 = 0;
	printer(request, ",");
	if (DRV_IsRunning("SHT3X")) {
		g_pin_1 = PIN_FindPinIndexForRole(IOR_SHT3X_DAT, g_pin_1);
		channel_1 = g_cfg.pins.channels[g_pin_1];
		channel_2 = g_cfg.pins.channels2[g_pin_1];

		chan_val1 = CHANNEL_GetFloat(channel_1) / 10.0f;
		chan_val2 = CHANNEL_GetFloat(channel_2);

		// writer header
		printer(request, "\"SHT3X\":");
		// following check will clear NaN values
		printer(request, "{");
		printer(request, "\"Temperature\": %.1f,", chan_val1);
		printer(request, "\"Humidity\": %.0f", chan_val2);
		// close ENERGY block
		printer(request, "},");
	}
	if (DRV_IsRunning("CHT83XX")) {
		g_pin_1 = PIN_FindPinIndexForRole(IOR_CHT83XX_DAT, g_pin_1);
		channel_1 = g_cfg.pins.channels[g_pin_1];
		channel_2 = g_cfg.pins.channels2[g_pin_1];

		chan_val1 = CHANNEL_GetFloat(channel_1) / 10.0f;
		chan_val2 = CHANNEL_GetFloat(channel_2);

		// writer header
		printer(request, "\"CHT83XX\":");
		// following check will clear NaN values
		printer(request, "{");
		printer(request, "\"Temperature\": %.1f,", chan_val1);
		printer(request, "\"Humidity\": %.0f", chan_val2);
		// close ENERGY block
		printer(request, "},");
	}
	for (int i = 0; i < PLATFORM_GPIO_MAX; i++) {
		int role = PIN_GetPinRoleForPinIndex(i);
		if (role != IOR_DHT11 && role != IOR_DHT12 && role != IOR_DHT21 && role != IOR_DHT22)
			continue;
		channel_1 = g_cfg.pins.channels[i];
		channel_2 = g_cfg.pins.channels2[i];

		chan_val1 = CHANNEL_GetFloat(channel_1) / 10.0f;
		chan_val2 = CHANNEL_GetFloat(channel_2);

		// writer header
		// TODO - index?
		printer(request, "\"DHT\":");
		// following check will clear NaN values
		printer(request, "{");
		printer(request, "\"Temperature\": %.1f,", chan_val1);
		printer(request, "\"Humidity\": %.0f", chan_val2);
		// close ENERGY block
		printer(request, "},");
	}
	if (DRV_IsRunning("SGP")) {
		g_pin_1 = PIN_FindPinIndexForRole(IOR_SGP_DAT, g_pin_1);
		channel_1 = g_cfg.pins.channels[g_pin_1];
		channel_2 = g_cfg.pins.channels2[g_pin_1];

		chan_val1 = CHANNEL_GetFloat(channel_1);
		chan_val2 = CHANNEL_GetFloat(channel_2);

		// writer header
		printer(request, "\"SGP\":");
		// following check will clear NaN values
		printer(request, "{");
		printer(request, "\"CO2\": %.0f,", chan_val1);
		printer(request, "\"Tvoc\": %.0f", chan_val2);
		// close ENERGY block
		printer(request, "},");
	}
	return 0;
}
// Test command: http://192.168.0.159/cm?cmnd=STATUS%208
// For a device without sensors, it returns (on Tasmota):
/*
{"StatusSNS":{"Time":"2023-04-10T10:19:55"}}
*/
static void format_date(char *buffer, int buflength, struct tm *ltm)
{
	snprintf(buffer, buflength, "%04d-%02d-%02dT%02d:%02d:%02d",ltm->tm_year+1900, ltm->tm_mon+1, ltm->tm_mday, ltm->tm_hour, ltm->tm_min, ltm->tm_sec);
}

static int http_tasmota_json_status_SNS(void* request, jsonCb_t printer, bool bAppendHeader) {
	char buff[20];

	if (bAppendHeader) {
		printer(request, "\"StatusSNS\":");
	}
	printer(request, "{");

	time_t localTime = (time_t)NTP_GetCurrentTime();
	format_date(buff, sizeof(buff), gmtime(&localTime));
	JSON_PrintKeyValue_String(request, printer, "Time", buff, false);

#ifndef OBK_DISABLE_ALL_DRIVERS
#ifdef ENABLE_DRIVER_BL0937
	if (DRV_IsMeasuringPower() || DRV_IsMeasuringBattery()) {

		// begin ENERGY block
		printer(request, ",");
		printer(request, "\"ENERGY\":");
		http_tasmota_json_ENERGY(request, printer);
	}
#endif	// ENABLE_DRIVER_BL0937
	bool bHasAnyDHT = false;
	for (int i = 0; i < PLATFORM_GPIO_MAX; i++) {
		int role = PIN_GetPinRoleForPinIndex(i);
		if (role != IOR_DHT11 && role != IOR_DHT12 && role != IOR_DHT21 && role != IOR_DHT22)
			continue;
		bHasAnyDHT = true;
		break;
	}
	if (DRV_IsSensor() || bHasAnyDHT) {
		http_tasmota_json_SENSOR(request, printer);
		JSON_PrintKeyValue_String(request, printer, "TempUnit", "C", false);
	}
#endif

	printer(request, "}");

	return 0;
}

#ifdef PLATFORM_XR809
//XR809 does not support drivers but its build script compiles many drivers including ntp.

#else
#ifndef ENABLE_NTP
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

void format_time(int total_seconds, char* output, int outLen) {
	int days = total_seconds / 86400;
	int hours = (total_seconds / 3600) % 24;
	int minutes = (total_seconds / 60) % 60;
	int seconds = total_seconds % 60;

	snprintf(output, outLen, "%dT%02d:%02d:%02d", days, hours, minutes, seconds);
}

static int http_tasmota_json_status_STS(void* request, jsonCb_t printer, bool bAppendHeader) {
	char buff[20];
	time_t localTime = (time_t)NTP_GetCurrentTime();

	if (bAppendHeader) {
		printer(request, "\"StatusSTS\":");
	}
	printer(request, "{");
	format_date(buff, sizeof(buff), gmtime(&localTime));
	JSON_PrintKeyValue_String(request, printer, "Time", buff, true);
	format_time(g_secondsElapsed, buff, sizeof(buff));
	JSON_PrintKeyValue_String(request, printer, "Uptime", buff, true);
	//JSON_PrintKeyValue_String(request, printer, "Uptime", "30T02:59:30", true);
	JSON_PrintKeyValue_Int(request, printer, "UptimeSec", g_secondsElapsed, true);
	JSON_PrintKeyValue_Int(request, printer, "Heap", 25, true);
	JSON_PrintKeyValue_String(request, printer, "SleepMode", "Dynamic", true);
	JSON_PrintKeyValue_Int(request, printer, "Sleep", 10, true);
	JSON_PrintKeyValue_Int(request, printer, "LoadAvg", 99, true);
	JSON_PrintKeyValue_Int(request, printer, "MqttCount", 23, true);
#ifdef ENABLE_DRIVER_BATTERY
	if (DRV_IsRunning("Battery")) {
		printer(request, "\"Vcc\":%.4f,", Battery_lastreading(OBK_BATT_VOLTAGE) / 1000.00);
	}
#endif
	http_tasmota_json_power(request, printer);
	printer(request, ",");
	printer(request, "\"Wifi\":{"); // open WiFi
	JSON_PrintKeyValue_Int(request, printer, "AP", 1, true);
	JSON_PrintKeyValue_String(request, printer, "SSId", CFG_GetWiFiSSID(), true);
	JSON_PrintKeyValue_String(request, printer, "BSSId", "30:B5:C2:5D:70:72", true);
	JSON_PrintKeyValue_Int(request, printer, "Channel", 11, true);
	JSON_PrintKeyValue_String(request, printer, "Mode", "11n", true);
	JSON_PrintKeyValue_Int(request, printer, "RSSI", (HAL_GetWifiStrength() + 100) * 2, true);
	JSON_PrintKeyValue_Int(request, printer, "Signal", HAL_GetWifiStrength(), true);
	JSON_PrintKeyValue_Int(request, printer, "LinkCount", 21, true);
	JSON_PrintKeyValue_String(request, printer, "Downtime", "0T06:13:34", false);
	printer(request, "}"); // close WiFi
	printer(request, "}");
	return 0;
}
static int http_tasmota_json_status_TIM(void* request, jsonCb_t printer) {
	char buff[20];

	time_t localTime = (time_t)NTP_GetCurrentTime();
	time_t localUTC = (time_t)NTP_GetCurrentTimeWithoutOffset();
	printer(request, "\"StatusTIM\":{");
	format_date(buff, sizeof(buff), gmtime(&localUTC));
	JSON_PrintKeyValue_String(request, printer, "UTC", buff, true);
	format_date(buff, sizeof(buff), gmtime(&localTime));
	JSON_PrintKeyValue_String(request, printer, "Local", buff, true);
	JSON_PrintKeyValue_String(request, printer, "StartDST", "2022-03-27T02:00:00", true);
	JSON_PrintKeyValue_String(request, printer, "EndDST", "2022-10-30T03:00:00", true);
	JSON_PrintKeyValue_String(request, printer, "Timezone", "+01:00", true);
	JSON_PrintKeyValue_String(request, printer, "Sunrise", "07:50", true);
	JSON_PrintKeyValue_String(request, printer, "Sunset", "17:17", false);
	printer(request, "}");
	return 0;
}
// Test command: http://192.168.0.159/cm?cmnd=STATUS%202
static int http_tasmota_json_status_FWR(void* request, jsonCb_t printer) {

	printer(request, "\"StatusFWR\":{");
	JSON_PrintKeyValue_String(request, printer, "Version", DEVICENAME_PREFIX_FULL"_"USER_SW_VER, true);
	JSON_PrintKeyValue_String(request, printer, "BuildDateTime", __DATE__" "__TIME__, true);
	// NOTE: what is this value? It's not a reboot count
	JSON_PrintKeyValue_Int(request, printer, "Boot", 7, true);
	JSON_PrintKeyValue_String(request, printer, "Core", "0.0", true);
	// do not change it - Flasher scanner is using it, it should be obk on all platforms!
	JSON_PrintKeyValue_String(request, printer, "SDK", "obk", true);
	JSON_PrintKeyValue_Int(request, printer, "CpuFrequency", 80, true);
	JSON_PrintKeyValue_String(request, printer, "Hardware", PLATFORM_MCU_NAME, true);
	JSON_PrintKeyValue_String(request, printer, "CR", "465/699", false);
	printer(request, "}");
	return 0;
}
static int http_obk_json_channels(void* request, jsonCb_t printer) {
	int i;
	int iCnt = 0;
	char tmp[8];

	printer(request, "{");
	for (i = 0; i < CHANNEL_MAX; i++) {
		if (CHANNEL_IsInUse(i)) {
			if (iCnt) {
				printer(request, ",");
			}
			iCnt++;
			sprintf(tmp, "Ch%i", i);
			JSON_PrintKeyValue_Int(request, printer, tmp, CHANNEL_Get(i), false);
		}
	}
	printer(request, "}");
	return 0;
}
// Test command: http://192.168.0.159/cm?cmnd=STATUS%204
static int http_tasmota_json_status_MEM(void* request, jsonCb_t printer) {
	printer(request, "\"StatusMEM\":{");
	JSON_PrintKeyValue_Int(request, printer, "ProgramSize", 616, true);
	JSON_PrintKeyValue_Int(request, printer, "Free", 384, true);
	JSON_PrintKeyValue_Int(request, printer, "Heap", 25, true);
	JSON_PrintKeyValue_Int(request, printer, "ProgramFlashSize", 1024, true);
	JSON_PrintKeyValue_Int(request, printer, "FlashSize", 2048, true);
	JSON_PrintKeyValue_String(request, printer, "FlashChipId", "1540A1", true);
	JSON_PrintKeyValue_Int(request, printer, "FlashFrequency", 40, true);
	JSON_PrintKeyValue_Int(request, printer, "FlashMode", 3, true);
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
	JSON_PrintKeyValue_String(request, printer, "Drivers", "1,2,3,4,5,6,7,8,9,10,12,16,18,19,20,21,22,24,26,27,29,30,35,37,45", true);
	JSON_PrintKeyValue_String(request, printer, "Sensors", "1,2,3,4,5,6", false);
	printer(request, "}");
	return 0;
}
// Test command: http://192.168.0.159/cm?cmnd=STATUS%205
static int http_tasmota_json_status_NET(void* request, jsonCb_t printer) {
	char tmpStr[19];	// will be used for MAC string 6*3 chars (18 would be o.k, since last hex has no ":" ...)
	HAL_GetMACStr(tmpStr);

	printer(request, "\"StatusNET\":{");
	JSON_PrintKeyValue_String(request, printer, "Hostname", CFG_GetShortDeviceName(), true);
	JSON_PrintKeyValue_String(request, printer, "IPAddress", HAL_GetMyIPString(), true);
#if 0
	JSON_PrintKeyValue_String(request, printer, "Gateway", HAL_GetMyGatewayString(), true);
	JSON_PrintKeyValue_String(request, printer, "Subnetmask", HAL_GetMyMaskString(), true);
	JSON_PrintKeyValue_String(request, printer, "DNSServer1", HAL_GetMyDNSString(), true);
#else
	JSON_PrintKeyValue_String(request, printer, "Gateway", "192.168.0.1", true);
	JSON_PrintKeyValue_String(request, printer, "Subnetmask", "255.255.255.0", true);
	JSON_PrintKeyValue_String(request, printer, "DNSServer1", "192.168.0.1", true);
#endif
	JSON_PrintKeyValue_String(request, printer, "DNSServer2", "0.0.0.0", true);
	JSON_PrintKeyValue_String(request, printer, "Mac", tmpStr, true);
	JSON_PrintKeyValue_Int(request, printer, "Webserver", 2, true);
	JSON_PrintKeyValue_Int(request, printer, "HTTP_API", 1, true);
	JSON_PrintKeyValue_Int(request, printer, "WifiConfig", 4, true);
	printer(request, "\"WifiPower\":17.0");
	printer(request, "}");
	return 0;
}
// Test command: http://192.168.0.159/cm?cmnd=STATUS%206
static int http_tasmota_json_status_MQT(void* request, jsonCb_t printer) {

	printer(request, "\"StatusMQT\":{");
	JSON_PrintKeyValue_String(request, printer, "MqttHost", CFG_GetMQTTHost(), true);
	JSON_PrintKeyValue_Int(request, printer, "MqttPort", CFG_GetMQTTPort(), true);
	JSON_PrintKeyValue_String(request, printer, "MqttClientMask", "core", true);
	JSON_PrintKeyValue_String(request, printer, "MqttClient", CFG_GetMQTTClientId(), true);
	JSON_PrintKeyValue_String(request, printer, "MqttUser", CFG_GetMQTTUserName(), true);
	JSON_PrintKeyValue_Int(request, printer, "MqttCount", 23, true);
	JSON_PrintKeyValue_Int(request, printer, "MAX_PACKET_SIZE", 1200, true);
	JSON_PrintKeyValue_Int(request, printer, "KEEPALIVE", 30, true);
	JSON_PrintKeyValue_Int(request, printer, "SOCKET_TIMEOUT", 4, false);
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

	PIN_get_Relay_PWM_Count(&relayCount, &pwmCount, &dInputCount);

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
	printer(request, "\"Status\":{\"Module\":0,");
	JSON_PrintKeyValue_String(request, printer, "DeviceName", deviceName, true);
	printer(request, "\"FriendlyName\":[");
	if (relayCount == 0) {
		printer(request, "\"%s\"", friendlyName);
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
	JSON_PrintKeyValue_Int(request, printer, "Baudrate", 115200, true);
	JSON_PrintKeyValue_String(request, printer, "SerialConfig", "8N1", true);
	JSON_PrintKeyValue_String(request, printer, "GroupTopic", CFG_DeviceGroups_GetName(), true);
	JSON_PrintKeyValue_String(request, printer, "OtaUrl", "https://github.com/openshwprojects/OpenBK7231T_App/releases/latest", true);
	JSON_PrintKeyValue_String(request, printer, "RestartReason", "HardwareWatchdog", true);
	JSON_PrintKeyValue_Int(request, printer, "Uptime", g_secondsElapsed, true);
	struct tm* ltm;
	time_t ntpTime = 0; // if no NTP_time set, we will not change this value, but just stick to 0 and hence "fake" start of epoch 1970-01-01T00:00:00
	if (NTP_GetCurrentTimeWithoutOffset() > g_secondsElapsed) {	// would be negative else, leading to unwanted results when converted to (unsigned) time_t 
		ntpTime = (time_t)NTP_GetCurrentTimeWithoutOffset() - (time_t)g_secondsElapsed;
	}
	ltm = gmtime(&ntpTime);
	if (ltm != 0) {
		printer(request, "\"StartupUTC\":\"%04d-%02d-%02dT%02d:%02d:%02d\",", ltm->tm_year + 1900, ltm->tm_mon + 1, ltm->tm_mday, ltm->tm_hour, ltm->tm_min, ltm->tm_sec);
	}
	else {
	}
	JSON_PrintKeyValue_Int(request, printer, "Sleep", 50, true);
	JSON_PrintKeyValue_Int(request, printer, "CfgHolder", 4617, true);
	JSON_PrintKeyValue_Int(request, printer, "BootCount", 22, true);
	JSON_PrintKeyValue_String(request, printer, "BCResetTime", "2022-01-27T16:10:56", true);
	JSON_PrintKeyValue_Int(request, printer, "SaveCount", 1235, true);
	JSON_PrintKeyValue_String(request, printer, "SaveAddress", "F9000", false);
	printer(request, "}");

	printer(request, ",");

	http_tasmota_json_status_FWR(request, printer);

	printer(request, ",");


	// Test command: http://192.168.0.159/cm?cmnd=STATUS%203
	printer(request, "\"StatusLOG\":{");
	printer(request, "\"SerialLog\":2,");
	printer(request, "\"WebLog\":2,");
	printer(request, "\"MqttLog\":0,");
	printer(request, "\"SysLog\":0,");
	printer(request, "\"LogHost\":\"\",");
	printer(request, "\"LogPort\":514,");
	printer(request, "\"SSId1\":\"%s\",", CFG_GetWiFiSSID());
	printer(request, "\"SSId2\":\"%s\",", CFG_GetWiFiSSID2());
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
// drv_tuyaMCU.c
int http_obk_json_dps(int id, void* request, jsonCb_t printer);

int JSON_ProcessCommandReply(const char* cmd, const char* arg, void* request, jsonCb_t printer, int flags) {
	int i;
	long int* pAllGenericFlags = (long int*)&g_cfg.genericFlags;

	if (!wal_strnicmp(cmd, "POWER", 5)) {

		printer(request, "{");
		http_tasmota_json_power(request, printer);
		printer(request, "}");
		if (flags == COMMAND_FLAG_SOURCE_MQTT) {
			MQTT_PublishPrinterContentsToStat((struct obk_mqtt_publishReplyPrinter_s*)request, "RESULT");
		}
	}
	else if (!wal_strnicmp(cmd, "SensorRetain", 12)) {
		printer(request, "{");
		if (CFG_HasFlag(OBK_PUBLISH_FLAG_RETAIN))
		{
			JSON_PrintKeyValue_String(request, printer, "SensorRetain", "ON", false);
		}
		else
		{
			JSON_PrintKeyValue_String(request, printer, "SensorRetain", "OFF", false);
		}

		printer(request, "}");
	}
	else if (!wal_strnicmp(cmd, "Prefix1", 7)) {
		//TODO 
		// Prefix1 	1 = reset MQTT command subscription prefix to firmware default (SUB_PREFIX) and restart
		// <value> = set MQTT command subscription prefix and restart
		// Prefix2 	1 = reset MQTT status prefix to firmware default (PUB_PREFIX) and restart
		// <value> = set MQTT status prefix and restart
		// Prefix3 	1 = Reset MQTT telemetry prefix to firmware default (PUB_PREFIX2) and restart
		// <value> = set MQTT telemetry prefix and restart
		printer(request, "{");
		JSON_PrintKeyValue_String(request, printer, "Prefix1", "cmnd", false);
		printer(request, "}");
	}
	else if (!wal_strnicmp(cmd, "Prefix2", 7)) {
		printer(request, "{");
		JSON_PrintKeyValue_String(request, printer, "Prefix2", "stat", false);
		printer(request, "}");
	}
	else if (!wal_strnicmp(cmd, "Prefix3", 7)) {
		printer(request, "{");
		JSON_PrintKeyValue_String(request, printer, "Prefix3", "tele", false);
		printer(request, "}");
	}
	else if (!wal_strnicmp(cmd, "StateText1", 10)) {
		//TODO 
		//StateText<x> 	<value> = set state text (<x> = 1..4)
		//  1 = OFF state text
		//	2 = ON state text
		//	3 = TOGGLE state text
		//	4 = HOLD state text
		printer(request, "{");
		JSON_PrintKeyValue_String(request, printer, "StateText1", "OFF", false);
		printer(request, "}");
	}
	else if (!wal_strnicmp(cmd, "StateText2", 10)) {
		printer(request, "{");
		JSON_PrintKeyValue_String(request, printer, "StateText2", "ON", false);
		printer(request, "}");
	}
	else if (!wal_strnicmp(cmd, "StateText3", 10)) {
		printer(request, "{");
		JSON_PrintKeyValue_String(request, printer, "StateText3", "TOGGLE", false);
		printer(request, "}");
	}
	else if (!wal_strnicmp(cmd, "StateText4", 10)) {
		printer(request, "{");
		JSON_PrintKeyValue_String(request, printer, "StateText4", "HOLD", false);
		printer(request, "}");
	}
	else if (!wal_strnicmp(cmd, "FullTopic", 9)) {
		printer(request, "{");
		JSON_PrintKeyValue_String(request, printer, "FullTopic", "%%prefix%%/%%topic%%", false);
		printer(request, "}");
	}
	else if (!wal_strnicmp(cmd, "SwitchTopic", 11)) {
		printer(request, "{");
		JSON_PrintKeyValue_String(request, printer, "SwitchTopic", "0", false);
		printer(request, "}");
	}
	else if (!wal_strnicmp(cmd, "ButtonTopic", 11)) {
		printer(request, "{");
		JSON_PrintKeyValue_String(request, printer, "ButtonTopic", "0", false);
		printer(request, "}");
	}
	else if (!wal_strnicmp(cmd, "MqttRetry", 9)) {
		printer(request, "{");
		JSON_PrintKeyValue_String(request, printer, "MqttRetry", "1", false);
		printer(request, "}");
	}
	else if (!wal_strnicmp(cmd, "TelePeriod", 10)) {
		printer(request, "{");
		JSON_PrintKeyValue_String(request, printer, "TelePeriod", "300", false);
		printer(request, "}");
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
			MQTT_PublishPrinterContentsToStat((struct obk_mqtt_publishReplyPrinter_s*)request, "RESULT");
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
			MQTT_PublishPrinterContentsToStat((struct obk_mqtt_publishReplyPrinter_s*)request, "RESULT");
		}
	}
	else if (!wal_strnicmp(cmd, "Color", 5) || !wal_strnicmp(cmd, "HsbColor", 8)) {
		printer(request, "{");
		//if (*arg == 0) {
		//	http_tasmota_json_Colo(request, printer);
		//}
		//else {
		http_tasmota_json_power(request, printer);
		//}
		printer(request, "}");
		if (flags == COMMAND_FLAG_SOURCE_MQTT) {
			MQTT_PublishPrinterContentsToStat((struct obk_mqtt_publishReplyPrinter_s*)request, "RESULT");
		}
	}
	else if (!wal_strnicmp(cmd, "STATE", 5)) {
		http_tasmota_json_status_STS(request, printer, false);
		if (flags == COMMAND_FLAG_SOURCE_MQTT) {
			MQTT_PublishPrinterContentsToStat((struct obk_mqtt_publishReplyPrinter_s*)request, "RESULT");
		}
		if (flags == COMMAND_FLAG_SOURCE_TELESENDER) {
			MQTT_PublishPrinterContentsToTele((struct obk_mqtt_publishReplyPrinter_s*)request, "STATE");
		}
	}
	else if (!wal_strnicmp(cmd, "SENSOR", 5)) {
		// not a Tasmota command, but still required for us
		http_tasmota_json_status_SNS(request, printer, false);
		if (flags == COMMAND_FLAG_SOURCE_TELESENDER) {
			MQTT_PublishPrinterContentsToTele((struct obk_mqtt_publishReplyPrinter_s*)request, "SENSOR");
		}
	}
	else if (!wal_strnicmp(cmd, "STATUS", 6)) {
		if (!stricmp(arg, "8") || !stricmp(arg, "10")) {
			printer(request, "{");
			http_tasmota_json_status_SNS(request, printer, true);
			printer(request, "}");
			if (flags == COMMAND_FLAG_SOURCE_MQTT) {
				if (arg[0] == '8') {
					MQTT_PublishPrinterContentsToStat((struct obk_mqtt_publishReplyPrinter_s*)request, "STATUS8");
				}
				else {
					MQTT_PublishPrinterContentsToStat((struct obk_mqtt_publishReplyPrinter_s*)request, "STATUS10");
				}
			}
		}
		else if (!stricmp(arg, "6")) {
			printer(request, "{");
			http_tasmota_json_status_MQT(request, printer);
			printer(request, "}");
			if (flags == COMMAND_FLAG_SOURCE_MQTT) {
				MQTT_PublishPrinterContentsToStat((struct obk_mqtt_publishReplyPrinter_s*)request, "STATUS6");
			}
		}
		else if (!stricmp(arg, "7")) {
			printer(request, "{");
			http_tasmota_json_status_TIM(request, printer);
			printer(request, "}");
			if (flags == COMMAND_FLAG_SOURCE_MQTT) {
				MQTT_PublishPrinterContentsToStat((struct obk_mqtt_publishReplyPrinter_s*)request, "STATUS7");
			}
		}
		else if (!stricmp(arg, "5")) {
			printer(request, "{");
			http_tasmota_json_status_NET(request, printer);
			printer(request, "}");
			if (flags == COMMAND_FLAG_SOURCE_MQTT) {
				MQTT_PublishPrinterContentsToStat((struct obk_mqtt_publishReplyPrinter_s*)request, "STATUS5");
			}
		}
		else if (!stricmp(arg, "4")) {
			printer(request, "{");
			http_tasmota_json_status_MEM(request, printer);
			printer(request, "}");
			if (flags == COMMAND_FLAG_SOURCE_MQTT) {
				MQTT_PublishPrinterContentsToStat((struct obk_mqtt_publishReplyPrinter_s*)request, "STATUS4");
			}
		}
		else if (!stricmp(arg, "11")) {
			printer(request, "{");
			http_tasmota_json_status_STS(request, printer, true);
			printer(request, "}");
			if (flags == COMMAND_FLAG_SOURCE_MQTT) {
				MQTT_PublishPrinterContentsToStat((struct obk_mqtt_publishReplyPrinter_s*)request, "STATUS11");
			}
		}
		else if (!stricmp(arg, "2")) {
			printer(request, "{");
			http_tasmota_json_status_FWR(request, printer);
			printer(request, "}");
			if (flags == COMMAND_FLAG_SOURCE_MQTT) {
				MQTT_PublishPrinterContentsToStat((struct obk_mqtt_publishReplyPrinter_s*)request, "STATUS2");
			}
		}
		else {
			http_tasmota_json_status_generic(request, printer);
			if (flags == COMMAND_FLAG_SOURCE_MQTT) {
				MQTT_PublishPrinterContentsToStat((struct obk_mqtt_publishReplyPrinter_s*)request, "STATUS");
			}
		}
	}
	else if (!wal_strnicmp(cmd, "SetChannelType", 14)) {
		// OBK-specific
		i = atoi(arg);
		i = CHANNEL_GetType(i);

		printer(request, "%i", i);
	}
	else if (!wal_strnicmp(cmd, "GetChannel", 10) || !wal_strnicmp(cmd, "SetChannel", 10) || !wal_strnicmp(cmd, "AddChannel", 10)) {
		// OBK-specific
		i = atoi(arg);
		i = CHANNEL_Get(i);

		printer(request, "%i", i);
	}
	else if (!wal_strnicmp(cmd, "led_basecolor_rgb", 17)) {
		// OBK-specific
		char tmp[16];
		LED_GetBaseColorString(tmp);
		printer(request, "{");
		JSON_PrintKeyValue_String(request, printer, "led_basecolor_rgb", tmp, false);
		printer(request, "}");
	}
	else if (!wal_strnicmp(cmd, "MQTTClient", 8)) {
		printer(request, "{");
		JSON_PrintKeyValue_String(request, printer, "MQTTClient", CFG_GetMQTTClientId(), false);
		printer(request, "}");
	}
	else if (!wal_strnicmp(cmd, "MQTTHost", 8)) {
		printer(request, "{");
		JSON_PrintKeyValue_String(request, printer, "MQTTHost", CFG_GetMQTTHost(), false);
		printer(request, "}");
	}
	else if (!wal_strnicmp(cmd, "MQTTUser", 8)) {
		printer(request, "{");
		JSON_PrintKeyValue_String(request, printer, "MQTTUser", CFG_GetMQTTUserName(), false);
		printer(request, "}");
	}
	else if (!wal_strnicmp(cmd, "MqttPassword", 12)) {
		printer(request, "{");
		JSON_PrintKeyValue_String(request, printer, "MqttPassword", "****", false);
		printer(request, "}");
	}
	else if (!wal_strnicmp(cmd, "SSID1", 5)) {
		printer(request, "{");
		JSON_PrintKeyValue_String(request, printer, "SSID1", CFG_GetWiFiSSID(), false);
		printer(request, "}");
	}
	else if (!wal_strnicmp(cmd, "LED_Map", 7)) {
		printer(request, "{");
		printer(request, "\"Map\":[%i,%i,%i,%i,%i]",
			(int)g_cfg.ledRemap.r, (int)g_cfg.ledRemap.g, (int)g_cfg.ledRemap.b, (int)g_cfg.ledRemap.c, (int)g_cfg.ledRemap.w);
		printer(request, "}");
	}
	else if (!wal_strnicmp(cmd, "Flags", 5)) {
		printer(request, "{");
		printer(request, "\"Flags\":\"%ld\"", *pAllGenericFlags);
		printer(request, "}");
	}
	else if (!wal_strnicmp(cmd, "Ch", 2)) {
		http_obk_json_channels(request, printer);
	}
#ifndef OBK_DISABLE_ALL_DRIVERS
#if ENABLE_DRIVER_TUYAMCU
	else if (!wal_strnicmp(cmd, "Dp", 2)) {
		int id = -1;
		if (isdigit((int)cmd[2])) {
			sscanf(cmd + 2, "%i", &id);
		}
		http_obk_json_dps(id,request, printer);
		if (flags == COMMAND_FLAG_SOURCE_MQTT) {
			MQTT_PublishPrinterContentsToStat((struct obk_mqtt_publishReplyPrinter_s*)request, "DP");
		}
	}
#endif
#endif
	else {
		printer(request, "{");
		printer(request, "}");
	}

	return 0;
}
// close for ENABLE_TASMOTA_JSON
#endif



