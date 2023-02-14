#ifndef __NEW_PINS_H__
#define __NEW_PINS_H__
#include "new_common.h"


enum IORole {
	IOR_None,
	IOR_Relay,
	IOR_Relay_n,
	IOR_Button,
	IOR_Button_n,
	IOR_LED,
	IOR_LED_n,
	IOR_PWM,
	IOR_LED_WIFI,
	IOR_LED_WIFI_n,
	IOR_Button_ToggleAll,
	IOR_Button_ToggleAll_n,
	// this copies directly value on pin (0 or 1) to channel
	IOR_DigitalInput,
	// as above, but inverted
	IOR_DigitalInput_n,
	IOR_ToggleChannelOnToggle,
	// this copies directly value on pin (0 or 1) to channel
	IOR_DigitalInput_NoPup,
	// as above, but inverted
	IOR_DigitalInput_NoPup_n,
	// energy sensor
	IOR_BL0937_SEL,
	IOR_BL0937_CF,
	IOR_BL0937_CF1,
	IOR_ADC,
	IOR_SM2135_DAT,
	IOR_SM2135_CLK,

	IOR_BP5758D_DAT,
	IOR_BP5758D_CLK,

	IOR_BP1658CJ_DAT,
	IOR_BP1658CJ_CLK,

	IOR_PWM_n,

	IOR_IRRecv,
	IOR_IRSend,

	// I have a LED strip that has separate power button and a 'next mode' button
	// It's a RGB strip
	IOR_Button_NextColor,
	IOR_Button_NextColor_n,

	IOR_Button_NextDimmer,
	IOR_Button_NextDimmer_n,

	IOR_AlwaysHigh,
	IOR_AlwaysLow,

	IOR_UCS1912_DIN,
	IOR_SM16703P_DIN,

	IOR_Button_NextTemperature,
	IOR_Button_NextTemperature_n,

	IOR_Button_ScriptOnly,
	IOR_Button_ScriptOnly_n,

	IOR_DHT11,
	IOR_DHT12,
	IOR_DHT21,
	IOR_DHT22,

	IOR_CHT8305_DAT,
	IOR_CHT8305_CLK,

	IOR_SHT3X_DAT,
	IOR_SHT3X_CLK,

	IOR_SOFT_SDA,
	IOR_SOFT_SCL,

	IOR_SM2235_DAT,
	IOR_SM2235_CLK,

    IOR_BridgeForward,
    IOR_BridgeReverse,

	IOR_Total_Options,
};

#define IS_PIN_DHT_ROLE(role) (((role)>=IOR_DHT11) && ((role)<=IOR_DHT22))
#define IS_PIN_TEMP_HUM_SENSOR_ROLE(role) (((role)==IOR_SHT3X_DAT) || ((role)==IOR_CHT8305_DAT))

typedef enum {
	ChType_Default,
	ChType_Error,
	ChType_Temperature,
	ChType_Humidity,
	// most likely will not be used
	// but it's humidity value times 10
	// TuyaMCU sends 225 instead of 22.5%
	ChType_Humidity_div10,
	ChType_Temperature_div10,
	ChType_Toggle,
	// 0-100 range
	ChType_Dimmer,
	ChType_LowMidHigh,
	ChType_TextField,
	ChType_ReadOnly,
	// off (0) and 3 speeds
	ChType_OffLowMidHigh,
	// off (0) and 5 speeds
	ChType_OffLowestLowMidHighHighest,
	// only 5 speeds
	ChType_LowestLowMidHighHighest,
	// like dimmer, but 0-255
	ChType_Dimmer256,
	// like dimmer, but 0-1000
	ChType_Dimmer1000,
	// for TuyaMCU power metering
	//NOTE: not used for BL0937 etc
	ChType_Frequency_div100,
	ChType_Voltage_div10,
	ChType_Power,
	ChType_Current_div100,
	ChType_ActivePower,
	ChType_PowerFactor_div1000,
	ChType_ReactivePower,
	ChType_EnergyTotal_kWh_div1000,
	ChType_EnergyExport_kWh_div1000,
	ChType_EnergyToday_kWh_div1000,
	ChType_Current_div1000,
	ChType_EnergyTotal_kWh_div100,
	ChType_OpenClosed,
	ChType_OpenClosed_Inv,
	ChType_BatteryLevelPercent,
	ChType_OffDimBright,

} ChannelType;


#if PLATFORM_BL602
#define PLATFORM_GPIO_MAX 24
#elif PLATFORM_XR809
#define PLATFORM_GPIO_MAX 13
#elif PLATFORM_W600
#define PLATFORM_GPIO_MAX 16
#elif PLATFORM_W800
#define PLATFORM_GPIO_MAX 17
#else
#define PLATFORM_GPIO_MAX 29
#endif

#define CHANNEL_MAX 64

#define SPECIAL_CHANNEL_BRIGHTNESS 129
#define SPECIAL_CHANNEL_LEDPOWER 130
#define SPECIAL_CHANNEL_BASECOLOR 131
#define SPECIAL_CHANNEL_TEMPERATURE 132

typedef struct pinsState_s {
	// All above values are indexed by physical pin index
	// (so we assume we have maximum of 32 pins)
	byte roles[32];
	byte channels[32];
	// extra channels array - this is needed for
	// buttons, so button can toggle one relay on single click
	// and other relay on double click
	byte channels2[32];
	// This single field above, is indexed by CHANNEL INDEX
	// (not by pin index)
	byte channelTypes[CHANNEL_MAX];
} pinsState_t;


// bit indexes (not values), so 0 1 2 3 4
#define OBK_FLAG_MQTT_BROADCASTLEDPARAMSTOGETHER	0
#define OBK_FLAG_MQTT_BROADCASTLEDFINALCOLOR		1
#define OBK_FLAG_MQTT_BROADCASTSELFSTATEPERMINUTE	2
#define OBK_FLAG_LED_RAWCHANNELSMODE				3
#define OBK_FLAG_LED_FORCESHOWRGBCWCONTROLLER		4
#define OBK_FLAG_CMD_ENABLETCPRAWPUTTYSERVER		5
#define OBK_FLAG_BTN_INSTANTTOUCH					6
#define OBK_FLAG_MQTT_ALWAYSSETRETAIN				7
#define OBK_FLAG_LED_ALTERNATE_CW_MODE				8
#define OBK_FLAG_SM2135_SEPARATE_MODES				9
#define OBK_FLAG_MQTT_BROADCASTSELFSTATEONCONNECT	10
#define OBK_FLAG_SLOW_PWM							11
#define OBK_FLAG_LED_REMEMBERLASTSTATE				12
#define OBK_FLAG_HTTP_PINMONITOR					13
#define OBK_FLAG_IR_PUBLISH_RECEIVED				14
#define OBK_FLAG_IR_ALLOW_UNKNOWN					15
#define OBK_FLAG_LED_BROADCAST_FULL_RGBCW			16
#define OBK_FLAG_LED_AUTOENABLE_ON_WWW_ACTION		17
#define OBK_FLAG_LED_SMOOTH_TRANSITIONS				18
#define OBK_FLAG_TUYAMCU_ALWAYSPUBLISHCHANNELS		19
#define OBK_FLAG_LED_FORCE_MODE_RGB					20
#define OBK_FLAG_MQTT_RETAIN_POWER_CHANNELS			21
#define OBK_FLAG_IR_PUBLISH_RECEIVED_IN_JSON		22
#define OBK_FLAG_LED_AUTOENABLE_ON_ANY_ACTION		23
#define OBK_FLAG_LED_EMULATE_COOL_WITH_RGB			24
#define OBK_FLAG_POWER_ALLOW_NEGATIVE				25
#define OBK_FLAG_USE_SECONDARY_UART					26
#define OBK_FLAG_AUTOMAIC_HASS_DISCOVERY			27
#define OBK_FLAG_LED_SETTING_WHITE_RGB_ENABLES_CW	28
#define OBK_FLAG_USE_SHORT_DEVICE_NAME_AS_HOSTNAME	29
#define OBK_FLAG_DO_TASMOTA_TELE_PUBLISHES			30
#define OBK_FLAG_CMD_ACCEPT_UART_COMMANDS			31
#define OBK_FLAG_LED_USE_OLD_LINEAR_MODE			32
#define OBK_FLAG_PUBLISH_MULTIPLIED_VALUES			33

#define OBK_TOTAL_FLAGS 34


#define CGF_MQTT_CLIENT_ID_SIZE			64
#define CGF_SHORT_DEVICE_NAME_SIZE		32
#define CGF_DEVICE_NAME_SIZE			64

typedef union cfgPowerMeasurementCal_u {
	float f;
	int i;
} cfgPowerMeasurementCal_t;

// 8 * 4 = 32 bytes
typedef struct cfgPowerMeasurementCalibration_s {
	cfgPowerMeasurementCal_t values[8];
} cfgPowerMeasurementCalibration_t;

// unit is 0.1s
#define CFG_DEFAULT_BTN_SHORT	3
#define CFG_DEFAULT_BTN_LONG	10
#define CFG_DEFAULT_BTN_REPEAT	5

enum {
	CFG_OBK_VOLTAGE = 0,
	CFG_OBK_CURRENT,
	CFG_OBK_POWER,
	CFG_OBK_POWER_MAX
};

typedef struct led_corr_s { // LED gamma correction and calibration data block
	float rgb_cal[3];     // RGB correction factors, range 0.0-1.0, 1.0 = no correction
	float led_gamma;      // LED gamma value, range 1.0-3.0
	float rgb_bright_min; // RGB minimum brightness, range 0.0-10.0%
	float cw_bright_min;  // CW minimum brightness, range 0.0-10.0%
} led_corr_t;

#define MAGIC_LED_CORR_SIZE		24

typedef struct ledRemap_s {
	//unsigned short r : 3;
	//unsigned short g : 3;
	//unsigned short b : 3;
	//unsigned short c : 3;
	//unsigned short w : 3;
	// I want to be able to easily access indices with []
	union {
		struct {
			byte r;
			byte g;
			byte b;
			byte c;
			byte w;
		};
		byte ar[5];
	};
} ledRemap_t;

#define MAGIC_LED_REMAP_SIZE 5

//
// Main config structure (less than 2KB)
//
// This config structure is supposed  to be saved only when user
// changes the device configuration, so really not often.
// We should not worry about flash memory wear in this case.
// The saved-every-reboot values are stored elsewhere
// (i.e. saved channel states, reboot counter?)
typedef struct mainConfig_s {
	byte ident0;
	byte ident1;
	byte ident2;
	byte crc;
	// 0x4
	int version;
	int genericFlags;
	int genericFlags2;
	unsigned short changeCounter;
	unsigned short otaCounter;
	// target wifi credentials
	char wifi_ssid[64];
	char wifi_pass[64];
	// MQTT information for Home Assistant
	char mqtt_host[256];
	// note: #define CGF_MQTT_CLIENT_ID_SIZE			64
	char mqtt_clientId[CGF_MQTT_CLIENT_ID_SIZE];
	char mqtt_userName[64];
	char mqtt_pass[128];
	//mqtt_port at offs 0x00000294 
	int mqtt_port;
	// addon JavaScript panel is hosted on external server
	char webappRoot[64];
	// TODO?
	byte mac[6];
	// NOTE: NO LONGER 4 byte aligned!!!
	// TODO?
	// #define CGF_SHORT_DEVICE_NAME_SIZE		32
	char shortDeviceName[CGF_SHORT_DEVICE_NAME_SIZE];
	// #define CGF_DEVICE_NAME_SIZE			64
	char longDeviceName[CGF_DEVICE_NAME_SIZE];

	//pins at offs 0x0000033E
	// 160 bytes
	pinsState_t pins;
	// startChannelValues at offs 0x000003DE
	// 64 * 2
	short startChannelValues[CHANNEL_MAX];
	// dgr_sendFlags at offs 0x0000045E 
	short unused_fill; // correct alignment
	int dgr_sendFlags;
	int dgr_recvFlags;
	char dgr_name[16];
	char ntpServer[32];
	// 8 * 4 = 32 bytes
	cfgPowerMeasurementCalibration_t cal;
	// offs 0x000004B8
	// short press 1 means 100 ms short press time
	// So basically unit is 0.1 second
	byte buttonShortPress;
	byte buttonLongPress;
	byte buttonHoldRepeat;
	byte unused_fill1;

	// offset 0x000004BC
	unsigned long LFS_Size; // szie of LFS volume.  it's aligned against the end of OTA
	byte unusedSectorAB[119];
	ledRemap_t ledRemap;
	led_corr_t led_corr;
	// alternate topic name for receiving MQTT commands
	// offset 0x00000554
	char mqtt_group[64];
	// offs 0x00000594
	byte unused_bytefill[3];
	byte timeRequiredToMarkBootSuccessfull;
	//offs 0x00000598
	int ping_interval;
	int ping_seconds;
	// 0x000005A0
	char ping_host[64];
	char initCommandLine[512];
} mainConfig_t; // size 0x000007E0 (2016 decimal)
#define MAGIC_CONFIG_SIZE		2016
extern mainConfig_t g_cfg;

extern char g_enable_pins;

#define CHANNEL_SET_FLAG_FORCE		1
#define CHANNEL_SET_FLAG_SKIP_MQTT	2
#define CHANNEL_SET_FLAG_SILENT		4

void PIN_ticks(void *param);

void PIN_set_wifi_led(int value);
void PIN_AddCommands(void);
void PINS_BeginDeepSleepWithPinWakeUp();
void PIN_SetupPins();
void PIN_OnReboot();
void CFG_ClearPins();
int PIN_CountPinsWithRole(int role);
int PIN_CountPinsWithRoleOrRole(int role, int role2);
int PIN_GetPinRoleForPinIndex(int index);
int PIN_GetPinChannelForPinIndex(int index);
int PIN_GetPinChannel2ForPinIndex(int index);
int PIN_FindPinIndexForRole(int role, int defaultIndexToReturnIfNotFound);
const char* PIN_GetPinNameAlias(int index);
void PIN_SetPinRoleForPinIndex(int index, int role);
void PIN_SetPinChannelForPinIndex(int index, int ch);
void PIN_SetPinChannel2ForPinIndex(int index, int ch);
void CHANNEL_Toggle(int ch);
void CHANNEL_DoSpecialToggleAll();
bool CHANNEL_Check(int ch);
void PIN_SetGenericDoubleClickCallback(void (*cb)(int pinIndex));
void CHANNEL_ClearAllChannels();
// CHANNEL_SET_FLAG_*
void CHANNEL_Set(int ch, int iVal, int iFlags);
void CHANNEL_Set_FloatPWM(int ch, float fVal, int iFlags);
void CHANNEL_Add(int ch, int iVal);
void CHANNEL_AddClamped(int ch, int iVal, int min, int max, int bWrapInsteadOfClamp);
int CHANNEL_Get(int ch);
float CHANNEL_GetFloat(int ch);
int CHANNEL_GetRoleForOutputChannel(int ch);
bool CHANNEL_HasRoleThatShouldBePublished(int ch);
bool CHANNEL_IsPowerRelayChannel(int ch);
// See: enum ChannelType
void CHANNEL_SetType(int ch, int type);
int CHANNEL_GetType(int ch);
void CHANNEL_SetAllChannelsByType(int requiredType, int newVal);
// CHANNEL_SET_FLAG_*
void CHANNEL_SetAll(int iVal, int iFlags);
void CHANNEL_SetStateOnly(int iVal);
int CHANNEL_HasChannelPinWithRole(int ch, int iorType);
int CHANNEL_HasChannelPinWithRoleOrRole(int ch, int iorType, int iorType2);
bool CHANNEL_IsInUse(int ch);
void Channel_SaveInFlashIfNeeded(int ch);
int CHANNEL_FindMaxValueForChannel(int ch);
// cmd_channels.c
const char *CHANNEL_GetLabel(int ch);
//ledRemap_t *CFG_GetLEDRemap();

void get_Relay_PWM_Count(int* relayCount, int* pwmCount, int* dInputCount);
int h_isChannelPWM(int tg_ch);
int h_isChannelRelay(int tg_ch);
int h_isChannelDigitalInput(int tg_ch);


//int PIN_GetPWMIndexForPinIndex(int pin);

int PIN_ParsePinRoleName(const char* name);
const char *PIN_RoleToString(int role);

// from new_builtin.c
/*

WARNING! THIS IS OBSOLETE NOW!

WE ARE USING THIS DATABASE:
https://github.com/OpenBekenIOT/webapp/blob/gh-pages/devices.json
Submit pull requests to the list above! Post teardowns on Elektroda.com!


HERE IS FRONTEND:
https://openbekeniot.github.io/webapp/devicesList.html
See above link for more info!

*/
//void Setup_Device_Empty();
//void Setup_Device_TuyaWL_SW01_16A();
//void Setup_Device_TuyaSmartLife4CH10A();
//void Setup_Device_BK7231N_TuyaLightBulb_RGBCW_5PWMs();
//void Setup_Device_IntelligentLife_NF101A();
//void Setup_Device_TuyaLEDDimmerSingleChannel();
//void Setup_Device_CalexLEDDimmerFiveChannel();
//void Setup_Device_CalexPowerStrip_900018_1v1_0UK();
//void Setup_Device_ArlecCCTDownlight();
//void Setup_Device_ArlecRGBCCTDownlight();
//void Setup_Device_CasaLifeCCTDownlight();
//void Setup_Device_NedisWIFIPO120FWT_16A();
//void Setup_Device_NedisWIFIP130FWT_10A();
//void Setup_Device_EmaxHome_EDU8774();
//void Setup_Device_TuyaSmartPFW02G();
//void Setup_Device_BK7231N_CB2S_QiachipSmartSwitch();
//void Setup_Device_BK7231T_WB2S_QiachipSmartSwitch();
//void Setup_Device_BK7231T_Raw_PrimeWiFiSmartOutletsOutdoor_CCWFIO232PK();
//void Setup_Device_AvatarASL04();
//void Setup_Device_TuyaSmartWIFISwith_4Gang_CB3S();
//void Setup_Device_BL602_MagicHome_IR_RGB_LedStrip();
//void Setup_Device_BL602_MagicHome_CCT_LedStrip();
//void Setup_Device_Sonoff_MiniR3();
//void Setup_Device_WiFi_DIY_Switch_WB2S_ZN268131();
//void Setup_Device_BK7231N_CB2S_LSPA9_BL0942();
//void Setup_Device_LSC_Smart_Connect_Plug_CB2S();
//void Setup_Device_DS_102_1Gang_WB3S();
//void Setup_Device_DS_102_2Gang_WB3S();
//void Setup_Device_DS_102_3Gang_WB3S();
//void Setup_Device_BK7231T_Gosund_Switch_SW5_A_V2_1();
//void Setup_Device_13A_Socket_CB2S();
//void Setup_Device_Deta_Smart_Double_Power_Point_6922HA_Series2();
//void Setup_Device_BK7231N_KS_602_TOUCH();
//void Setup_Device_Enbrighten_WFD4103();
//void Setup_Device_Aubess_Mini_Smart_Switch_16A();
//void Setup_Device_Zemismart_Light_Switch_KS_811_3();
//void Setup_Device_TeslaSmartPlus_TSL_SPL_1();
//void Setup_Device_Calex_900011_1_WB2S();
//void Setup_Device_Immax_NEO_LITE_NAS_WR07W();
//void Setup_Device_MOES_TouchSwitch_WS_EU1_RFW_N();


#endif
