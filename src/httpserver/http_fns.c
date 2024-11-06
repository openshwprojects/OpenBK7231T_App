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

static char SUBMIT_AND_END_FORM[] = "<br><input type=\"submit\" value=\"Submit\"></form>";

#ifdef WINDOWS
// nothing
#elif PLATFORM_BL602
#include <bl_sys.h>
#include <bl_adc.h>     //  For BL602 ADC HAL
#include <bl602_adc.h>  //  For BL602 ADC Standard Driver
#include <bl602_glb.h>  //  For BL602 Global Register Standard Driver
#include <wifi_mgmr_ext.h> //For BL602 WiFi AP Scan
#elif PLATFORM_W600 || PLATFORM_W800

#elif PLATFORM_XR809
#include <image/flash.h>
#elif defined(PLATFORM_BK7231N)
// tuya-iotos-embeded-sdk-wifi-ble-bk7231n/sdk/include/tuya_hal_storage.h
#include "tuya_hal_storage.h"
#include "BkDriverFlash.h"
#include "temp_detect_pub.h"
#elif defined(PLATFORM_LN882H)
#elif defined(PLATFORM_ESPIDF)
#include "esp_wifi.h"
#include "esp_system.h"
#else
// REALLY? A typo in Tuya SDK? Storge?
// tuya-iotos-embeded-sdk-wifi-ble-bk7231t/platforms/bk7231t/tuya_os_adapter/include/driver/tuya_hal_storge.h
#include "tuya_hal_storge.h"
#include "BkDriverFlash.h"
#include "temp_detect_pub.h"
#endif

#if defined(PLATFORM_BK7231T) || defined(PLATFORM_BK7231N)
int tuya_os_adapt_wifi_all_ap_scan(AP_IF_S** ap_ary, unsigned int* num);
int tuya_os_adapt_wifi_release_ap(AP_IF_S* ap);
#endif


const char* g_typesOffLowMidHigh[] = { "Off","Low","Mid","High" };
const char* g_typesOffLowMidHighHighest[] = { "Off", "Low","Mid","High","Highest" };
const char* g_typesOffLowestLowMidHighHighest[] = { "Off", "Lowest", "Low", "Mid", "High", "Highest" };
const char* g_typesLowMidHighHighest[] = { "Low","Mid","High","Highest" };
const char* g_typesOffOnRemember[] = { "Off", "On", "Remember" };
const char* g_typeLowMidHigh[] = { "Low","Mid","High" };
const char* g_typesLowestLowMidHighHighest[] = { "Lowest", "Low", "Mid", "High", "Highest" };;

#define ADD_OPTION(t,a) if(type == t) { *numTypes = sizeof(a)/sizeof(a[0]); return a; }

const char **Channel_GetOptionsForChannelType(int type, int *numTypes) {
	ADD_OPTION(ChType_OffLowMidHigh, g_typesOffLowMidHigh);
	ADD_OPTION(ChType_OffLowestLowMidHighHighest, g_typesOffLowestLowMidHighHighest);
	ADD_OPTION(ChType_LowestLowMidHighHighest, g_typesLowestLowMidHighHighest);
	ADD_OPTION(ChType_OffLowMidHighHighest, g_typesOffLowMidHighHighest);
	ADD_OPTION(ChType_LowMidHighHighest, g_typesLowMidHighHighest);
	ADD_OPTION(ChType_OffOnRemember, g_typesOffOnRemember);
	ADD_OPTION(ChType_LowMidHigh, g_typeLowMidHigh);
	
	*numTypes = 0;
	return 0;
}

unsigned char hexdigit(char hex) {
	return (hex <= '9') ? hex - '0' :
		toupper((unsigned char)hex) - 'A' + 10;
}

unsigned char hexbyte(const char* hex) {
	return (hexdigit(*hex) << 4) | hexdigit(*(hex + 1));
}

int http_fn_empty_url(http_request_t* request) {
	poststr(request, "HTTP/1.1 302 OK\nLocation: /index\nConnection: close\n\n");
	poststr(request, NULL);
	return 0;
}

void postFormAction(http_request_t* request, char* action, char* value) {
	//"<form action=\"cfg_pins\"><input type=\"submit\" value=\"Configure Module\"/></form>"
	hprintf255(request, "<form action=\"%s\"><input type=\"submit\" value=\"%s\"/></form>", action, value);
}

void poststr_h2(http_request_t* request, const char* content) {
	hprintf255(request, "<h2>%s</h2>", content);
}
void poststr_h4(http_request_t* request, const char* content) {
	hprintf255(request, "<h4>%s</h4>", content);
}

/// @brief Generate a pair of label and field elements for Name type entry. The field is limited to entry of a-zA-Z0-9_- characters.
/// @param request 
/// @param label 
/// @param fieldId This also gets used as the field name
/// @param value 
/// @param preContent
void add_label_name_field(http_request_t* request, char* label, char* fieldId, const char* value, char* preContent) {
	if (strlen(preContent) > 0) {
		poststr(request, preContent);
	}

	hprintf255(request, "<label for=\"%s\">%s:</label><br>", fieldId, label);
	hprintf255(request, "<input type=\"text\" id=\"%s\" name=\"%s\" value=\"%s\" ", fieldId, fieldId, value);
	poststr(request, "pattern=\"^[a-zA-Z0-9_-]+$\" title=\"Only alphanumerics, underscore and hyphen characters allowed.\">");
}

/// @brief Generate a pair of label and field elements.
/// @param request 
/// @param label 
/// @param fieldId This also gets used as the field name
/// @param value 
/// @param preContent 
void add_label_input(http_request_t* request, char* inputType, char* label, char* fieldId, const char* value, char* preContent) {
	if (strlen(preContent) > 0) {
		poststr(request, preContent);
	}

	hprintf255(request, "<label for=\"%s\">%s:</label><br>", fieldId, label);
	hprintf255(request, "<input type=\"%s\" id=\"%s\" name=\"%s\" value=\"", inputType, fieldId, fieldId);
	poststr(request, value);
	hprintf255(request, "\">");
}

/// @brief Generates a pair of label and text field elements.
/// @param request 
/// @param label Label for the field
/// @param fieldId Field id, this also gets used as the name
/// @param value String value
/// @param preContent Content before the label
void add_label_text_field(http_request_t* request, char* label, char* fieldId, const char* value, char* preContent) {
	add_label_input(request, "text", label, fieldId, value, preContent);
}

/// @brief Generates a pair of label and text field elements.
/// @param request 
/// @param label Label for the field
/// @param fieldId Field id, this also gets used as the name
/// @param value String value
/// @param preContent Content before the label
void add_label_password_field(http_request_t* request, char* label, char* fieldId, const char* value, char* preContent) {
	add_label_input(request, "password", label, fieldId, value, preContent);
}

/// @brief Generate a pair of label and numeric field elements.
/// @param request 
/// @param label Label for the field
/// @param fieldId Field id, this also gets used as the name
/// @param value Integer value
/// @param preContent Content before the label
void add_label_numeric_field(http_request_t* request, char* label, char* fieldId, int value, char* preContent) {
	char strValue[32];
	sprintf(strValue, "%i", value);
	add_label_input(request, "number", label, fieldId, strValue, preContent);
}

int http_fn_testmsg(http_request_t* request) {
	poststr(request, "This is just a test msg\n\n");
	poststr(request, NULL);
	return 0;

}

// bit mask telling which channels are hidden from HTTP
// If given bit is set, then given channel is hidden
extern int g_hiddenChannels;

int http_fn_index(http_request_t* request) {
	int j, i, ch1, ch2;
	char tmpA[128];
	int bRawPWMs;
	bool bForceShowRGBCW;
	float fValue;
	int iValue;
	bool bForceShowRGB;
	const char* inputName;
	int channelType;

	bRawPWMs = CFG_HasFlag(OBK_FLAG_LED_RAWCHANNELSMODE);
	bForceShowRGBCW = CFG_HasFlag(OBK_FLAG_LED_FORCESHOWRGBCWCONTROLLER);
	bForceShowRGB = CFG_HasFlag(OBK_FLAG_LED_FORCE_MODE_RGB);

	// user override is always stronger, so if no override set
	if (bForceShowRGB == false && bForceShowRGBCW == false) {
#ifndef OBK_DISABLE_ALL_DRIVERS
		if (DRV_IsRunning("SM16703P")) {
			bForceShowRGB = true;
		}
		else
#endif
		if (LED_IsLedDriverChipRunning()) {
			bForceShowRGBCW = true;
		}
	}
	http_setup(request, httpMimeTypeHTML);	//Add mimetype regardless of the request

	// use ?state URL parameter to only request current state
	if (!http_getArg(request->url, "state", tmpA, sizeof(tmpA))) {
		http_html_start(request, NULL);

		poststr(request, "<div id=\"changed\">");
#if defined(PLATFORM_BEKEN) || defined(WINDOWS)
		if (DRV_IsRunning("PWMToggler")) {
			DRV_Toggler_ProcessChanges(request);
		}
#endif
#if defined(PLATFORM_BEKEN) || defined(WINDOWS)
		if (DRV_IsRunning("httpButtons")) {
			DRV_HTTPButtons_ProcessChanges(request);
		}
#endif
		if (http_getArg(request->url, "tgl", tmpA, sizeof(tmpA))) {
			j = atoi(tmpA);
			if (j == SPECIAL_CHANNEL_LEDPOWER) {
				hprintf255(request, "<h3>Toggled LED power!</h3>", j);
			}
			else {
				hprintf255(request, "<h3>Toggled %s!</h3>", CHANNEL_GetLabel(j));
			}
			CHANNEL_Toggle(j);
		}
		if (http_getArg(request->url, "on", tmpA, sizeof(tmpA))) {
			j = atoi(tmpA);
			hprintf255(request, "<h3>Enabled %s!</h3>", CHANNEL_GetLabel(j));
			CHANNEL_Set(j, 255, 1);
		}
		if (http_getArg(request->url, "rgb", tmpA, sizeof(tmpA))) {
			hprintf255(request, "<h3>Set RGB to %s!</h3>", tmpA);
			LED_SetBaseColor(0, "led_basecolor", tmpA, 0);
			// auto enable - but only for changes made from WWW panel
			// This happens when users changes COLOR
			if (CFG_HasFlag(OBK_FLAG_LED_AUTOENABLE_ON_WWW_ACTION)) {
				LED_SetEnableAll(true);
			}
		}

		if (http_getArg(request->url, "off", tmpA, sizeof(tmpA))) {
			j = atoi(tmpA);
			hprintf255(request, "<h3>Disabled %s!</h3>", CHANNEL_GetLabel(j));
			CHANNEL_Set(j, 0, 1);
		}
		if (http_getArg(request->url, "pwm", tmpA, sizeof(tmpA))) {
			int newPWMValue = atoi(tmpA);
			http_getArg(request->url, "pwmIndex", tmpA, sizeof(tmpA));
			j = atoi(tmpA);
			if (j == SPECIAL_CHANNEL_TEMPERATURE) {
				hprintf255(request, "<h3>Changed Temperature to %i!</h3>", newPWMValue);
			}
			else {
				hprintf255(request, "<h3>Changed pwm %i to %i!</h3>", j, newPWMValue);
			}
			CHANNEL_Set(j, newPWMValue, 1);

			if (j == SPECIAL_CHANNEL_TEMPERATURE) {
				// auto enable - but only for changes made from WWW panel
				// This happens when users changes TEMPERATURE
				if (CFG_HasFlag(OBK_FLAG_LED_AUTOENABLE_ON_WWW_ACTION)) {
					LED_SetEnableAll(true);
				}
			}
		}
		if (http_getArg(request->url, "dim", tmpA, sizeof(tmpA))) {
			int newDimmerValue = atoi(tmpA);
			http_getArg(request->url, "dimIndex", tmpA, sizeof(tmpA));
			j = atoi(tmpA);
			if (j == SPECIAL_CHANNEL_BRIGHTNESS) {
				hprintf255(request, "<h3>Changed LED brightness to %i!</h3>", newDimmerValue);
			}
			else {
				hprintf255(request, "<h3>Changed dimmer %i to %i!</h3>", j, newDimmerValue);
			}
			CHANNEL_Set(j, newDimmerValue, 1);

			if (j == SPECIAL_CHANNEL_BRIGHTNESS) {
				// auto enable - but only for changes made from WWW panel
				// This happens when users changes DIMMER
				if (CFG_HasFlag(OBK_FLAG_LED_AUTOENABLE_ON_WWW_ACTION)) {
					LED_SetEnableAll(true);
				}
			}
		}
		if (http_getArg(request->url, "set", tmpA, sizeof(tmpA))) {
			int newSetValue = atoi(tmpA);
			http_getArg(request->url, "setIndex", tmpA, sizeof(tmpA));
			j = atoi(tmpA);
			hprintf255(request, "<h3>Changed channel %s to %i!</h3>", CHANNEL_GetLabel(j), newSetValue);
			CHANNEL_Set(j, newSetValue, 1);
		}
		if (http_getArg(request->url, "restart", tmpA, sizeof(tmpA))) {
			poststr(request, "<h5> Module will restart soon</h5>");
			RESET_ScheduleModuleReset(3);
		}
		if (http_getArg(request->url, "unsafe", tmpA, sizeof(tmpA))) {
			poststr(request, "<h5> Will try to do unsafe init in few seconds</h5>");
			MAIN_ScheduleUnsafeInit(3);
		}
		poststr(request, "</div>"); // end div#change
		poststr(request, "<div id=\"state\">"); // replaceable content follows
	}

	if (!CFG_HasFlag(OBK_FLAG_HTTP_NO_ONOFF_WORDS)){
		poststr(request, "<table>");	//Table default to 100% width in stylesheet
		for (i = 0; i < CHANNEL_MAX; i++) {

			channelType = CHANNEL_GetType(i);
			// check ability to hide given channel from gui
			if (BIT_CHECK(g_hiddenChannels, i)) {
				continue; // hidden
			}
			bool bToggleInv = channelType == ChType_Toggle_Inv;
			if (h_isChannelRelay(i) || channelType == ChType_Toggle) {
				if (i <= 1) {
					hprintf255(request, "<tr>");
				}
				if (CHANNEL_Check(i) != bToggleInv) {
					poststr(request, "<td class='on'>ON</td>");
				}
				else {
					poststr(request, "<td class='off'>OFF</td>");
				}
				if (i == CHANNEL_MAX - 1) {
					poststr(request, "</tr>");
				}
			}
		}
		poststr(request, "</table>");
	}
	poststr(request, "<table>");	//Table default to 100% width in stylesheet
	for (i = 0; i < CHANNEL_MAX; i++) {


		// check ability to hide given channel from gui
		if (BIT_CHECK(g_hiddenChannels, i)) {
			continue; // hidden
		}

		channelType = CHANNEL_GetType(i);
		bool bToggleInv = channelType == ChType_Toggle_Inv;
		if (h_isChannelRelay(i) || channelType == ChType_Toggle || bToggleInv) {
			const char* c;
			const char* prefix;
			if (i <= 1) {
				hprintf255(request, "<tr>");
			}
			if (CHANNEL_Check(i) != bToggleInv) {
				c = "bgrn";
			}
			else {
				c = "bred";
			}
			poststr(request, "<td><form action=\"index\">");
			hprintf255(request, "<input type=\"hidden\" name=\"tgl\" value=\"%i\">", i);

			if (CHANNEL_ShouldAddTogglePrefixToUI(i)) {
				prefix = "Toggle ";
			}
			else {
				prefix = "";
			}

			hprintf255(request, "<input class=\"%s\" type=\"submit\" value=\"%s%s\"/></form></td>", c, prefix, CHANNEL_GetLabel(i));
			if (i == CHANNEL_MAX - 1) {
				poststr(request, "</tr>");
			}
		}
	}
	poststr(request, "</table>");
	poststr(request, "<table>");	//Table default to 100% width in stylesheet
	for (i = 0; i < PLATFORM_GPIO_MAX; i++) {
		int role;

		role = PIN_GetPinRoleForPinIndex(i);

		if (IS_PIN_DHT_ROLE(role)) {
			// DHT pin has two channels - temperature and humidity
			poststr(request, "<tr><td>");
			ch1 = PIN_GetPinChannelForPinIndex(i);
			ch2 = PIN_GetPinChannel2ForPinIndex(i);
			iValue = CHANNEL_Get(ch1);
			hprintf255(request, "Sensor %s on pin %i temperature %.2fC", PIN_RoleToString(role), i, (float)(iValue * 0.1f));
			iValue = CHANNEL_Get(ch2);
			hprintf255(request, ", humidity %.1f%%<br>", (float)iValue);
			if (ch1 == ch2) {
				hprintf255(request, "WARNING: you have the same channel set twice for DHT, please fix in pins config, set two different channels");
			}
			poststr(request, "</td></tr>");
		}
	}
	for (i = 0; i < CHANNEL_MAX; i++) {
		const char **types;
		int numTypes;

		// check ability to hide given channel from gui
		if (BIT_CHECK(g_hiddenChannels, i)) {
			continue; // hidden
		}

		channelType = CHANNEL_GetType(i);
		if (channelType == ChType_TimerSeconds) {
			iValue = CHANNEL_Get(i);

			poststr(request, "<tr><td>");
			hprintf255(request, "Timer Channel %s value ", CHANNEL_GetLabel(i));
			if (iValue < 60) {
				hprintf255(request, "%i seconds<br>", iValue);
			}
			else if (iValue < 3600) {
				int minutes = iValue / 60;
				int seconds = iValue % 60;
				hprintf255(request, "%i minutes %i seconds<br>", minutes, seconds);
			}
			else {
				int hours = iValue / 3600;
				int remainingSeconds = iValue % 3600;
				int minutes = remainingSeconds / 60;
				int seconds = remainingSeconds % 60;
				hprintf255(request, "%i hours %i minutes %i seconds<br>", hours, minutes, seconds);
			}
			poststr(request, "</td></tr>");

		} else if (channelType == ChType_ReadOnlyLowMidHigh) {
			const char* types[] = { "Low","Mid","High" };
			iValue = CHANNEL_Get(i);
			poststr(request, "<tr><td>");
			if (iValue >= 0 && iValue <= 2) {
				hprintf255(request, "Channel %s = %s", CHANNEL_GetLabel(i), types[iValue]);
			}
			else {
				hprintf255(request, "Channel %s = %i", CHANNEL_GetLabel(i), iValue);
			}
			poststr(request, "</td></tr>");
		}
		else if ((types = Channel_GetOptionsForChannelType(channelType, &numTypes)) != 0) {
			const char *what;
			if (channelType == ChType_OffOnRemember) {
				what = "memory";
			}
			else {
				what = "speed";
			}
			
			iValue = CHANNEL_Get(i);

			poststr(request, "<tr><td>");
			hprintf255(request, "<p>Select %s:</p><form action=\"index\">", what);
			hprintf255(request, "<input type=\"hidden\" name=\"setIndex\" value=\"%i\">", i);
			for (j = 0; j < numTypes; j++) {
				const char* check;
				if (j == iValue)
					check = "checked";
				else
					check = "";
				hprintf255(request, "<input type=\"radio\" name=\"set\" value=\"%i\" onchange=\"this.form.submit()\" %s>%s", j, check, types[j]);
			}
			hprintf255(request, "</form>");
			poststr(request, "</td></tr>");

		}
		else if (channelType == ChType_TextField) {
			iValue = CHANNEL_Get(i);

			poststr(request, "<tr><td>");
			hprintf255(request, "<p>Change channel %s value:</p><form action=\"index\">", CHANNEL_GetLabel(i));
			hprintf255(request, "<input type=\"hidden\" name=\"setIndex\" value=\"%i\">", i);
			hprintf255(request, "<input type=\"number\" name=\"set\" value=\"%i\" onblur=\"this.form.submit()\">", iValue);
			hprintf255(request, "<input type=\"submit\" value=\"Set!\"/></form>");
			hprintf255(request, "</form>");
			poststr(request, "</td></tr>");

		}
		else if (channelType == ChType_ReadOnly) {
			iValue = CHANNEL_Get(i);

			poststr(request, "<tr><td>");
			hprintf255(request, "Channel %s = %i", CHANNEL_GetLabel(i), iValue);
			poststr(request, "</td></tr>");
		}
		else if (channelType == ChType_Motion || channelType == ChType_Motion_n) {
			iValue = CHANNEL_Get(i);

			poststr(request, "<tr><td>");
			if (iValue == (channelType != ChType_Motion)) {
				hprintf255(request, "No motion (ch %i)", i);
			}
			else {
				hprintf255(request, "Motion! (ch %i)", i);
			}
			poststr(request, "</td></tr>");
		}
		else if (channelType == ChType_OpenClosed) {
			iValue = CHANNEL_Get(i);

			poststr(request, "<tr><td>");
			if (iValue) {
				hprintf255(request, "CLOSED (ch %i)", i);
			}
			else {
				hprintf255(request, "OPEN (ch %i)", i);
			}
			poststr(request, "</td></tr>");
		}
		else if (channelType == ChType_OpenClosed_Inv) {
			iValue = CHANNEL_Get(i);

			poststr(request, "<tr><td>");
			if (!iValue) {
				hprintf255(request, "CLOSED (ch %i)", i);
			}
			else {
				hprintf255(request, "OPEN (ch %i)", i);
			}
			poststr(request, "</td></tr>");
		}
		else if (h_isChannelRelay(i) || channelType == ChType_Toggle || channelType == ChType_Toggle_Inv) {
			// HANDLED ABOVE in previous loop
		}
		else if ((bRawPWMs && h_isChannelPWM(i)) || (channelType == ChType_Dimmer) || (channelType == ChType_Dimmer256) || (channelType == ChType_Dimmer1000)) {
			int maxValue;
			// PWM and dimmer both use a slider control
			inputName = h_isChannelPWM(i) ? "pwm" : "dim";
			int pwmValue;

			if (channelType == ChType_Dimmer256) {
				maxValue = 255;
			}
			else if (channelType == ChType_Dimmer1000) {
				maxValue = 1000;
			}
			else {
				maxValue = 100;
			}
			pwmValue = CHANNEL_Get(i);
			poststr(request, "<tr><td>");
			hprintf255(request, "Channel %s:<br><form action=\"index\" id=\"form%i\">", CHANNEL_GetLabel(i), i);
			hprintf255(request, "<input type=\"range\" min=\"0\" max=\"%i\" name=\"%s\" id=\"slider%i\" value=\"%i\" onchange=\"this.form.submit()\">", maxValue, inputName, i, pwmValue);
			hprintf255(request, "<input type=\"hidden\" name=\"%sIndex\" value=\"%i\">", inputName, i);
			hprintf255(request, "<input type=\"submit\" class='disp-none' value=\"Toggle %s\"/></form>", CHANNEL_GetLabel(i));
			poststr(request, "</td></tr>");
		}
		else if (channelType == ChType_OffDimBright) {
			const char* types[] = { "Off","Dim","Bright" };
			iValue = CHANNEL_Get(i);

			poststr(request, "<tr><td>");
			hprintf255(request, "<p>Select level:</p><form action=\"index\">");
			hprintf255(request, "<input type=\"hidden\" name=\"setIndex\" value=\"%i\">", i);
			for (j = 0; j < 3; j++) {
				const char* check;
				if (j == iValue)
					check = "checked";
				else
					check = "";
				hprintf255(request, "<input type=\"radio\" name=\"set\" value=\"%i\" onchange=\"this.form.submit()\" %s>%s", j, check, types[j]);
			}
			hprintf255(request, "</form>");
			poststr(request, "</td></tr>");

		}
		else {
			const char *channelTitle;

			channelTitle = ChannelType_GetTitle(channelType);

			if (*channelTitle) {
				int div;
				const char *channelUnit;
				char formatStr[16];
				strcpy(formatStr, " %.4f");

				div = ChannelType_GetDivider(channelType);
				channelUnit = ChannelType_GetUnit(channelType);

				iValue = CHANNEL_Get(i);
				fValue = (float)iValue / (float)div;

				poststr(request, "<tr><td>");
				poststr(request, channelTitle);
				// how many decimal places?
				formatStr[3] = '0'+ChannelType_GetDecimalPlaces(channelType);

				hprintf255(request, formatStr, fValue);
				poststr(request, channelUnit);
				hprintf255(request, " (%s)", CHANNEL_GetLabel(i));
				poststr(request, "</td></tr>");
			}
		}
	}

	if (bRawPWMs == 0 || bForceShowRGBCW || bForceShowRGB) {
		int c_pwms;
		int lm;
		int c_realPwms = 0;

		lm = LED_GetMode();

		//c_pwms = PIN_CountPinsWithRoleOrRole(IOR_PWM, IOR_PWM_n);
		// This will treat multiple PWMs on a single channel as one.
		// Thanks to this users can turn for example RGB LED controller
		// into high power 3-outputs single colors LED controller
		PIN_get_Relay_PWM_Count(0, &c_pwms, 0);
		c_realPwms = c_pwms;
		if (bForceShowRGBCW) {
			c_pwms = 5;
		}
		else if (bForceShowRGB) {
			c_pwms = 3;
		}

		if (c_pwms > 0) {
			const char* c;
			if (CHANNEL_Check(SPECIAL_CHANNEL_LEDPOWER)) {
				c = "bgrn";
			}
			else {
				c = "bred";
			}
			poststr(request, "<tr><td>");
			poststr(request, "<form action=\"index\">");
			hprintf255(request, "<input type=\"hidden\" name=\"tgl\" value=\"%i\">", SPECIAL_CHANNEL_LEDPOWER);
			hprintf255(request, "<input class=\"%s\" type=\"submit\" value=\"Toggle Light\"/></form>", c);
			poststr(request, "</td></tr>");
		}

		if (c_pwms > 0) {
			int pwmValue;

			inputName = "dim";

			pwmValue = LED_GetDimmer();

			poststr(request, "<tr><td>");
			hprintf255(request, "<h5>LED Dimmer/Brightness</h5>");
			hprintf255(request, "<form action=\"index\" id=\"form%i\">", SPECIAL_CHANNEL_BRIGHTNESS);
			hprintf255(request, "<input type=\"range\" min=\"0\" max=\"100\" name=\"%s\" id=\"slider%i\" value=\"%i\" onchange=\"this.form.submit()\">", inputName, SPECIAL_CHANNEL_BRIGHTNESS, pwmValue);
			hprintf255(request, "<input type=\"hidden\" name=\"%sIndex\" value=\"%i\">", inputName, SPECIAL_CHANNEL_BRIGHTNESS);
			hprintf255(request, "<input  type=\"submit\" class='disp-none' value=\"Toggle %i\"/></form>", SPECIAL_CHANNEL_BRIGHTNESS);
			poststr(request, "</td></tr>");
		}
		if (c_pwms >= 3) {
			char colorValue[16];
			inputName = "rgb";
			const char* activeStr = "";
			if (lm == Light_RGB) {
				activeStr = "[ACTIVE]";
			}

			LED_GetBaseColorString(colorValue);
			poststr(request, "<tr><td>");
			hprintf255(request, "<h5>LED RGB Color %s</h5>", activeStr);
			hprintf255(request, "<form action=\"index\" id=\"form%i\">", SPECIAL_CHANNEL_BASECOLOR);
			// onchange would fire only if colour was changed
			// onblur will fire every time
			hprintf255(request, "<input type=\"color\" name=\"%s\" id=\"color%i\" value=\"#%s\"  oninput=\"this.form.submit()\" >", inputName, SPECIAL_CHANNEL_BASECOLOR, colorValue);
			hprintf255(request, "<input type=\"hidden\" name=\"%sIndex\" value=\"%i\">", inputName, SPECIAL_CHANNEL_BASECOLOR);
			hprintf255(request, "<input  type=\"submit\" class='disp-none' value=\"Toggle Light\"/></form>");
			poststr(request, "</td></tr>");
		}
		bool bShowCWForPixelAnim = false;
#if ENABLE_DRIVER_PIXELANIM
		if (DRV_IsRunning("PixelAnim")) {
			if (c_realPwms == 2)
				bShowCWForPixelAnim = true;
			PixelAnim_CreatePanel(request);
		}
#endif
		if (c_pwms == 2 || c_pwms >= 4 || bShowCWForPixelAnim) {
			// TODO: temperature slider
			int pwmValue;
			const char* activeStr = "";
			if (lm == Light_Temperature) {
				activeStr = "[ACTIVE]";
			}

			inputName = "pwm";

			pwmValue = LED_GetTemperature();
			long pwmKelvin = HASS_TO_KELVIN(pwmValue);
			long pwmKelvinMax = HASS_TO_KELVIN(led_temperature_min);
			long pwmKelvinMin = HASS_TO_KELVIN(led_temperature_max);

			poststr(request, "<tr><td>");
			hprintf255(request, "<h5>LED Temperature Slider %s (%ld K) (Warm <--- ---> Cool)</h5>", activeStr, pwmKelvin);
			hprintf255(request, "<form class='r' action=\"index\" id=\"form%i\">", SPECIAL_CHANNEL_TEMPERATURE);

			//(KELVIN_TEMPERATURE_MAX - KELVIN_TEMPERATURE_MIN) / (HASS_TEMPERATURE_MAX - HASS_TEMPERATURE_MIN) = 13
			hprintf255(request, "<input type=\"range\" step='13' min=\"%ld\" max=\"%ld\" ", pwmKelvinMin, pwmKelvinMax);
			hprintf255(request, "value=\"%ld\" onchange=\"submitTemperature(this);\"/>", pwmKelvin);

			hprintf255(request, "<input type=\"hidden\" name=\"%sIndex\" value=\"%i\"/>", inputName, SPECIAL_CHANNEL_TEMPERATURE);
			hprintf255(request, "<input id=\"kelvin%i\" type=\"hidden\" name=\"%s\" />", SPECIAL_CHANNEL_TEMPERATURE, inputName);

			poststr(request, "</form></td></tr>");
		}

	}
#if defined(PLATFORM_BEKEN) || defined(WINDOWS)
	if (DRV_IsRunning("PWMToggler")) {
		DRV_Toggler_AddToHtmlPage(request);
	}
#endif
#if defined(PLATFORM_BEKEN) || defined(WINDOWS)
	if (DRV_IsRunning("httpButtons")) {
		DRV_HTTPButtons_AddToHtmlPage(request);
	}
#endif

	poststr(request, "</table>");
#ifndef OBK_DISABLE_ALL_DRIVERS
	DRV_AppendInformationToHTTPIndexPage(request);
#endif

	if (1) {
		int bFirst = true;
		hprintf255(request, "<h5>");
		for (i = 0; i < CHANNEL_MAX; i++) {
			if (CHANNEL_IsInUse(i)) {
				float value = CHANNEL_GetFloat(i);
				if (bFirst == false) {
					hprintf255(request, ", ");
				}
				hprintf255(request, "Channel %i = %.2f", i, value);
				bFirst = false;
			}
		}
		if (1) {
			i = RepeatingEvents_GetActiveCount();
			if (i) {
				if (bFirst == false) {
					hprintf255(request, ", ");
				}
				hprintf255(request, "%i repeating events", i);
				bFirst = false;
			}
			i = EventHandlers_GetActiveCount();
			if (i) {
				if (bFirst == false) {
					hprintf255(request, ", ");
				}
				hprintf255(request, "%i event handlers", i);
				bFirst = false;
			}
#if defined(WINDOWS) || defined(PLATFORM_BEKEN)
			i = CMD_GetCountActiveScriptThreads();
			if (i) {
				if (bFirst == false) {
					hprintf255(request, ", ");
				}
				hprintf255(request, "%i script threads", i);
				bFirst = false;
			}
#endif
		}
		hprintf255(request, "</h5>");
	}
	hprintf255(request, "<h5>Cfg size: %i, change counter: %i, ota counter: %i, incomplete boots: %i (might change to 0 if you wait to 30 sec)!</h5>",
		sizeof(g_cfg), g_cfg.changeCounter, g_cfg.otaCounter, g_bootFailures);

  // display temperature - thanks to giedriuslt
  // only in Normal mode, and if boot is not failing
	hprintf255(request, "<h5>Chip temperature: %.1f°C</h5>", g_wifi_temperature);

	inputName = CFG_GetPingHost();
	if (inputName && *inputName && CFG_GetPingDisconnectedSecondsToRestart()) {
		hprintf255(request, "<h5>Ping watchdog (%s) - ", inputName);
		if (g_startPingWatchDogAfter > 0) {
			hprintf255(request, "will start in %i!</h5>", g_startPingWatchDogAfter);
		}
		else {
			hprintf255(request, "%i lost, %i ok, last reply was %is ago!</h5>",
				PingWatchDog_GetTotalLost(), PingWatchDog_GetTotalReceived(), g_timeSinceLastPingReply);
		}
	}
	if (Main_HasWiFiConnected())
	{
		int rssi = HAL_GetWifiStrength();
		hprintf255(request, "<h5>Wifi RSSI: %s (%idBm)</h5>", str_rssi[wifi_rssi_scale(rssi)], rssi);
	}
#if PLATFORM_BEKEN
	/*
typedef enum {
	RESET_SOURCE_POWERON = 0,
	RESET_SOURCE_REBOOT = 1,
	RESET_SOURCE_WATCHDOG = 2,

	RESET_SOURCE_DEEPPS_GPIO = 3,
	RESET_SOURCE_DEEPPS_RTC = 4,

	RESET_SOURCE_CRASH_XAT0 = 5,
	RESET_SOURCE_CRASH_UNDEFINED = 6,
	RESET_SOURCE_CRASH_PREFETCH_ABORT = 7,
	RESET_SOURCE_CRASH_DATA_ABORT = 8,
	RESET_SOURCE_CRASH_UNUSED = 9,

} RESET_SOURCE_STATUS;
*/


	{
		const char* s = "Unk";
		if (g_rebootReason == 0)
			s = "Pwr";
		else if (g_rebootReason == 1)
			s = "Rbt";
		else if (g_rebootReason == 2)
			s = "Wdt";
		else if (g_rebootReason == 3)
			s = "Pin Interrupt";
		else if (g_rebootReason == 4)
			s = "Sleep Timer";
		hprintf255(request, "<h5>Reboot reason: %i - %s</h5>", g_rebootReason, s);
	}
#elif PLATFORM_BL602
	char reason[26];
	bl_sys_rstinfo_getsting(reason);
	hprintf255(request, "<h5>Reboot reason: %s</h5>", reason);
#elif PLATFORM_LN882H
	// type is chip_reboot_cause_t
	g_rebootReason = ln_chip_get_reboot_cause();
	{
		const char* s = "Unk";
		if (g_rebootReason == 0)
			s = "Pwr";
		else if (g_rebootReason == 1)
			s = "Soft";
		else if (g_rebootReason == 2)
			s = "Wdt";
		hprintf255(request, "<h5>Reboot reason: %i - %s</h5>", g_rebootReason, s);
	}
#elif PLATFORM_ESPIDF
	esp_reset_reason_t reason = esp_reset_reason();
	const char* s = "Unknown";
	switch(reason)
	{
	case ESP_RST_UNKNOWN:    s = "ESP_RST_UNKNOWN"; break;
	case ESP_RST_POWERON:    s = "ESP_RST_POWERON"; break;
	case ESP_RST_EXT:        s = "ESP_RST_EXT"; break;
	case ESP_RST_SW:         s = "ESP_RST_SW"; break;
	case ESP_RST_PANIC:      s = "ESP_RST_PANIC"; break;
	case ESP_RST_INT_WDT:    s = "ESP_RST_INT_WDT"; break;
	case ESP_RST_TASK_WDT:   s = "ESP_RST_TASK_WDT"; break;
	case ESP_RST_WDT:        s = "ESP_RST_WDT"; break;
	case ESP_RST_DEEPSLEEP:  s = "ESP_RST_DEEPSLEEP"; break;
	case ESP_RST_BROWNOUT:   s = "ESP_RST_BROWNOUT"; break;
	case ESP_RST_SDIO:       s = "ESP_RST_SDIO"; break;
	case ESP_RST_USB:        s = "ESP_RST_USB"; break;
	case ESP_RST_JTAG:       s = "ESP_RST_JTAG"; break;
	case ESP_RST_EFUSE:      s = "ESP_RST_EFUSE"; break;
	case ESP_RST_PWR_GLITCH: s = "ESP_RST_PWR_GLITCH"; break;
	case ESP_RST_CPU_LOCKUP: s = "ESP_RST_CPU_LOCKUP"; break;
	default: break;
	}
	hprintf255(request, "<h5>Reboot reason: %i - %s</h5>", reason, s);
#endif
	if (CFG_GetMQTTHost()[0] == 0) {
		hprintf255(request, "<h5>MQTT State: not configured<br>");
	}
	else {
		const char* stateStr;
		const char* colorStr;
		if (mqtt_reconnect > 0) {
			stateStr = "awaiting reconnect";
			colorStr = "orange";
		}
		else if (Main_HasMQTTConnected() == 1) {
			stateStr = "connected";
			colorStr = "green";
		}
		else {
			stateStr = "disconnected";
			colorStr = "yellow";
		}
		hprintf255(request, "<h5>MQTT State: <span style=\"color:%s\">%s</span> RES: %d(%s)<br>", colorStr,
			stateStr, MQTT_GetConnectResult(), get_error_name(MQTT_GetConnectResult()));
		hprintf255(request, "MQTT ErrMsg: %s <br>", (MQTT_GetStatusMessage() != NULL) ? MQTT_GetStatusMessage() : "");
		hprintf255(request, "MQTT Stats:CONN: %d PUB: %d RECV: %d ERR: %d </h5>", MQTT_GetConnectEvents(),
			MQTT_GetPublishEventCounter(), MQTT_GetReceivedEventCounter(), MQTT_GetPublishErrorCounter());
	}
	/* Format current PINS input state for all unused pins */
	if (CFG_HasFlag(OBK_FLAG_HTTP_PINMONITOR))
	{
		for (i = 0; i < 29; i++)
		{
			if ((PIN_GetPinRoleForPinIndex(i) == IOR_None) && (i != 0) && (i != 1))
			{
				HAL_PIN_Setup_Input(i);
			}
		}

		hprintf255(request, "<h5> PIN States<br>");
		for (i = 0; i < 29; i++)
		{
			if ((PIN_GetPinRoleForPinIndex(i) != IOR_None) || (i == 0) || (i == 1))
			{
				hprintf255(request, "P%02i: NA ", i);
			}
			else
			{
				hprintf255(request, "P%02i: %i  ", i, (int)HAL_PIN_ReadDigitalInput(i));
			}
			if (i % 10 == 9)
			{
				hprintf255(request, "<br>");
			}
		}
		hprintf255(request, "</h5>");
	}

#if ENABLE_DRIVER_CHARTS		
/*	// moved from drv_charts.c:
	// on every "state" request, JS code will be loaded and canvas is redrawn
	// this leads to a flickering graph
	// so put this right below the "state" div
	// with a "#ifdef 
	// drawback : We need to take care, if driver is loaded and canvas will be displayed only on a reload of the page
	// or we might try and hide/unhide it ...
*/
	// since we can't simply stop showing the graph in updated status, we need to "hide" it if driver was stopped
	if (! DRV_IsRunning("Charts")) {
		poststr(request, "<style onload=\"document.getElementById('obkChart').style.display='none'\"></style>");		
	};

#endif

#if WINDOWS
#elif PLATFORM_BL602
#elif PLATFORM_W600 || PLATFORM_W800
#elif PLATFORM_XR809
#elif PLATFORM_BK7231N || PLATFORM_BK7231T
	if (ota_progress() >= 0)
	{
		hprintf255(request, "<h5>OTA In Progress. Downloaded: %i B Flashed: %06lXh</h5>", OTA_GetTotalBytes(), ota_progress());
	}
#endif
	if (bSafeMode) {
		hprintf255(request, "<h5 class='safe'>You are in safe mode (AP mode) because full reboot failed %i times. ",
			g_bootFailures);
		hprintf255(request, "Pins, relays, etc are disabled.</h5>");

	}
	// for normal page loads, show the rest of the HTML
	if (!http_getArg(request->url, "state", tmpA, sizeof(tmpA))) {
		poststr(request, "</div>"); // end div#state
#if ENABLE_DRIVER_CHARTS		
/*	// moved from drv_charts.c:
	// on every "state" request, JS code will be loaded and canvas is redrawn
	// this leads to a flickering graph
	// so put this right below the "state" div
	// with a "#ifdef 
	// drawback : We need to take care, if driver is loaded and canvas will be displayed only on a reload of the page
	// or we might try and hide/unhide it ...
*/
//	if (DRV_IsRunning("Charts")) {
		poststr(request, "<canvas style='display: none' id=\"obkChart\" width=\"400\" height=\"200\"></canvas>");
		poststr(request, "<script src=\"https://cdn.jsdelivr.net/npm/chart.js\"></script>");
//	};

#endif


		// Shared UI elements 
		poststr(request, "<form action=\"cfg\"><input type=\"submit\" value=\"Config\"/></form>");

		poststr(request, "<form action=\"/index\">"
			"<input type=\"hidden\" id=\"restart\" name=\"restart\" value=\"1\">"
			"<input class=\"bred\" type=\"submit\" value=\"Restart\" onclick=\"return confirm('Are you sure to restart module?')\">"
			"</form>");

		if (bSafeMode) {
			poststr(request, "<form action=\"/index\">"
				"<input type=\"hidden\" id=\"unsafe\" name=\"unsafe\" value=\"1\">"
				"<input class=\"bred\" type=\"submit\" value=\"Exit safe mode\" onclick=\"");
			poststr(request, "return confirm('Are you sure to try exiting safe mode? NOTE: This will enable rest interface etc, but still wont run autoexec')\">"
				"</form>");
		}
		poststr(request, "<form action=\"/app\" target=\"_blank\"><input type=\"submit\" value=\"Launch Web Application\"></form> ");
		poststr(request, "<form action=\"about\"><input type=\"submit\" value=\"About\"/></form>");

		poststr(request, htmlFooterRefreshLink);
		http_html_end(request);
	}

	poststr(request, NULL);
	return 0;
}

int http_fn_about(http_request_t* request) {
	http_setup(request, httpMimeTypeHTML);
	http_html_start(request, "About");
	poststr_h2(request, "Open source firmware for BK7231N, BK7231T, T34, BL2028N, XR809, W600/W601, W800/W801, BL602, LF686 and LN882H by OpenSHWProjects");
	poststr(request, htmlFooterReturnToMainPage);
	http_html_end(request);
	poststr(request, NULL);
	return 0;
}

int http_fn_cfg_mqtt(http_request_t* request) {
	http_setup(request, httpMimeTypeHTML);
	http_html_start(request, "MQTT");
	poststr_h2(request, "Use this to connect to your MQTT");
	poststr_h4(request, "To disable MQTT, clear the host field.");
	hprintf255(request, "<h4>Command topic: cmnd/%s/[Command]</h4>", CFG_GetMQTTClientId());
	hprintf255(request, "<h4>Publish data topic: %s/[Channel]/get</h4>", CFG_GetMQTTClientId());
	hprintf255(request, "<h4>Receive data topic:  %s/[Channel]/set</h4>", CFG_GetMQTTClientId());

	add_label_text_field(request, "Host", "host", CFG_GetMQTTHost(), "<form action=\"/cfg_mqtt_set\">");
	add_label_numeric_field(request, "Port", "port", CFG_GetMQTTPort(), "<br>");
	add_label_text_field(request, "Client Topic (Base Topic)", "client", CFG_GetMQTTClientId(), "<br><br>");
	add_label_text_field(request, "Group Topic (Secondary Topic to only receive cmnds)", "group", CFG_GetMQTTGroupTopic(), "<br>");
	add_label_text_field(request, "User", "user", CFG_GetMQTTUserName(), "<br>");
	add_label_password_field(request, "Password", "password", CFG_GetMQTTPass(), "<br>");

	poststr(request, "<br><input type=\"submit\" value=\"Submit\" onclick=\"return confirm('Are you sure? Please check MQTT data twice?')\"></form> ");
	poststr(request, htmlFooterReturnToCfgOrMainPage);
	http_html_end(request);
	poststr(request, NULL);
	return 0;
}

int http_fn_cfg_ip(http_request_t* request) {
	char tmp[64];
	int g_changes = 0;
	http_setup(request, httpMimeTypeHTML);
	http_html_start(request, "IP");
	poststr_h2(request, "Here you can set static IP or DHCP");
	hprintf255(request, "<h4>This setting applies only to WiFi client mode.</h4>");
	hprintf255(request, "<h4>You must restart manually for changes to take place.</h4>");
	hprintf255(request, "<h4>Currently, DHCP is enabled by default and works when you set IP to 0.0.0.0.</h4>");

	if (http_getArg(request->url, "IP", tmp, sizeof(tmp))) {
		str_to_ip(tmp, g_cfg.staticIP.localIPAddr);
//hprintf255(request, "<br>IP=%s (%02x %02x %02x %02x)<br>",tmp,g_cfg.staticIP.localIPAddr[0],g_cfg.staticIP.localIPAddr[1],g_cfg.staticIP.localIPAddr[2],g_cfg.staticIP.localIPAddr[3]);
		g_changes++;
	}
	if (http_getArg(request->url, "mask", tmp, sizeof(tmp))) {
		str_to_ip(tmp, g_cfg.staticIP.netMask);
//hprintf255(request, "<br>Mask=%s (%02x %02x %02x %02x)<br>",tmp, g_cfg.staticIP.netMask[0], g_cfg.staticIP.netMask[1], g_cfg.staticIP.netMask[2], g_cfg.staticIP.netMask[3]);
		g_changes++;
	}
	if (http_getArg(request->url, "dns", tmp, sizeof(tmp))) {
		str_to_ip(tmp, g_cfg.staticIP.dnsServerIpAddr);
//hprintf255(request, "<br>DNS=%s (%02x %02x %02x %02x)<br>",tmp, g_cfg.staticIP.dnsServerIpAddr[0], g_cfg.staticIP.dnsServerIpAddr[1], g_cfg.staticIP.dnsServerIpAddr[2], g_cfg.staticIP.dnsServerIpAddr[3]);
		g_changes++;
	}
	if (http_getArg(request->url, "gate", tmp, sizeof(tmp))) {
		str_to_ip(tmp, g_cfg.staticIP.gatewayIPAddr);
//hprintf255(request, "<br>GW=%s (%02x %02x %02x %02x)<br>",tmp, g_cfg.staticIP.gatewayIPAddr[0], g_cfg.staticIP.gatewayIPAddr[1], g_cfg.staticIP.gatewayIPAddr[2], g_cfg.staticIP.gatewayIPAddr[3]);
		g_changes++;
	}
	if (g_changes) {
		CFG_MarkAsDirty();
		hprintf255(request, "<h4>Saved.</h4>");
	}
	convert_IP_to_string(tmp, g_cfg.staticIP.localIPAddr);
	add_label_text_field(request, "IP", "IP", tmp, "<form action=\"/cfg_ip\">");
	convert_IP_to_string(tmp, g_cfg.staticIP.netMask);
	add_label_text_field(request, "Mask", "mask", tmp, "<br><br>");
	convert_IP_to_string(tmp, g_cfg.staticIP.dnsServerIpAddr);
	add_label_text_field(request, "DNS", "dns", tmp, "<br>");
	convert_IP_to_string(tmp, g_cfg.staticIP.gatewayIPAddr);
	add_label_text_field(request, "Gate", "gate", tmp, "<br>");

	poststr(request, "<br><input type=\"submit\" value=\"Submit\" onclick=\"return confirm('Are you sure? Remember that you need to reboot manually to apply changes')\"></form> ");
	poststr(request, htmlFooterReturnToCfgOrMainPage);
	http_html_end(request);
	poststr(request, NULL);
	return 0;
}

int http_fn_cfg_mqtt_set(http_request_t* request) {
	char tmpA[128];
	http_setup(request, httpMimeTypeHTML);
	http_html_start(request, "Saving MQTT");

	if (http_getArg(request->url, "host", tmpA, sizeof(tmpA))) {
	}
	// FIX: always set, so people can clear field
	CFG_SetMQTTHost(tmpA);
	if (http_getArg(request->url, "port", tmpA, sizeof(tmpA))) {
		CFG_SetMQTTPort(atoi(tmpA));
	}
	if (http_getArg(request->url, "user", tmpA, sizeof(tmpA))) {
		CFG_SetMQTTUserName(tmpA);
	}
	if (http_getArg(request->url, "password", tmpA, sizeof(tmpA))) {
		CFG_SetMQTTPass(tmpA);
	}
	if (http_getArg(request->url, "client", tmpA, sizeof(tmpA))) {
		CFG_SetMQTTClientId(tmpA);
	}
	if (http_getArg(request->url, "group", tmpA, sizeof(tmpA))) {
		CFG_SetMQTTGroupTopic(tmpA);
	}

	CFG_Save_SetupTimer();

	poststr(request, "Please wait for module to connect... if there is problem, restart it from Index html page...");

	g_mqtt_bBaseTopicDirty = 1;

	poststr(request, "<br><a href=\"cfg_mqtt\">Return to MQTT settings</a><br>");
	poststr(request, htmlFooterReturnToCfgOrMainPage);
	http_html_end(request);
	poststr(request, NULL);
	return 0;
}

int http_fn_cfg_webapp(http_request_t* request) {
	http_setup(request, httpMimeTypeHTML);
	http_html_start(request, "Set Webapp");
	add_label_text_field(request, "URL of the Webapp", "url", CFG_GetWebappRoot(), "<form action=\"/cfg_webapp_set\">");
	poststr(request, SUBMIT_AND_END_FORM);
	poststr(request, htmlFooterReturnToCfgOrMainPage);
	http_html_end(request);
	poststr(request, NULL);
	return 0;
}

int http_fn_cfg_webapp_set(http_request_t* request) {
	char tmpA[128];
	http_setup(request, httpMimeTypeHTML);
	http_html_start(request, "Saving Webapp");

	if (http_getArg(request->url, "url", tmpA, sizeof(tmpA))) {
		CFG_SetWebappRoot(tmpA);
		CFG_Save_IfThereArePendingChanges();
		hprintf255(request, "Webapp url set to %s", tmpA);
	}
	else {
		poststr(request, "Webapp url not set because you didn't specify the argument.");
	}

	poststr(request, "<br>");
	poststr(request, htmlFooterReturnToCfgOrMainPage);
	http_html_end(request);
	poststr(request, NULL);
	return 0;
}





int http_fn_cfg_ping(http_request_t* request) {
	char tmpA[128];
	int bChanged;

	http_setup(request, httpMimeTypeHTML);
	http_html_start(request, "Set Watchdog");
	bChanged = 0;
	poststr(request, "<h3>Ping watchdog (backup reconnect mechanism)</h3>");
	poststr(request, "<p> By default, all OpenBeken devices automatically try to reconnect to WiFi when a connection is lost.");
	poststr(request, " I have tested the reconnect mechanism many times by restarting my router and it always worked reliably.");
	poststr(request, " However, according to some reports, there are still some edge cases where a device fails to reconnect to WiFi.");
	poststr(request, " This is why <b>this mechanism</b> has been added.</p>");
	poststr(request, "<p>This mechanism continuously pings a specified host and reconnects to WiFi if it doesn't respond for the specified number of seconds.</p>");
	poststr(request, "<p>USAGE: For the host, choose the main address of your router and ensure it responds to pings. The interval is around 1 second, and the timeout can be set by the user, for example, to 60 seconds.</p>");
	if (http_getArg(request->url, "host", tmpA, sizeof(tmpA))) {
		CFG_SetPingHost(tmpA);
		poststr_h4(request, "New ping host set!");
		bChanged = 1;
	}
	/* if(http_getArg(request->url,"interval",tmpA,sizeof(tmpA))) {
		 CFG_SetPingIntervalSeconds(atoi(tmpA));
		 poststr(request,"<h4> New ping interval set!</h4>");
		 bChanged = 1;
	 }*/
	if (http_getArg(request->url, "disconnectTime", tmpA, sizeof(tmpA))) {
		CFG_SetPingDisconnectedSecondsToRestart(atoi(tmpA));
		poststr_h4(request, "New ping disconnectTime set!");
		bChanged = 1;
	}
	if (http_getArg(request->url, "clear", tmpA, sizeof(tmpA))) {
		CFG_SetPingDisconnectedSecondsToRestart(0);
		CFG_SetPingIntervalSeconds(0);
		CFG_SetPingHost("");
		poststr_h4(request, "Ping watchdog disabled!");
		bChanged = 1;
	}
	if (bChanged) {
		CFG_Save_IfThereArePendingChanges();
		poststr_h4(request, "Changes will be applied after restarting");
	}
	poststr(request, "<form action=\"/cfg_ping\">\
<input type=\"hidden\" id=\"clear\" name=\"clear\" value=\"1\">\
<input type=\"submit\" value=\"Disable ping watchdog!\">\
</form>");
	poststr_h2(request, "Use this to enable pinger");
	add_label_text_field(request, "Host", "host", CFG_GetPingHost(), "<form action=\"/cfg_ping\">");
	add_label_numeric_field(request, "Take action after this number of seconds with no reply", "disconnectTime",
		CFG_GetPingDisconnectedSecondsToRestart(), "<br>");
	poststr(request, "<br><br>\
<input type=\"submit\" value=\"Submit\" onclick=\"return confirm('Are you sure?')\">\
</form>");
	poststr(request, htmlFooterReturnToCfgOrMainPage);
	http_html_end(request);
	poststr(request, NULL);
	return 0;
}
int http_fn_cfg_wifi(http_request_t* request) {
	// for a test, show password as well...
	char tmpA[128];

	http_setup(request, httpMimeTypeHTML);
	http_html_start(request, "Set Wifi");
	/*bChanged = 0;
	if(http_getArg(recvbuf,"ssid",tmpA,sizeof(tmpA))) {
		CFG_SetWiFiSSID(tmpA);
		poststr(request,"<h4> WiFi SSID set!</h4>");
		bChanged = 1;
	}
	if(http_getArg(recvbuf,"pass",tmpA,sizeof(tmpA))) {
		CFG_SetWiFiPass(tmpA);
		poststr(request,"<h4> WiFi Password set!</h4>");
		bChanged = 1;
	}
	if(bChanged) {
		poststr(request,"<h4> Device will reconnect after restarting</h4>");
	}*/
	poststr(request, "<h2> Check networks reachable by module</h2> This will take a few seconds<br>");
	if (http_getArg(request->url, "scan", tmpA, sizeof(tmpA))) {
#ifdef WINDOWS

		poststr(request, "Not available on Windows<br>");
#elif PLATFORM_XR809
		poststr(request, "TODO XR809<br>");

#elif PLATFORM_W600 || PLATFORM_W800
		poststr(request, "TODO W800<br>");
#elif PLATFORM_BL602
               wifi_mgmr_ap_item_t *ap_info;
               uint32_t i, ap_num;

               bk_printf("Scan begin...\r\n");
               wifi_mgmr_all_ap_scan(&ap_info, &ap_num);
               bk_printf("Scan returned %li networks\r\n", ap_num);

               for (i = 0; i < ap_num; i++) {
                       hprintf255(request, "[%i/%i] SSID: %s, Channel: %i, Signal %i<br>", (i+1), (int)ap_num, ap_info[i].ssid, ap_info[i].channel, ap_info[i].rssi);
               }
               vPortFree(ap_info);
#elif PLATFORM_BK7231T
		int i;

		AP_IF_S* ar;
		uint32_t num;

		bk_printf("Scan begin...\r\n");
		tuya_hal_wifi_all_ap_scan(&ar, &num);
		bk_printf("Scan returned %i networks\r\n", num);
		for (i = 0; i < num; i++) {
			hprintf255(request, "[%i/%i] SSID: %s, Channel: %i, Signal %i<br>", i, (int)num, ar[i].ssid, ar[i].channel, ar[i].rssi);
		}
		tuya_hal_wifi_release_ap(ar);
#elif PLATFORM_BK7231N
		int i;

		AP_IF_S* ar;
		uint32_t num;

		bk_printf("Scan begin...\r\n");
		tuya_os_adapt_wifi_all_ap_scan(&ar, (unsigned int*)&num);
		bk_printf("Scan returned %i networks\r\n", num);
		for (i = 0; i < num; i++) {
			hprintf255(request, "[%i/%i] SSID: %s, Channel: %i, Signal %i<br>", i + 1, (int)num, ar[i].ssid, ar[i].channel, ar[i].rssi);
		}
		tuya_os_adapt_wifi_release_ap(ar);
#elif PLATFORM_LN882H
// TODO:LN882H action
        poststr(request, "TODO LN882H<br>");
#elif PLATFORM_ESPIDF
		// doesn't work in ap mode, only sta/apsta
		uint16_t ap_count = 0, number = 30;
		wifi_ap_record_t ap_info[number];
		memset(ap_info, 0, sizeof(ap_info));
		bk_printf("Scan begin...\r\n");
		esp_wifi_scan_start(NULL, true);
		esp_wifi_scan_get_ap_num(&ap_count);
		bk_printf("Scan returned %i networks, max allowed: %i\r\n", ap_count, number);
		esp_wifi_scan_get_ap_records(&number, ap_info);
		for(int i = 0; i < number; i++)
		{
			hprintf255(request, "[%i/%u] SSID: %s, Channel: %i, Signal %i<br>", i + 1, number, ap_info[i].ssid, ap_info[i].primary, ap_info[i].rssi);
		}
#else
#error "Unknown platform"
		poststr(request, "Unknown platform<br>");
#endif
	}
	poststr(request, "<form action=\"/cfg_wifi\">\
<input type=\"hidden\" id=\"scan\" name=\"scan\" value=\"1\">\
<input type=\"submit\" value=\"Scan Local Networks\">\
</form>");
	poststr_h4(request, "Use this to disconnect from your WiFi");
	poststr(request, "<form action=\"/cfg_wifi_set\">\
<input type=\"hidden\" id=\"open\" name=\"open\" value=\"1\">\
<input type=\"submit\" value=\"Convert to Open Access WiFi\" onclick=\"return confirm('Are you sure you want to switch to open access WiFi?')\">\
</form>");
	poststr_h2(request, "Use this to connect to your WiFi");
	add_label_text_field(request, "SSID", "ssid", CFG_GetWiFiSSID(), "<form action=\"/cfg_wifi_set\">");
	add_label_password_field(request, "", "pass", CFG_GetWiFiPass(), "<br>Password<span  style=\"float:right;\"><input type=\"checkbox\" onclick=\"e=getElement('pass');if(this.checked){e.value='';e.type='text'}else e.type='password'\" > enable clear text password (clears existing)</span>");
	poststr_h2(request, "Alternate WiFi (used when first one is not responding)");
#ifndef PLATFORM_BEKEN
	poststr_h2(request, "SSID2 only on Beken Platform (BK7231T, BK7231N)");
#endif
	add_label_text_field(request, "SSID2", "ssid2", CFG_GetWiFiSSID2(), "");
	add_label_password_field(request, "", "pass2", CFG_GetWiFiPass2(), "<br>Password2<span  style=\"float:right;\"><input type=\"checkbox\" onclick=\"e=getElement('pass2');if(this.checked){e.value='';e.type='text'}else e.type='password'\" > enable clear text password (clears existing)</span>");
#if ALLOW_WEB_PASSWORD
	int web_password_enabled = strcmp(CFG_GetWebPassword(), "") == 0 ? 0 : 1;
	poststr_h2(request, "Web Authentication");
	poststr(request, "<p>Enabling web authentication will protect this web interface and API using basic HTTP authentication. Username is always <b>admin</b>.</p>");
	hprintf255(request, "<div><input type=\"checkbox\" name=\"web_admin_password_enabled\" id=\"web_admin_password_enabled\" value=\"1\"%s>", (web_password_enabled > 0 ? " checked" : ""));
	poststr(request, "<label for=\"web_admin_password_enabled\">Enable web authentication</label></div>");
	add_label_password_field(request, "Admin Password", "web_admin_password", CFG_GetWebPassword(), "");
#endif
	poststr(request, "<br><br>\
<input type=\"submit\" value=\"Submit\" onclick=\"return confirm('Are you sure? Please double-check SSID and password.')\">\
</form>");
	poststr(request, htmlFooterReturnToCfgOrMainPage);
	http_html_end(request);
	poststr(request, NULL);
	return 0;
}
int http_fn_cfg_name(http_request_t* request) {
	// for a test, show password as well...
	char tmpA[128];

	http_setup(request, httpMimeTypeHTML);
	http_html_start(request, "Set name");

	poststr_h2(request, "Change device names for display");
	if (http_getArg(request->url, "shortName", tmpA, sizeof(tmpA))) {
		if (STR_ReplaceWhiteSpacesWithUnderscore(tmpA)) {
			poststr_h2(request, "You cannot have whitespaces in short name!");
		}
		CFG_SetShortDeviceName(tmpA);
	}
	if (http_getArg(request->url, "name", tmpA, sizeof(tmpA))) {
		CFG_SetDeviceName(tmpA);
	}
	CFG_Save_IfThereArePendingChanges();

	poststr_h2(request, "Use this to change device names");
	add_label_name_field(request, "ShortName", "shortName", CFG_GetShortDeviceName(), "<form action=\"/cfg_name\">");
	add_label_name_field(request, "Full Name", "name", CFG_GetDeviceName(), "<br>");

	poststr(request, "<br><br>");
	poststr(request, "<input type=\"submit\" value=\"Submit\" "
		"onclick=\"return confirm('Are you sure? "
		"Short name might be used by Home Assistant, "
		"so you will have to reconfig some stuff.')\">");
	poststr(request, "</form>");
	//poststr(request,htmlReturnToCfg);
	//HTTP_AddBuildFooter(request);
	//poststr(request,htmlEnd);
	poststr(request, htmlFooterReturnToCfgOrMainPage);
	http_html_end(request);
	poststr(request, NULL);
	return 0;
}

int http_fn_cfg_wifi_set(http_request_t* request) {
	char tmpA[128];
	int bChanged;

	addLogAdv(LOG_INFO, LOG_FEATURE_HTTP, "HTTP_ProcessPacket: generating cfg_wifi_set \r\n");
	bChanged = 0;

	http_setup(request, httpMimeTypeHTML);
	http_html_start(request, "Saving Wifi");
	if (http_getArg(request->url, "open", tmpA, sizeof(tmpA))) {
		bChanged |= CFG_SetWiFiSSID("");
		bChanged |= CFG_SetWiFiPass("");
		poststr(request, "WiFi mode set: open access point.");
	}
	else {
		if (http_getArg(request->url, "ssid", tmpA, sizeof(tmpA))) {
			bChanged |= CFG_SetWiFiSSID(tmpA);
		}
		if (http_getArg(request->url, "pass", tmpA, sizeof(tmpA))) {
			bChanged |= CFG_SetWiFiPass(tmpA);
		}
		poststr(request, "WiFi mode set: connect to WLAN.");
	}
	if (http_getArg(request->url, "ssid2", tmpA, sizeof(tmpA))) {
		bChanged |= CFG_SetWiFiSSID2(tmpA);
	}
	if (http_getArg(request->url, "pass2", tmpA, sizeof(tmpA))) {
		bChanged |= CFG_SetWiFiPass2(tmpA);
	}
#if ALLOW_WEB_PASSWORD
	if (http_getArg(request->url, "web_admin_password_enabled", tmpA, sizeof(tmpA))) {
		int web_password_enabled = atoi(tmpA);
		if (web_password_enabled > 0 && http_getArg(request->url, "web_admin_password", tmpA, sizeof(tmpA))) {
			if (strlen(tmpA) < 5) {
				poststr_h4(request, "Web password needs to be at least 5 characters long!");
			} else {
				poststr(request, "<p>Web password has been changed.</p>");
				CFG_SetWebPassword(tmpA);
			}
		}
	} else {
		CFG_SetWebPassword("");
	}
#endif
	CFG_Save_SetupTimer();
	if (bChanged == 0) {
		poststr(request, "<p>WiFi: No changes detected.</p>");
	}
	else {
		poststr(request, "<p>WiFi: Please wait for module to reset...</p>");
		RESET_ScheduleModuleReset(3);
	}
	poststr(request, "<br><a href=\"cfg_wifi\">Return to WiFi settings</a><br>");
	poststr(request, htmlFooterReturnToCfgOrMainPage);
	http_html_end(request);
	poststr(request, NULL);
	return 0;
}

int http_fn_cfg_loglevel_set(http_request_t* request) {
	char tmpA[128];
	addLogAdv(LOG_INFO, LOG_FEATURE_HTTP, "HTTP_ProcessPacket: generating cfg_loglevel_set \r\n");

	http_setup(request, httpMimeTypeHTML);
	http_html_start(request, "Set log level");
	if (http_getArg(request->url, "loglevel", tmpA, sizeof(tmpA))) {
#if WINDOWS
#else
		g_loglevel = atoi(tmpA);
#endif
		poststr(request, "LOG level changed.");
	}

	tmpA[0] = 0;
#if WINDOWS
	add_label_text_field(request, "Loglevel", "loglevel", "", "<form action=\"/cfg_loglevel_set\">");
#else
	add_label_numeric_field(request, "Loglevel", "loglevel", g_loglevel, "<form action=\"/cfg_loglevel_set\">");
#endif
	poststr(request, "<br><br>\
<input type=\"submit\" value=\"Submit\" >\
</form>");

	poststr(request, "<br><a href=\"cfg\">Return to config settings</a><br>");
	poststr(request, htmlFooterReturnToCfgOrMainPage);
	http_html_end(request);
	poststr(request, NULL);
	return 0;
}


int http_fn_cfg_mac(http_request_t* request) {
	// must be unsigned, else print below prints negatives as e.g. FFFFFFFe
	unsigned char mac[6];
	char tmpA[128];
	int i;
	char macStr[16];

	http_setup(request, httpMimeTypeHTML);
	http_html_start(request, "Set MAC address");

	if (http_getArg(request->url, "mac", tmpA, sizeof(tmpA))) {
		for (i = 0; i < 6; i++)
		{
			mac[i] = hexbyte(&tmpA[i * 2]);
		}
		//sscanf(tmpA,"%02X%02X%02X%02X%02X%02X",&mac[0],&mac[1],&mac[2],&mac[3],&mac[4],&mac[5]);
		if (WiFI_SetMacAddress((char*)mac)) {
			poststr_h4(request, "New MAC set!");
		}
		else {
			poststr_h4(request, "MAC change error?");
		}
		CFG_Save_IfThereArePendingChanges();
	}

	WiFI_GetMacAddress((char*)mac);

	poststr_h2(request, "Here you can change MAC address.");

	sprintf(macStr, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	add_label_text_field(request, "MAC", "mac", macStr, "<form action=\"/cfg_mac\">");
	poststr(request, "<br><br>\
<input type=\"submit\" value=\"Submit\" onclick=\"return confirm('Are you sure? Please check MAC hex string twice?')\">\
</form>");
	poststr(request, htmlFooterReturnToCfgOrMainPage);
	http_html_end(request);
	poststr(request, NULL);
	return 0;
}
//
//int http_fn_flash_read_tool(http_request_t* request) {
//	int len = 16;
//	int ofs = 1970176;
//	int res;
//	int rem;
//	int now;
//	int nowOfs;
//	int hex;
//	int i;
//	char tmpA[128];
//	char tmpB[64];
//
//	http_setup(request, httpMimeTypeHTML);
//	http_html_start(request, "Flash read");
//	poststr_h4(request, "Flash Read Tool");
//	if (http_getArg(request->url, "hex", tmpA, sizeof(tmpA))) {
//		hex = atoi(tmpA);
//	}
//	else {
//		hex = 0;
//	}
//
//	if (http_getArg(request->url, "offset", tmpA, sizeof(tmpA)) &&
//		http_getArg(request->url, "len", tmpB, sizeof(tmpB))) {
//		unsigned char buffer[128];
//		len = atoi(tmpB);
//		ofs = atoi(tmpA);
//		hprintf255(request, "Memory at %i with len %i reads: ", ofs, len);
//		poststr(request, "<br>");
//
//		///res = bekken_hal_flash_read (ofs, buffer,len);
//		//sprintf(tmpA,"Result %i",res);
//	//	strcat(outbuf,tmpA);
//	///	strcat(outbuf,"<br>");
//
//		nowOfs = ofs;
//		rem = len;
//		while (1) {
//			if (rem > sizeof(buffer)) {
//				now = sizeof(buffer);
//			}
//			else {
//				now = rem;
//			}
//#if PLATFORM_XR809
//			//uint32_t flash_read(uint32_t flash, uint32_t addr,void *buf, uint32_t size)
//#define FLASH_INDEX_XR809 0
//			res = flash_read(FLASH_INDEX_XR809, nowOfs, buffer, now);
//#elif PLATFORM_BL602
//
//#elif PLATFORM_W600 || PLATFORM_W800
//
//#else
//			res = bekken_hal_flash_read(nowOfs, buffer, now);
//#endif
//			for (i = 0; i < now; i++) {
//				unsigned char val = buffer[i];
//				if (!hex && isprint(val)) {
//					hprintf255(request, "'%c' ", val);
//				}
//				else {
//					hprintf255(request, "%02X ", val);
//				}
//			}
//			rem -= now;
//			nowOfs += now;
//			if (rem <= 0) {
//				break;
//			}
//		}
//
//		poststr(request, "<br>");
//	}
//	poststr(request, "<form action=\"/flash_read_tool\">");
//
//	poststr(request, "<input type=\"checkbox\" id=\"hex\" name=\"hex\" value=\"1\"");
//	if (hex) {
//		poststr(request, " checked");
//	}
//	poststr(request, "><label for=\"hex\">Show all hex?</label><br>");
//
//	add_label_numeric_field(request, "Offset", "offset", ofs, "");
//	add_label_numeric_field(request, "Length", "len", len, "<br>");
//	poststr(request, SUBMIT_AND_END_FORM);
//
//	poststr(request, htmlFooterReturnToCfgOrMainPage);
//	http_html_end(request);
//	poststr(request, NULL);
//	return 0;
//}
const char* CMD_GetResultString(commandResult_t r) {
	if (r == CMD_RES_OK)
		return "OK";
	if (r == CMD_RES_EMPTY_STRING)
		return "No command entered";
	if (r == CMD_RES_ERROR)
		return "Command found but returned error";
	if (r == CMD_RES_NOT_ENOUGH_ARGUMENTS)
		return "Not enough arguments for this command";
	if (r == CMD_RES_UNKNOWN_COMMAND)
		return "Unknown command";
	if (r == CMD_RES_BAD_ARGUMENT)
		return "Bad argument";
	return "Unknown error";
}
// all log printfs made by command will be sent also to request
void LOG_SetCommandHTTPRedirectReply(http_request_t* request);

int http_fn_cmd_tool(http_request_t* request) {
	commandResult_t res;
	const char* resStr;
	char tmpA[128];
	char* long_str_alloced = 0;
	int commandLen;

	http_setup(request, httpMimeTypeHTML);
	http_html_start(request, "Command tool");
	poststr_h4(request, "Command Tool");
	poststr(request, "This is a basic command line. <br>");
	poststr(request, "Please consider using the 'Web Application' console for more options and real-time log viewing. <br>");
	poststr(request, "Remember that some commands are added after a restart when a driver is activated. <br>");

	commandLen = http_getArg(request->url, "cmd", tmpA, sizeof(tmpA));
	addLogAdv(LOG_ERROR, LOG_FEATURE_HTTP, "http_fn_cmd_tool: len %i",commandLen);
	if (commandLen) {
		poststr(request, "<br>");
		// all log printfs made by command will be sent also to request
		LOG_SetCommandHTTPRedirectReply(request);
		if (commandLen > (sizeof(tmpA) - 5)) {
			commandLen += 8;
			long_str_alloced = (char*)malloc(commandLen);
			if (long_str_alloced) {
				http_getArg(request->url, "cmd", long_str_alloced, commandLen);
				res = CMD_ExecuteCommand(long_str_alloced, COMMAND_FLAG_SOURCE_CONSOLE);
				free(long_str_alloced);
			}
			else {
				res = CMD_RES_ERROR;
			}
		}
		else {
			res = CMD_ExecuteCommand(tmpA, COMMAND_FLAG_SOURCE_CONSOLE);
		}
		LOG_SetCommandHTTPRedirectReply(0);
		resStr = CMD_GetResultString(res);
		hprintf255(request, "<h3>%s</h3>", resStr);
		poststr(request, "<br>");
	}
	add_label_text_field(request, "Command", "cmd", tmpA, "<form action=\"/cmd_tool\">");
	poststr(request, SUBMIT_AND_END_FORM);

	poststr(request, htmlFooterReturnToCfgOrMainPage);
	http_html_end(request);
	poststr(request, NULL);
	return 0;
}

int http_fn_startup_command(http_request_t* request) {
	char tmpA[512];
	http_setup(request, httpMimeTypeHTML);
	http_html_start(request, "Set startup command");
	poststr_h4(request, "Set/Change/Clear startup command line");
	poststr(request, "<p>Startup command is a shorter, smaller alternative to LittleFS autoexec.bat. "
		"The startup commands are run at device startup. "
		"You can use them to init peripherals and drivers, like BL0942 energy sensor. "
		"Use backlog cmd1; cmd2; cmd3; etc to enter multiple commands</p>");

	if (http_getArg(request->url, "startup_cmd", tmpA, sizeof(tmpA))) {
		http_getArg(request->url, "data", tmpA, sizeof(tmpA));
		//  hprintf255(request,"<h3>Set command to  %s!</h3>",tmpA);
		// tmpA can be longer than 128 bytes and this would crash
		hprintf255(request, "<h3>Command changed!</h3>");
		CFG_SetShortStartupCommand(tmpA);
		CFG_Save_IfThereArePendingChanges();
	}

	add_label_text_field(request, "Startup command", "data", CFG_GetShortStartupCommand(), "<form action=\"/startup_command\">");
	poststr(request, "<input type='hidden' name='startup_cmd' value='1'>");
	poststr(request, SUBMIT_AND_END_FORM);

	poststr(request, htmlFooterReturnToCfgOrMainPage);
	http_html_end(request);
	poststr(request, NULL);
	return 0;
}

void doHomeAssistantDiscovery(const char* topic, http_request_t* request) {
	int i;
	int relayCount;
	int pwmCount;
	int dInputCount;
	int excludedCount = 0;
	bool ledDriverChipRunning;
	HassDeviceInfo* dev_info = NULL;
	bool measuringPower = false;
	bool measuringBattery = false;
	struct cJSON_Hooks hooks;
	bool discoveryQueued = false;
	int type;
	// warning - this is 32 bit
	int flagsChannelPublished;
	int ch;
	int dimmer, toggle, brightness_scale;

	// no channels published yet
	flagsChannelPublished = 0;

	for (i = 0; i < CHANNEL_MAX; i++) {
		if (CHANNEL_HasNeverPublishFlag(i)) {
			BIT_SET(flagsChannelPublished, i);
			excludedCount++;
		}
	}
	
	if (topic == 0 || *topic == 0) {
		topic = "homeassistant";
	}

#ifdef ENABLE_DRIVER_BL0937
	measuringPower = DRV_IsMeasuringPower();
	measuringBattery = DRV_IsMeasuringBattery();
#endif

	PIN_get_Relay_PWM_Count(&relayCount, &pwmCount, &dInputCount);
	addLogAdv(LOG_INFO, LOG_FEATURE_HTTP, "HASS counts: %i rels, %i pwms, %i inps, %i excluded", relayCount, pwmCount, dInputCount, excludedCount);

	ledDriverChipRunning = LED_IsLedDriverChipRunning();

	hooks.malloc_fn = os_malloc;
	hooks.free_fn = os_free;
	cJSON_InitHooks(&hooks);


#if ENABLE_ADVANCED_CHANNELTYPES_DISCOVERY
	// try to pair toggles with dimmers. This is needed only for TuyaMCU, 
	// where custom channel types are used. This is NOT used for simple
	// CW/RGB/RGBCW/etc lights.
	if (CFG_HasFlag(OBK_FLAG_DISCOVERY_DONT_MERGE_LIGHTS) == false) {
		while (true) {
			// find first dimmer
			dimmer = -1;
			for (i = 0; i < CHANNEL_MAX; i++) {
				type = g_cfg.pins.channelTypes[i];
				if (BIT_CHECK(flagsChannelPublished, i)) {
					continue;
				}
				if (type == ChType_Dimmer) {
					brightness_scale = 100;
					dimmer = i;
					break;
				}
				if (type == ChType_Dimmer1000) {
					brightness_scale = 1000;
					dimmer = i;
					break;
				}
				if (type == ChType_Dimmer256) {
					brightness_scale = 256;
					dimmer = i;
					break;
				}
			}
			// find first togle
			toggle = -1;
			for (i = 0; i < CHANNEL_MAX; i++) {
				type = g_cfg.pins.channelTypes[i];
				if (BIT_CHECK(flagsChannelPublished, i)) {
					continue;
				}
				if (type == ChType_Toggle) {
					toggle = i;
					break;
				}
			}
			// if nothing found, stop
			if (toggle == -1 || dimmer == -1) {
				break;
			}

			BIT_SET(flagsChannelPublished, toggle);
			BIT_SET(flagsChannelPublished, dimmer);
			dev_info = hass_init_light_singleColor_onChannels(toggle, dimmer, brightness_scale);
			MQTT_QueuePublish(topic, dev_info->channel, hass_build_discovery_json(dev_info), OBK_PUBLISH_FLAG_RETAIN);
			hass_free_device_info(dev_info);
		}
	}
#endif


	if (pwmCount == 5 || ledDriverChipRunning || (pwmCount == 4 && CFG_HasFlag(OBK_FLAG_LED_EMULATE_COOL_WITH_RGB))) {
		if (dev_info == NULL) {
			dev_info = hass_init_light_device_info(LIGHT_RGBCW);
		}
		// Enable + RGB control + CW control
		MQTT_QueuePublish(topic, dev_info->channel, hass_build_discovery_json(dev_info), OBK_PUBLISH_FLAG_RETAIN);
		hass_free_device_info(dev_info);
		dev_info = NULL;
		discoveryQueued = true;
	}
	else if (pwmCount > 0) {
		if (pwmCount == 4) {
			addLogAdv(LOG_ERROR, LOG_FEATURE_HTTP, "4 PWM device not yet handled\r\n");
		}
		else if (pwmCount == 3) {
			// Enable + RGB control
			dev_info = hass_init_light_device_info(LIGHT_RGB);
		}
		else if (pwmCount == 2) {
			// PWM + Temperature (https://github.com/openshwprojects/OpenBK7231T_App/issues/279)
			dev_info = hass_init_light_device_info(LIGHT_PWMCW);
		}
		else {
			dev_info = hass_init_light_device_info(LIGHT_PWM);
		}

		if (dev_info != NULL) {
			MQTT_QueuePublish(topic, dev_info->channel, hass_build_discovery_json(dev_info), OBK_PUBLISH_FLAG_RETAIN);
			hass_free_device_info(dev_info);
			dev_info = NULL;
			discoveryQueued = true;
		}
	}

#ifdef ENABLE_DRIVER_BL0937
	if (measuringPower == true) {
		for (i = OBK__FIRST; i <= OBK__LAST; i++)
		{
			dev_info = hass_init_energy_sensor_device_info(i);
			if (dev_info) {
				MQTT_QueuePublish(topic, dev_info->channel, hass_build_discovery_json(dev_info), OBK_PUBLISH_FLAG_RETAIN);
				hass_free_device_info(dev_info);
				discoveryQueued = true;
			}
		}
	}
#endif

	if (measuringBattery == true) {
		dev_info = hass_init_sensor_device_info(BATTERY_SENSOR, 0, -1, -1, 1);
		MQTT_QueuePublish(topic, dev_info->channel, hass_build_discovery_json(dev_info), OBK_PUBLISH_FLAG_RETAIN);
		hass_free_device_info(dev_info);

		dev_info = hass_init_sensor_device_info(BATTERY_VOLTAGE_SENSOR, 0, -1, -1, 1);
		MQTT_QueuePublish(topic, dev_info->channel, hass_build_discovery_json(dev_info), OBK_PUBLISH_FLAG_RETAIN);
		hass_free_device_info(dev_info);

		discoveryQueued = true;
	}

	for (i = 0; i < PLATFORM_GPIO_MAX; i++) {
		if (IS_PIN_DHT_ROLE(g_cfg.pins.roles[i]) || IS_PIN_TEMP_HUM_SENSOR_ROLE(g_cfg.pins.roles[i])) {
			ch = PIN_GetPinChannelForPinIndex(i);
			// TODO: flags are 32 bit and there are 64 max channels
			BIT_SET(flagsChannelPublished, ch);
			dev_info = hass_init_sensor_device_info(TEMPERATURE_SENSOR, ch, 2, 1, 1);
			MQTT_QueuePublish(topic, dev_info->channel, hass_build_discovery_json(dev_info), OBK_PUBLISH_FLAG_RETAIN);
			hass_free_device_info(dev_info);

			ch = PIN_GetPinChannel2ForPinIndex(i);
			// TODO: flags are 32 bit and there are 64 max channels
			BIT_SET(flagsChannelPublished, ch);
			dev_info = hass_init_sensor_device_info(HUMIDITY_SENSOR, ch, -1, -1, 1);
			MQTT_QueuePublish(topic, dev_info->channel, hass_build_discovery_json(dev_info), OBK_PUBLISH_FLAG_RETAIN);
			hass_free_device_info(dev_info);

			discoveryQueued = true;
		}
		else if (IS_PIN_AIR_SENSOR_ROLE(g_cfg.pins.roles[i])) {
			ch = PIN_GetPinChannelForPinIndex(i);
			// TODO: flags are 32 bit and there are 64 max channels
			BIT_SET(flagsChannelPublished, ch);
			dev_info = hass_init_sensor_device_info(CO2_SENSOR, ch, -1, -1, 1);
			MQTT_QueuePublish(topic, dev_info->channel, hass_build_discovery_json(dev_info), OBK_PUBLISH_FLAG_RETAIN);
			hass_free_device_info(dev_info);

			ch = PIN_GetPinChannel2ForPinIndex(i);
			// TODO: flags are 32 bit and there are 64 max channels
			BIT_SET(flagsChannelPublished, ch);
			dev_info = hass_init_sensor_device_info(TVOC_SENSOR, ch, -1, -1, 1);
			MQTT_QueuePublish(topic, dev_info->channel, hass_build_discovery_json(dev_info), OBK_PUBLISH_FLAG_RETAIN);
			hass_free_device_info(dev_info);

			discoveryQueued = true;
		}
	}
#if ENABLE_ADVANCED_CHANNELTYPES_DISCOVERY
	for (i = 0; i < CHANNEL_MAX; i++) {
		type = g_cfg.pins.channelTypes[i];
		// TODO: flags are 32 bit and there are 64 max channels
		if (BIT_CHECK(flagsChannelPublished, i)) {
			continue;
		}
		dev_info = 0;
		switch (type)
		{
			case ChType_Motion:
			{
				dev_info = hass_init_binary_sensor_device_info(i, true);
				cJSON_AddStringToObject(dev_info->root, "dev_cla", "motion");
			}
			break;
			case ChType_Motion_n:
			{
				dev_info = hass_init_binary_sensor_device_info(i, false);
				cJSON_AddStringToObject(dev_info->root, "dev_cla", "motion");
			}
			break;
			case ChType_OpenClosed:
			{
				dev_info = hass_init_binary_sensor_device_info(i, false);
			}
			break;
			case ChType_OpenClosed_Inv:
			{
				dev_info = hass_init_binary_sensor_device_info(i, true);
			}
			break;
			case ChType_Voltage_div10:
			{
				dev_info = hass_init_sensor_device_info(VOLTAGE_SENSOR, i, 2, 1, 1);
			}
			break;
			case ChType_Voltage_div100:
			{
				dev_info = hass_init_sensor_device_info(VOLTAGE_SENSOR, i, 2, 2, 1);
			}
			break;
			case ChType_ReadOnlyLowMidHigh:
			{
				dev_info = hass_init_sensor_device_info(READONLYLOWMIDHIGH_SENSOR, i, -1, -1, 1);
			}
			break;
			case ChType_SmokePercent:
			{
				dev_info = hass_init_sensor_device_info(SMOKE_SENSOR, i, -1, -1, 1);
			}
			break;
			case ChType_Illuminance:
			{
				dev_info = hass_init_sensor_device_info(ILLUMINANCE_SENSOR, i, -1, -1, 1);
			}
			break;
			case ChType_Custom:
			case ChType_ReadOnly:
			{
				dev_info = hass_init_sensor_device_info(CUSTOM_SENSOR, i, -1, -1, 1);
			}
			break;
			case ChType_Temperature:
			{
				dev_info = hass_init_sensor_device_info(TEMPERATURE_SENSOR, i, -1, -1, 1);
			}
			break;
			case ChType_Temperature_div2:
			{
				dev_info = hass_init_sensor_device_info(TEMPERATURE_SENSOR, i, 2, 1, 5);
			}
			break;
			case ChType_Temperature_div10:
			{
				dev_info = hass_init_sensor_device_info(TEMPERATURE_SENSOR, i, 2, 1, 1);
			}
			break;
			case ChType_ReadOnly_div10:
			{
				dev_info = hass_init_sensor_device_info(CUSTOM_SENSOR, i, 2, 1, 1);
			}
			break;
			case ChType_Temperature_div100:
			{
				dev_info = hass_init_sensor_device_info(TEMPERATURE_SENSOR, i, 2, 2, 1);
			}
			break;
			case ChType_ReadOnly_div100:
			{
				dev_info = hass_init_sensor_device_info(CUSTOM_SENSOR, i, 2, 2, 1);
			}
			break;
			case ChType_Humidity:
			{
				dev_info = hass_init_sensor_device_info(HUMIDITY_SENSOR, i, -1, -1, 1);
			}
			break;
			case ChType_Humidity_div10:
			{
				dev_info = hass_init_sensor_device_info(HUMIDITY_SENSOR, i, 2, 1, 1);
			}
			break;
			case ChType_Current_div100:
			{
				dev_info = hass_init_sensor_device_info(CURRENT_SENSOR, i, 3, 2, 1);
			}
			break;
			case ChType_ReadOnly_div1000:
			{
				dev_info = hass_init_sensor_device_info(CUSTOM_SENSOR, i, 3, 3, 1);
			}
			break;
			case ChType_LeakageCurrent_div1000:
			case ChType_Current_div1000:
			{
				dev_info = hass_init_sensor_device_info(CURRENT_SENSOR, i, 3, 3, 1);
			}
			break;
			case ChType_Power:
			{
				dev_info = hass_init_sensor_device_info(POWER_SENSOR, i, -1, -1, 1);
			}
			break;
			case ChType_Power_div10:
			{
				dev_info = hass_init_sensor_device_info(POWER_SENSOR, i, 2, 1, 1);
			}
			break;
			case ChType_Power_div100:
			{
				dev_info = hass_init_sensor_device_info(POWER_SENSOR, i, 3, 2, 1);
			}
			break;
			case ChType_PowerFactor_div100:
			{
				dev_info = hass_init_sensor_device_info(POWERFACTOR_SENSOR, i, 3, 2, 1);
			}
			break;
			case ChType_Pressure_div100:
			{
				dev_info = hass_init_sensor_device_info(PRESSURE_SENSOR, i, 3, 2, 1);
			}
			break;
			case ChType_PowerFactor_div1000:
			{
				dev_info = hass_init_sensor_device_info(POWERFACTOR_SENSOR, i, 4, 3, 1);
			}
			break;
			case ChType_Frequency_div100:
			{
				dev_info = hass_init_sensor_device_info(FREQUENCY_SENSOR, i, 3, 2, 1);
			}
			break;
			case ChType_Frequency_div10:
			{
				dev_info = hass_init_sensor_device_info(FREQUENCY_SENSOR, i, 3, 1, 1);
			}
			break;
			case ChType_EnergyTotal_kWh_div100:
			{
				dev_info = hass_init_sensor_device_info(ENERGY_SENSOR, i, 3, 2, 1);
			}
			break;
			case ChType_EnergyTotal_kWh_div1000:
			{
				dev_info = hass_init_sensor_device_info(ENERGY_SENSOR, i, 3, 3, 1);
			}
			break;
			case ChType_Ph:
			{
				dev_info = hass_init_sensor_device_info(WATER_QUALITY_PH, i, 2, 2, 1);
			}
			break;
			case ChType_Orp:
			{
				dev_info = hass_init_sensor_device_info(WATER_QUALITY_ORP, i, -1, 2, 1);
			}
			break;
			case ChType_Tds:
			{
				dev_info = hass_init_sensor_device_info(WATER_QUALITY_TDS, i, -1, 2, 1);
			}
			break;
		}
		if (dev_info) {
			MQTT_QueuePublish(topic, dev_info->channel, hass_build_discovery_json(dev_info), OBK_PUBLISH_FLAG_RETAIN);
			hass_free_device_info(dev_info);

			BIT_SET(flagsChannelPublished, i);
			discoveryQueued = true;
		}
	}
#endif

	//if (relayCount > 0) {
	for (i = 0; i < CHANNEL_MAX; i++) {
		// if already included by light, skip
		if (BIT_CHECK(flagsChannelPublished, i)) {
			continue;
		}
		bool bToggleInv = g_cfg.pins.channelTypes[i] == ChType_Toggle_Inv;
		if (h_isChannelRelay(i) || g_cfg.pins.channelTypes[i] == ChType_Toggle || bToggleInv) {
			// TODO: flags are 32 bit and there are 64 max channels
			BIT_SET(flagsChannelPublished, i);
			if (CFG_HasFlag(OBK_FLAG_MQTT_HASS_ADD_RELAYS_AS_LIGHTS)) {
				dev_info = hass_init_relay_device_info(i, LIGHT_ON_OFF, bToggleInv);
			}
			else {
				dev_info = hass_init_relay_device_info(i, RELAY, bToggleInv);
			}
			MQTT_QueuePublish(topic, dev_info->channel, hass_build_discovery_json(dev_info), OBK_PUBLISH_FLAG_RETAIN);
			hass_free_device_info(dev_info);
			dev_info = NULL;
			discoveryQueued = true;
		}
	}
	//}
	if (dInputCount > 0) {
		for (i = 0; i < CHANNEL_MAX; i++) {
			if (h_isChannelDigitalInput(i)) {
				if (BIT_CHECK(flagsChannelPublished, i)) {
					continue;
				}
				// TODO: flags are 32 bit and there are 64 max channels
				BIT_SET(flagsChannelPublished, i);
				dev_info = hass_init_binary_sensor_device_info(i, false);
				MQTT_QueuePublish(topic, dev_info->channel, hass_build_discovery_json(dev_info), OBK_PUBLISH_FLAG_RETAIN);
				hass_free_device_info(dev_info);
				dev_info = NULL;
				discoveryQueued = true;
			}
		}
	}
	if (1) {
		//use -1 for channel as these don't correspond to channels
		dev_info = hass_init_sensor_device_info(HASS_TEMP, -1, -1, -1, 1);
		MQTT_QueuePublish(topic, dev_info->channel, hass_build_discovery_json(dev_info), OBK_PUBLISH_FLAG_RETAIN);
		dev_info = hass_init_sensor_device_info(HASS_RSSI, -1, -1, -1, 1);
		MQTT_QueuePublish(topic, dev_info->channel, hass_build_discovery_json(dev_info), OBK_PUBLISH_FLAG_RETAIN);
		hass_free_device_info(dev_info);
		dev_info = hass_init_sensor_device_info(HASS_UPTIME, -1, -1, -1, 1);
		MQTT_QueuePublish(topic, dev_info->channel, hass_build_discovery_json(dev_info), OBK_PUBLISH_FLAG_RETAIN);
		hass_free_device_info(dev_info);
		dev_info = hass_init_sensor_device_info(HASS_BUILD, -1, -1, -1, 1);
		MQTT_QueuePublish(topic, dev_info->channel, hass_build_discovery_json(dev_info), OBK_PUBLISH_FLAG_RETAIN);
		hass_free_device_info(dev_info);
		dev_info = hass_init_sensor_device_info(HASS_SSID, -1, -1, -1, 1);
		MQTT_QueuePublish(topic, dev_info->channel, hass_build_discovery_json(dev_info), OBK_PUBLISH_FLAG_RETAIN);
		hass_free_device_info(dev_info);
		dev_info = hass_init_sensor_device_info(HASS_IP, -1, -1, -1, 1);
		MQTT_QueuePublish(topic, dev_info->channel, hass_build_discovery_json(dev_info), OBK_PUBLISH_FLAG_RETAIN);
		hass_free_device_info(dev_info);
		discoveryQueued = true;

	}
	if (discoveryQueued) {
		MQTT_InvokeCommandAtEnd(PublishChannels);
	}
	else {
		const char* msg = "No relay, PWM, sensor or power driver running.";
		if (request) {
			poststr(request, msg);
			poststr(request, NULL);
		}
		else {
			addLogAdv(LOG_ERROR, LOG_FEATURE_HTTP, "HA discovery: %s\r\n", msg);
		}
	}
}

/// @brief Sends HomeAssistant discovery MQTT messages.
/// @param request 
/// @return 
int http_fn_ha_discovery(http_request_t* request) {
	char topic[32];

	http_setup(request, httpMimeTypeText);

	if (MQTT_IsReady() == false) {
		poststr(request, "MQTT not running.");
		poststr(request, NULL);
		return 0;
	}

	// even if it returns the empty HA topic,
	// the function call below will set default
	http_getArg(request->url, "prefix", topic, sizeof(topic));
	doHomeAssistantDiscovery(topic, request);

	poststr(request, "MQTT discovery queued.");
	poststr(request, NULL);
	return 0;
}

void http_generate_singleColor_cfg(http_request_t* request, const char* clientId) {
	hprintf255(request, "    command_topic: \"cmnd/%s/led_enableAll\"\n", clientId);
	hprintf255(request, "    state_topic: \"%s/led_enableAll/get\"\n", clientId);
	hprintf255(request, "    availability_topic: \"%s/connected\"\n", clientId);
	hprintf255(request, "    payload_on: 1\n");
	hprintf255(request, "    payload_off: 0\n");
	hprintf255(request, "    brightness_command_topic: \"cmnd/%s/led_dimmer\"\n", clientId);
	hprintf255(request, "    brightness_state_topic: \"%s/led_dimmer/get\"\n", clientId);
	hprintf255(request, "    brightness_scale: 100\n");
}
void http_generate_rgb_cfg(http_request_t* request, const char* clientId) {
	hprintf255(request, "    rgb_command_template: \"{{ '#%%02x%%02x%%02x0000' | format(red, green, blue)}}\"\n");
	hprintf255(request, "    rgb_value_template: \"{{ value[0:2]|int(base=16) }},{{ value[2:4]|int(base=16) }},{{ value[4:6]|int(base=16) }}\"\n");
	hprintf255(request, "    rgb_state_topic: \"%s/led_basecolor_rgb/get\"\n", clientId);
	hprintf255(request, "    rgb_command_topic: \"cmnd/%s/led_basecolor_rgb\"\n", clientId);

	http_generate_singleColor_cfg(request, clientId);
}
void http_generate_cw_cfg(http_request_t* request, const char* clientId) {
	hprintf255(request, "    color_temp_command_topic: \"cmnd/%s/led_temperature\"\n", clientId);
	hprintf255(request, "    color_temp_state_topic: \"%s/led_temperature/get\"\n", clientId);
	http_generate_singleColor_cfg(request, clientId);
}

void hprintf_qos_payload(http_request_t* request, const char* clientId) {
	poststr(request, "    qos: 1\n");
	poststr(request, "    payload_on: 1\n");
	poststr(request, "    payload_off: 0\n");
	poststr(request, "    retain: true\n");
	hprintf255(request, "    availability:\n");
	hprintf255(request, "      - topic: \"%s/connected\"\n", clientId);
}
int http_fn_ha_cfg(http_request_t* request) {
	int relayCount;
	int pwmCount;
	int dInputCount;
	const char* shortDeviceName;
	const char* clientId;
	int i;
	char mqttAdded = 0;
	char switchAdded = 0;
	char lightAdded = 0;

	i = 0;

	shortDeviceName = CFG_GetShortDeviceName();
	clientId = CFG_GetMQTTClientId();

	http_setup(request, httpMimeTypeHTML);
	http_html_start(request, "Home Assistant Setup");
	poststr_h4(request, "Home Assistant Cfg");
	hprintf255(request, "<h4>Note that your short device name is: %s</h4>", shortDeviceName);
	poststr_h4(request, "Paste this to configuration yaml");
	poststr(request, "<h5>Make sure that you have \"switch:\" keyword only once! Home Assistant doesn't like dup keywords.</h5>");
	poststr(request, "<h5>You can also use \"switch MyDeviceName:\" to avoid keyword duplication!</h5>");

	poststr(request, "<textarea rows=\"40\" cols=\"50\">");

	PIN_get_Relay_PWM_Count(&relayCount, &pwmCount, &dInputCount);

	if (relayCount > 0) {

		for (i = 0; i < CHANNEL_MAX; i++) {
			if (h_isChannelRelay(i)) {
				if (mqttAdded == 0) {
					poststr(request, "mqtt:\n");
					mqttAdded = 1;
				}
				if (switchAdded == 0) {
					poststr(request, "  switch:\n");
					switchAdded = 1;
				}

				hass_print_unique_id(request, "  - unique_id: \"%s\"\n", RELAY, i);
				hprintf255(request, "    name: %i\n", i);
				hprintf255(request, "    state_topic: \"%s/%i/get\"\n", clientId, i);
				hprintf255(request, "    command_topic: \"%s/%i/set\"\n", clientId, i);
				hprintf_qos_payload(request, clientId);
			}
		}
	}
	if (dInputCount > 0) {
		for (i = 0; i < CHANNEL_MAX; i++) {
			if (h_isChannelDigitalInput(i)) {
				if (mqttAdded == 0) {
					poststr(request, "mqtt:\n");
					mqttAdded = 1;
				}
				if (switchAdded == 0) {
					poststr(request, "  binary_sensor:\n");
					switchAdded = 1;
				}

				hass_print_unique_id(request, "  - unique_id: \"%s\"\n", BINARY_SENSOR, i);
				hprintf255(request, "    name: %i\n", i);
				hprintf255(request, "    state_topic: \"%s/%i/get\"\n", clientId, i);
				hprintf_qos_payload(request, clientId);
			}
		}
	}
	if (pwmCount == 5 || LED_IsLedDriverChipRunning()) {
		// Enable + RGB control + CW control
		if (mqttAdded == 0) {
			poststr(request, "mqtt:\n");
			mqttAdded = 1;
		}
		if (switchAdded == 0) {
			poststr(request, "  light:\n");
			switchAdded = 1;
		}

		hass_print_unique_id(request, "  - unique_id: \"%s\"\n", LIGHT_RGBCW, i);
		hprintf255(request, "    name: %i\n", i);
		http_generate_rgb_cfg(request, clientId);
		//hprintf255(request, "    #brightness_value_template: \"{{ value }}\"\n");
		hprintf255(request, "    color_temp_command_topic: \"cmnd/%s/led_temperature\"\n", clientId);
		hprintf255(request, "    color_temp_state_topic: \"%s/led_temperature/get\"\n", clientId);
		//hprintf255(request, "    #color_temp_value_template: \"{{ value }}\"\n");
	}
	else
		if (pwmCount == 3) {
			// Enable + RGB control
			if (mqttAdded == 0) {
				poststr(request, "mqtt:\n");
				mqttAdded = 1;
			}
			if (switchAdded == 0) {
				poststr(request, "  light:\n");
				switchAdded = 1;
			}

			hass_print_unique_id(request, "  - unique_id: \"%s\"\n", LIGHT_RGB, i);
			hprintf255(request, "    name: Light\n");
			http_generate_rgb_cfg(request, clientId);
		}
		else if (pwmCount == 1) {
			// single color
			if (mqttAdded == 0) {
				poststr(request, "mqtt:\n");
				mqttAdded = 1;
			}
			if (switchAdded == 0) {
				poststr(request, "  light:\n");
				switchAdded = 1;
			}

			hass_print_unique_id(request, "  - unique_id: \"%s\"\n", LIGHT_PWM, i);
			hprintf255(request, "    name: Light\n");
			http_generate_singleColor_cfg(request, clientId);
		}
		else if (pwmCount == 2) {
			// CW
			if (mqttAdded == 0) {
				poststr(request, "mqtt:\n");
				mqttAdded = 1;
			}
			if (switchAdded == 0) {
				poststr(request, "  light:\n");
				switchAdded = 1;
			}

			hass_print_unique_id(request, "  - unique_id: \"%s\"\n", LIGHT_PWMCW, i);
			hprintf255(request, "    name: Light\n");
			http_generate_cw_cfg(request, clientId);
		}
		else if (pwmCount > 0) {

			for (i = 0; i < CHANNEL_MAX; i++) {
				if (h_isChannelPWM(i)) {
					if (mqttAdded == 0) {
						poststr(request, "mqtt:\n");
						mqttAdded = 1;
					}
					if (lightAdded == 0) {
						poststr(request, "  light:\n");
						lightAdded = 1;
					}

					hass_print_unique_id(request, "  - unique_id: \"%s\"\n", LIGHT_PWM, i);
					hprintf255(request, "    name: %i\n", i);
					hprintf255(request, "    state_topic: \"%s/%i/get\"\n", clientId, i);
					hprintf255(request, "    command_topic: \"%s/%i/set\"\n", clientId, i);
					hprintf255(request, "    brightness_command_topic: \"%s/%i/set\"\n", clientId, i);
					poststr(request, "    on_command_type: \"brightness\"\n");
					poststr(request, "    brightness_scale: 99\n");
					poststr(request, "    qos: 1\n");
					poststr(request, "    payload_on: 99\n");
					poststr(request, "    payload_off: 0\n");
					poststr(request, "    retain: true\n");
					poststr(request, "    optimistic: true\n");
					hprintf255(request, "    availability:\n");
					hprintf255(request, "      - topic: \"%s/connected\"\n", clientId);
				}
			}
		}

	poststr(request, "</textarea>");
	poststr(request, "<br/><div><label for=\"ha_disc_topic\">Discovery topic:</label><input id=\"ha_disc_topic\" value=\"homeassistant\"><button onclick=\"send_ha_disc();\">Start Home Assistant Discovery</button>&nbsp;<form action=\"cfg_mqtt\" class='disp-inline'><button type=\"submit\">Configure MQTT</button></form></div><br/>");
	poststr(request, htmlFooterReturnToCfgOrMainPage);
	http_html_end(request);
	poststr(request, ha_discovery_script);
	poststr(request, NULL);
	return 0;
}

void runHTTPCommandInternal(http_request_t* request, const char *cmd) {
	bool bEchoHack = strncmp(cmd, "echo", 4) == 0;
	CMD_ExecuteCommand(cmd, COMMAND_FLAG_SOURCE_HTTP);
#if ENABLE_TASMOTA_JSON
	if (!bEchoHack) {
		JSON_ProcessCommandReply(cmd, skipToNextWord(cmd), request, (jsonCb_t)hprintf255, COMMAND_FLAG_SOURCE_HTTP);
	}
	else {
		const char *s = Tokenizer_GetArg(0);
		poststr(request, s);
	}
#endif
}
int http_fn_cm(http_request_t* request) {
	char tmpA[128];
	char* long_str_alloced = 0;
	int commandLen = 0;

	http_setup(request, httpMimeTypeJson);
	// exec command
	if (request->method == HTTP_GET) {
		commandLen = http_getArg(request->url, "cmnd", tmpA, sizeof(tmpA));
		//ADDLOG_INFO(LOG_FEATURE_HTTP, "Got here (GET) %s;%s;%d\n", request->url, tmpA, commandLen);
    } else if (request->method == HTTP_POST || request->method == HTTP_PUT) {
		commandLen = http_getRawArg(request->bodystart, "cmnd", tmpA, sizeof(tmpA));
		//ADDLOG_INFO(LOG_FEATURE_HTTP, "Got here (POST) %s;%s;%d\n", request->bodystart, tmpA, commandLen);
    }
	if (commandLen) {
		if (commandLen > (sizeof(tmpA) - 5)) {
			commandLen += 8;
			long_str_alloced = (char*)malloc(commandLen);
			if (long_str_alloced) {
				if (request->method == HTTP_GET) {
					http_getArg(request->url, "cmnd", long_str_alloced, commandLen);
				} else if (request->method == HTTP_POST || request->method == HTTP_PUT) {
					http_getRawArg(request->bodystart, "cmnd", long_str_alloced, commandLen);
				}
				CMD_ExecuteCommand(long_str_alloced, COMMAND_FLAG_SOURCE_HTTP);

				runHTTPCommandInternal(request, long_str_alloced);

				free(long_str_alloced);
			}
		}
		else {
			runHTTPCommandInternal(request, tmpA);
		}
	}

	poststr(request, NULL);


	return 0;
}

int http_fn_cfg(http_request_t* request) {
	http_setup(request, httpMimeTypeHTML);
	http_html_start(request, "Config");
	postFormAction(request, "cfg_pins", "Configure Module");
	postFormAction(request, "cfg_generic", "Configure General/Flags");
	postFormAction(request, "cfg_startup", "Configure Startup");
	postFormAction(request, "cfg_dgr", "Configure Device Groups");
	postFormAction(request, "cfg_wifi", "Configure WiFi &amp; Web");
	postFormAction(request, "cfg_ip", "Configure IP");
	postFormAction(request, "cfg_mqtt", "Configure MQTT");
	postFormAction(request, "cfg_name", "Configure Names");
	postFormAction(request, "cfg_mac", "Change MAC");
	postFormAction(request, "cfg_ping", "Ping Watchdog (network lost restarter)");
	postFormAction(request, "cfg_webapp", "Configure WebApp");
	postFormAction(request, "ha_cfg", "Home Assistant Configuration");
	postFormAction(request, "ota", "OTA (update software by WiFi)");
	postFormAction(request, "cmd_tool", "Execute Custom Command");
	//postFormAction(request, "flash_read_tool", "Flash Read Tool");
	postFormAction(request, "startup_command", "Change Startup Command Text");

#if 0
#if PLATFORM_BK7231T | PLATFORM_BK7231N
	{
		int i, j, k;
		k = config_get_tableOffsets(BK_PARTITION_NET_PARAM, &i, &j);
		hprintf255(request, "BK_PARTITION_NET_PARAM: bOk %i, at %i, len %i<br>", k, i, j);
		k = config_get_tableOffsets(BK_PARTITION_RF_FIRMWARE, &i, &j);
		hprintf255(request, "BK_PARTITION_RF_FIRMWARE: bOk %i, at %i, len %i<br>", k, i, j);
		k = config_get_tableOffsets(BK_PARTITION_OTA, &i, &j);
		hprintf255(request, "BK_PARTITION_OTA: bOk %i, at %i, len %i<br>", k, i, j);
	}
#endif
#endif
	poststr(request, htmlFooterReturnToMainPage);
	http_html_end(request);
	poststr(request, NULL);
	return 0;
}

int http_fn_cfg_pins(http_request_t* request) {
	int iChanged = 0;
	int iChangedRequested = 0;
	int i;
	char tmpA[128];
	char tmpB[64];

	http_setup(request, httpMimeTypeHTML);
	http_html_start(request, "Pin config");
	poststr(request, "<p>The first field assigns a role to the given pin. The next field is used to enter channel index (relay index), used to support multiple relays and buttons. ");
	poststr(request, "So, first button and first relay should have channel 1, second button and second relay have channel 2, etc.</p>");
	poststr(request, "<p>Only for button roles another field will be provided to enter channel to toggle when doing double click. ");
	poststr(request, "It shows up when you change role to button and save.</p>");
#if PLATFORM_BK7231N || PLATFORM_BK7231T
	poststr(request, "<p>BK7231N/BK7231T supports PWM only on pins 6, 7, 8, 9, 24 and 26!</p>");
#endif
	for (i = 0; i < PLATFORM_GPIO_MAX; i++) {
		sprintf(tmpA, "%i", i);
		if (http_getArg(request->url, tmpA, tmpB, sizeof(tmpB))) {
			int role;
			int pr;

			iChangedRequested++;

			role = atoi(tmpB);

			pr = PIN_GetPinRoleForPinIndex(i);
			if (pr != role) {
				PIN_SetPinRoleForPinIndex(i, role);
				iChanged++;
			}
		}
		sprintf(tmpA, "r%i", i);
		if (http_getArg(request->url, tmpA, tmpB, sizeof(tmpB))) {
			int rel;
			int prevRel;

			iChangedRequested++;

			rel = atoi(tmpB);

			prevRel = PIN_GetPinChannelForPinIndex(i);
			if (prevRel != rel) {
				PIN_SetPinChannelForPinIndex(i, rel);
				iChanged++;
			}
		}
		sprintf(tmpA, "e%i", i);
		if (http_getArg(request->url, tmpA, tmpB, sizeof(tmpB))) {
			int rel;
			int prevRel;

			iChangedRequested++;

			rel = atoi(tmpB);

			prevRel = PIN_GetPinChannel2ForPinIndex(i);
			if (prevRel != rel) {
				PIN_SetPinChannel2ForPinIndex(i, rel);
				iChanged++;
			}
		}
	}
	if (iChangedRequested > 0) {
		// Anecdotally, if pins are configured badly, the
		// second-timer breaks. To reconfigure, force
		// saving the configuration instead of waiting.
		//CFG_Save_SetupTimer(); 
		CFG_Save_IfThereArePendingChanges();

		// Invoke Hass discovery if configuration has changed and not in safe mode.
		if (!bSafeMode && CFG_HasFlag(OBK_FLAG_AUTOMAIC_HASS_DISCOVERY)) {
			Main_ScheduleHomeAssistantDiscovery(1);
		}
		hprintf255(request, "Pins update - %i reqs, %i changed!<br><br>", iChangedRequested, iChanged);
	}
	//	strcat(outbuf,"<button type=\"button\">Click Me!</button>");
	poststr(request, "<form action=\"cfg_pins\" id=\"x\">");


	poststr(request, "<script> var r = [");
	for (i = 0; i < IOR_Total_Options; i++) {
		if (i) {
			poststr(request, ",");
		}
		// print array with ["name_of_role",<Number of channnels for this role>]
		hprintf255(request, "[\"%s\",%i]", htmlPinRoleNames[i],PIN_IOR_NofChan(i));
	}
	poststr(request, "];");

	poststr(request, "var  sr = r.map((e,i)=>{return e[0]+'#'+i}).sort(Intl.Collator().compare).map(e=>e.split('#'));");
	
	poststr(request, "function hide_show() {"
		"n=this.name;"
		"er=getElement('r'+n);"
		"ee=getElement('e'+n);"
		"ch=r[this.value][1];"		// since we might have skiped PWM entries in options list, don't use "selectedIndex" but "value" (it's even shorter ;-)
		"er.disabled = (ch<1); er.style.display= ch<1 ? 'none' : 'inline';"
		"ee.disabled = (ch<2); ee.style.display= ch<2 ? 'none' : 'inline';"
		"}");

	poststr(request, "function f(alias, id, c, b, ch1, ch2) {"
		"let f = document.getElementById(\"x\");"
		"let d = document.createElement(\"div\");"
		"d.className = \"hdiv\";"
		"d.innerHTML = \"<span class='disp-inline' style='min-width: 15ch'>\"+alias+\"</span>\";"
		"f.appendChild(d);"
		"let s = document.createElement(\"select\");"
		"s.className = \"hele\";"
		"s.name = id;"
		"d.appendChild(s);"
		"	for (var i = 0; i < sr.length; i++) {"
		"	if(b && sr[i][0].startsWith(\"PWM\")) continue; "
		"var o = document.createElement(\"option\");"
		"	o.text = sr[i][0];"
		"	o.value = sr[i][1];"
		"	o.selected = (sr[i][1] == c);"
		"s.add(o);s.onchange = hide_show;"
		"}"
		"var y = document.createElement(\"input\");"
		"y.className = \"hele\";"
		"y.name = \"r\"+id;"
		"y.id = \"r\"+id;"
		"y.disabled = ch1==null;"
		"y.style.display = ch1==null ? 'none' :'inline' ;"
		"y.value = ch1==null ? 0 : ch1;"
		"d.appendChild(y);"
		"y = document.createElement(\"input\");"
		"y.className = \"hele\";"
		"y.name = \"e\"+id;"
		"y.id = \"e\"+id;"
		"y.disabled = ch2==null ;"
		"y.style.display = ch2==null ? 'none' :'inline' ;"
		"y.value = ch2==null ? 0 : ch2;"
		"d.appendChild(y);"
		" }");

	for (i = 0; i < PLATFORM_GPIO_MAX; i++) {
		// On BL602, any GPIO can be mapped to one of 5 PWM channels
		// But on Beken chips, only certain pins can be PWM
		int bCanThisPINbePWM;
		int si, ch, ch2;
		const char* alias;

		si = PIN_GetPinRoleForPinIndex(i);
		ch = PIN_GetPinChannelForPinIndex(i);
		ch2 = PIN_GetPinChannel2ForPinIndex(i);

		// if available..
		alias = HAL_PIN_GetPinNameAlias(i);

		bCanThisPINbePWM = HAL_PIN_CanThisPinBePWM(i);
		si = PIN_GetPinRoleForPinIndex(i);
		hprintf255(request, "f(\"");
		if (alias) {
#if defined(PLATFORM_BEKEN) || defined(WINDOWS)
			hprintf255(request, "P%i (%s) ", i, alias);
#else
			poststr(request, alias);
			poststr(request, " ");
#endif
		}
		else {
			hprintf255(request, "P%i ", i);
		}
		hprintf255(request, "\",%i,%i, %i,", i, si, !bCanThisPINbePWM);
		// Primary linked channel
		int NofC = PIN_IOR_NofChan(si);
		if (NofC >= 1)
		{
			hprintf255(request, "%i,", ch);
		}
		// Some roles do not need any channels
		else {
			hprintf255(request, "null,", ch);
		}
		// Secondary linked channel
		if (NofC > 1)
		{
			hprintf255(request, "%i,", ch2);
		}
		else {
			hprintf255(request, "null,", ch);
		}
		hprintf255(request, ");");

	}
	poststr(request, " </script>");
	poststr(request, "<input type=\"submit\" value=\"Save\"/></form>");

	poststr(request, htmlFooterReturnToCfgOrMainPage);
	http_html_end(request);
	poststr(request, NULL);
	return 0;
}


const char* g_obk_flagNames[] = {
	"[MQTT] Broadcast led params together (send dimmer and color when dimmer or color changes, topic name: YourDevName/led_basecolor_rgb/get, YourDevName/led_dimmer/get)",
	"[MQTT] Broadcast led final color (topic name: YourDevName/led_finalcolor_rgb/get)",
	"[MQTT] Broadcast self state every N (def: 60) seconds (delay configurable by 'mqtt_broadcastInterval' and 'mqtt_broadcastItemsPerSec' commands)",
	"[LED][Debug] Show raw PWM controller on WWW index instead of new LED RGB/CW/etc picker",
	"[LED] Force show RGBCW controller (for example, for SM2135 LEDs, or for DGR sender)",
	"[CMD] Enable TCP console command server (for Putty, etc)",
	"[BTN] Instant touch reaction instead of waiting for release (aka SetOption 13)",
	"[MQTT] [Debug] Always set Retain flag to all published values",
	"[LED] Alternate CW light mode (first PWM for warm/cold slider, second for brightness)",
	"[SM2135] Use separate RGB/CW modes instead of writing all 5 values as RGB",
	"[MQTT] Broadcast self state on MQTT connect",
	"[PWM] BK7231 use 600hz instead of 1khz default",
	"[LED] Remember LED driver state (RGBCW, enable, brightness, temperature) after reboot",
	"[HTTP] Show actual PIN logic level for unconfigured pins",
	"[IR] Do MQTT publish (RAW STRING) for incoming IR data",
	"[IR] Allow 'unknown' protocol",
	"[MQTT] Broadcast led final color RGBCW (topic name: YourDevName/led_finalcolor_rgbcw/get)",
	"[LED] Automatically enable Light when changing brightness, color or temperature on WWW panel",
	"[LED] Smooth transitions for LED (EXPERIMENTAL)",
	"[MQTT] Always publish channels used by TuyaMCU",
	"[LED] Force RGB mode (3 PWMs for LEDs) and ignore further PWMs if they are set",
	"[MQTT] Retain power channels (Relay channels, etc)",
	"[IR] Do MQTT publish (Tasmota JSON format) for incoming IR data",
	"[LED] Automatically enable Light on any change of brightness, color or temperature",
	"[LED] Emulate Cool White with RGB in device with four PWMS - Red is 0, Green 1, Blue 2, and Warm is 4",
	"[POWER] Allow negative current/power for power measurement (all chips, BL0937, BL0942, etc)",
	// On BL602, if marked, uses /dev/ttyS1, otherwise S0
	// On Beken, if marked, uses UART2, otherwise UART1
	"[UART] Use alternate UART for BL0942, CSE, TuyaMCU, etc",
	"[HASS] Invoke HomeAssistant discovery on change to ip address, configuration",
	"[LED] Setting RGB white (FFFFFF) enables temperature mode",
	"[NETIF] Use short device name as a hostname instead of a long name",
	"[MQTT] Enable Tasmota TELE etc publishes (for ioBroker etc)",
	"[UART] Enable UART command line",
	"[LED] Use old linear brightness mode, ignore gamma ramp",
	"[MQTT] Apply channel type multiplier on (if any) on channel value before publishing it",
	"[MQTT] In HA discovery, add relays as lights",
	"[HASS] Deactivate avty_t flag when publishing to HASS (permit to keep value). You must restart HASS discovery for change to take effect.",
	"[DRV] Deactivate Autostart of all drivers",
	"[WiFi] Quick connect to WiFi on reboot (TODO: check if it works for you and report on github)",
	"[Power] Set power and current to zero if all relays are open",
	"[MQTT] [Debug] Publish all channels (don't enable it, it will be publish all 64 possible channels on connect)",
	"[MQTT] Use kWh unit for energy consumption (total, last hour, today) instead of Wh",
	"[BTN] Ignore all button events (aka child lock)",
	"[DoorSensor] Invert state",
	"[TuyaMCU] Use queue",
	"[HTTP] Disable authentication in safe mode (not recommended)",
	"[MQTT Discovery] Don't merge toggles and dimmers into lights",
	"[TuyaMCU] Store raw data",
	"[TuyaMCU] Store ALL data",
	"[PWR] Invert AC dir",
	"[HTTP] Hide ON/OFF for relays (only red/green buttons)",
	"[MQTT] Never add get sufix",
	"error",
	"error",
	"error",
}; 
int http_fn_cfg_generic(http_request_t* request) {
	int i;
	char tmpA[64];
	char tmpB[64];

	http_setup(request, httpMimeTypeHTML);
	http_html_start(request, "Generic config");

	if (http_getArg(request->url, "boot_ok_delay", tmpA, sizeof(tmpA))) {
		i = atoi(tmpA);
		if (i <= 0) {
			poststr(request, "<h5>Boot ok delay must be at least 1 second<h5>");
			i = 1;
		}
		hprintf255(request, "<h5>Setting boot OK delay to %i<h5>", i);
		CFG_SetBootOkSeconds(i);
	}

	if (http_getArg(request->url, "setFlags", tmpA, sizeof(tmpA))) {
		for (i = 0; i < OBK_TOTAL_FLAGS; i++) {
			int ni;
			sprintf(tmpB, "flag%i", i);

			if (http_getArg(request->url, tmpB, tmpA, sizeof(tmpA))) {
				ni = atoi(tmpA);
			}
			else {
				ni = 0;
			}
			//hprintf255(request, "<h5>Setting flag %i to %i<h5>", i, ni);
			CFG_SetFlag(i, ni);
		}
	}

	CFG_Save_IfThereArePendingChanges();

	// 32 bit type
	hprintf255(request, "<h4>Flags (Current value=%i)</h4>", CFG_GetFlags());
	// 64 bit - TODO fixme
	//hprintf255(request, "<h4>Flags (Current value=%lu)</h4>", CFG_GetFlags64());
	poststr(request, "<form action=\"/cfg_generic\">");

	for (i = 0; i < OBK_TOTAL_FLAGS; i++) {
		const char* flagName = g_obk_flagNames[i];
		/*
		<div><input type="checkbox" name="flag0" id="flag0" value="1" checked>
		<label for="flag0">Flag 0 - [MQTT] Broadcast led params together (send dimmer and color when dimmer or color changes, topic name: YourDevName/led_basecolor_rgb/get, YourDevName/led_dimmer/get)</label>
		</div>
		*/
		hprintf255(request, "<div><input type=\"checkbox\" name=\"flag%i\" id=\"flag%i\" value=\"1\"%s>",
			i, i, (CFG_HasFlag(i) ? " checked" : "")); //this is less that 128 char

		hprintf255(request, "<label for=\"flag%i\">Flag %i - ", i, i);
		poststr(request, flagName);
		poststr(request, "</label></div>");
	}
	poststr(request, "<input type=\"hidden\" id=\"setFlags\" name=\"setFlags\" value=\"1\">");
	poststr(request, SUBMIT_AND_END_FORM);

	add_label_numeric_field(request, "Uptime in seconds required to mark boot as OK", "boot_ok_delay",
		CFG_GetBootOkSeconds(), "<form action=\"/cfg_generic\">");
	poststr(request, "<br><input type=\"submit\" value=\"Save\"/></form>");

	poststr(request, htmlFooterReturnToCfgOrMainPage);
	http_html_end(request);
	poststr(request, NULL);
	return 0;
}
int http_fn_cfg_startup(http_request_t* request) {
	int channelIndex;
	int newValue;
	int i;
	char tmpA[128];

	http_setup(request, httpMimeTypeHTML);
	http_html_start(request, "Config startup");
	poststr_h4(request, "Here you can set pin start values");
	poststr(request, "<ul><li>For relays, use 1 or 0</li>");
	poststr(request, "<li>To 'remember last power state', use -1 as a special value</li>");
	poststr(request, "<li>For dimmers, range is 0 to 100</li>");
	poststr(request, "<li>For custom values, you can set any numeric value</li>");
	poststr(request, "<li>Remember that you can also use short <a href='startup_command'>startup command</a> to run commands like led_baseColor #FF0000 and led_enableAll 1 etc</li>");
	hprintf255(request, "<li>To remember last state of LED driver, set ");
	hprintf255(request, "<a href='cfg_generic'>Flag 12 - %s</a>", g_obk_flagNames[12]);
	poststr(request, "</li></ul>");

	if (http_getArg(request->url, "idx", tmpA, sizeof(tmpA))) {
		channelIndex = atoi(tmpA);
		if (http_getArg(request->url, "value", tmpA, sizeof(tmpA))) {
			newValue = atoi(tmpA);


			CFG_SetChannelStartupValue(channelIndex, newValue);
			// also save current value if marked as saved
			Channel_SaveInFlashIfNeeded(channelIndex);
			hprintf255(request, "<h5>Setting channel %i start value to %i<h5>", channelIndex, newValue);

			CFG_Save_IfThereArePendingChanges();
		}
	}

	poststr_h4(request, "New start values");

	for (i = 0; i < CHANNEL_MAX; i++) {
		if (CHANNEL_IsInUse(i)) {
			int startValue = CFG_GetChannelStartupValue(i);

			poststr(request, "<form action='/cfg_startup' class='indent'>");
			hprintf255(request, "<input type=\"hidden\" id=\"idx\" name=\"idx\" value=\"%i\"/>", i);

			sprintf(tmpA, "Channel %i", i);
			add_label_numeric_field(request, tmpA, "value", startValue, "");

			poststr(request, "<input type=\"submit\" value=\"Save\"/></form><br/>");
		}
	}

	poststr(request, htmlFooterReturnToCfgOrMainPage);
	http_html_end(request);
	poststr(request, NULL);
	return 0;
}

int http_fn_cfg_dgr(http_request_t* request) {
	char tmpA[128];
	bool bForceSet;

	http_setup(request, httpMimeTypeHTML);
	http_html_start(request, "Device groups");

	hprintf255(request, "<h5>Here you can configure Tasmota Device Groups<h5>");

	if (http_getArg(request->url, "bSet", tmpA, sizeof(tmpA))) {
		bForceSet = true;
	}
	else {
		bForceSet = false;
	}

	if (http_getArg(request->url, "name", tmpA, sizeof(tmpA)) || bForceSet) {
		int newSendFlags;
		int newRecvFlags;

		newSendFlags = 0;
		newRecvFlags = 0;

		if (http_getArgInteger(request->url, "s_pwr"))
			newSendFlags |= DGR_SHARE_POWER;
		if (http_getArgInteger(request->url, "r_pwr"))
			newRecvFlags |= DGR_SHARE_POWER;
		if (http_getArgInteger(request->url, "s_lbr"))
			newSendFlags |= DGR_SHARE_LIGHT_BRI;
		if (http_getArgInteger(request->url, "r_lbr"))
			newRecvFlags |= DGR_SHARE_LIGHT_BRI;
		if (http_getArgInteger(request->url, "s_lcl"))
			newSendFlags |= DGR_SHARE_LIGHT_COLOR;
		if (http_getArgInteger(request->url, "r_lcl"))
			newRecvFlags |= DGR_SHARE_LIGHT_COLOR;

		CFG_DeviceGroups_SetName(tmpA);
		CFG_DeviceGroups_SetSendFlags(newSendFlags);
		CFG_DeviceGroups_SetRecvFlags(newRecvFlags);

		if (tmpA[0] != 0) {
#ifndef OBK_DISABLE_ALL_DRIVERS
			DRV_StartDriver("DGR");
#endif
		}
		CFG_Save_IfThereArePendingChanges();
	}
	{
		int newSendFlags;
		int newRecvFlags;
		const char* groupName = CFG_DeviceGroups_GetName();


		newSendFlags = CFG_DeviceGroups_GetSendFlags();
		newRecvFlags = CFG_DeviceGroups_GetRecvFlags();

		add_label_text_field(request, "Group name", "name", groupName, "<form action=\"/cfg_dgr\">");
		poststr(request, "<br><table><tr><th>Name</th><th>Tasmota Code</th><th>Receive</th><th>Send</th></tr><tr><td>Power</td><td>1</td>");

		poststr(request, "<td><input type=\"checkbox\" name=\"r_pwr\" value=\"1\"");
		if (newRecvFlags & DGR_SHARE_POWER)
			poststr(request, " checked");
		poststr(request, "></td><td><input type=\"checkbox\" name=\"s_pwr\" value=\"1\"");
		if (newSendFlags & DGR_SHARE_POWER)
			poststr(request, " checked");
		poststr(request, "></td> ");

		poststr(request, "</tr><tr><td>Light Brightness</td><td>2</td>");

		poststr(request, "<td><input type=\"checkbox\" name=\"r_lbr\" value=\"1\"");
		if (newRecvFlags & DGR_SHARE_LIGHT_BRI)
			poststr(request, " checked");
		poststr(request, "></td><td><input type=\"checkbox\" name=\"s_lbr\" value=\"1\"");
		if (newSendFlags & DGR_SHARE_LIGHT_BRI)
			poststr(request, " checked");
		poststr(request, "></td> ");

		poststr(request, "</tr><tr><td>Light Color</td><td>16</td>");
		poststr(request, "<td><input type=\"checkbox\" name=\"r_lcl\" value=\"1\"");
		if (newRecvFlags & DGR_SHARE_LIGHT_COLOR)
			poststr(request, " checked");
		poststr(request, "></td><td><input type=\"checkbox\" name=\"s_lcl\" value=\"1\"");
		if (newSendFlags & DGR_SHARE_LIGHT_COLOR)
			poststr(request, " checked");
		poststr(request, "></td> ");

		poststr(request, "<input type=\"hidden\" name=\"bSet\" value=\"1\">");

		poststr(request, "</tr></table>");
		poststr(request, SUBMIT_AND_END_FORM);
	}

	poststr(request, htmlFooterReturnToCfgOrMainPage);
	http_html_end(request);
	poststr(request, NULL);
	return 0;
}

void XR809_RequestOTAHTTP(const char* s);

void OTA_RequestDownloadFromHTTP(const char* s) {
#if WINDOWS

#elif PLATFORM_BL602

#elif PLATFORM_LN882H

#elif PLATFORM_ESPIDF
#elif PLATFORM_W600 || PLATFORM_W800
	t_http_fwup(s);
#elif PLATFORM_XR809
	XR809_RequestOTAHTTP(s);
#else
	otarequest(s);
#endif
}
int http_fn_ota_exec(http_request_t* request) {
	char tmpA[128];
	//char tmpB[64];

	http_setup(request, httpMimeTypeHTML);
	http_html_start(request, "OTA request");
	if (http_getArg(request->url, "host", tmpA, sizeof(tmpA))) {
		hprintf255(request, "<h3>OTA requested for %s!</h3>", tmpA);
		addLogAdv(LOG_INFO, LOG_FEATURE_HTTP, "http_fn_ota_exec: will try to do OTA for %s \r\n", tmpA);
		OTA_RequestDownloadFromHTTP(tmpA);
	}
	poststr(request, htmlFooterReturnToCfgOrMainPage);
	http_html_end(request);
	poststr(request, NULL);
	return 0;
}

int http_fn_ota(http_request_t* request) {
	http_setup(request, httpMimeTypeHTML);
	http_html_start(request, "OTA system");
	poststr(request, "<p>For a more user-friendly experience, it is recommended to use the OTA option in the Web Application, where you can easily drag and drop files without needing to set up a server. On Beken platforms, the .rbl file is used for OTA updates. In the OTA section below, paste the link to the .rbl file (an HTTP server is required).</p>");
	add_label_text_field(request, "URL for new bin file", "host", "", "<form action=\"/ota_exec\">");
	poststr(request, "<br>\
<input type=\"submit\" value=\"Submit\" onclick=\"return confirm('Are you sure?')\">\
</form>");
	poststr(request, htmlFooterReturnToCfgOrMainPage);
	http_html_end(request);
	poststr(request, NULL);
	return 0;
}

int http_fn_other(http_request_t* request) {
	http_setup(request, httpMimeTypeHTML);
	http_html_start(request, "Not found");
	poststr(request, "Not found.<br/>");
	poststr(request, htmlFooterReturnToMainPage);
	http_html_end(request);
	poststr(request, NULL);
	return 0;
}
