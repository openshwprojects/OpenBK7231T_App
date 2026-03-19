//////////////////////////////////////////////////////
// specify which parts of the app we wish to be active
//
#ifndef OBK_CONFIG_H
#define OBK_CONFIG_H

#define OBK_VARIANT_DEFAULT						0
#define OBK_VARIANT_BERRY						1
#define OBK_VARIANT_TUYAMCU						2
#define OBK_VARIANT_POWERMETERING				3
#define OBK_VARIANT_IRREMOTEESP					4
#define OBK_VARIANT_SENSORS						5
#define OBK_VARIANT_HLW8112						6
#define OBK_VARIANT_BATTERY						7
#define OBK_VARIANT_BTPROXY						8
#define OBK_VARIANT_ESP2M						1
#define OBK_VARIANT_ESP4M						2
#define OBK_VARIANT_ESP2M_BERRY					3

// Starts with all driver flags undefined

// NOTE:
// Defines for HTTP/HTMP (UI) pages: ENABLE_HTTP_*
// Defines for drivers from drv_main.c: ENABLE_DRIVER_*
// Other defines: ENABLE_* , for example: ENABLE_LED_BASIC

#define ENABLE_HTTP_MQTT						1
#define ENABLE_HTTP_IP							1
#define ENABLE_HTTP_WEBAPP						1
#define ENABLE_HTTP_NAMES						1
#define ENABLE_HTTP_MAC							1
#define ENABLE_HTTP_FLAGS						1
#define ENABLE_HTTP_STARTUP						1
#define ENABLE_HTTP_PING						1
#define ENABLE_LED_BASIC						1



#elif WINDOWS

#if LINUX

#else

#define ENABLE_SDL_WINDOW						1

#endif

#define ENABLE_DRIVER_TCA9554					1
#define ENABLE_DRIVER_PINMUTEX					1
#define ENABLE_DRIVER_TESTSPIFLASH				1

#define ENABLE_DRIVER_GIRIERMCU					1

#define ENABLE_HTTP_OVERRIDE					1
#define ENABLE_DRIVER_TCL						1
#define ENABLE_DRIVER_PIR						1
#define ENABLE_HA_DISCOVERY						1
#define ENABLE_SEND_POSTANDGET					1
#define ENABLE_MQTT								1
#define ENABLE_TASMOTADEVICEGROUPS				1
#define ENABLE_LITTLEFS							1
#define ENABLE_NTP								1
#define ENABLE_TIME_DST							1
#define ENABLE_DRIVER_LED						1
#define ENABLE_DRIVER_BL0937					1
#define ENABLE_DRIVER_BL0942					1
#define ENABLE_DRIVER_BL0942SPI					1
#define ENABLE_DRIVER_CSE7766					1
#define ENABLE_DRIVER_CSE7761					1
#define ENABLE_DRIVER_TESTPOWER					1
#define ENABLE_DRIVER_HT16K33					1
#define ENABLE_DRIVER_MAX72XX					1
#define ENABLE_DRIVER_TUYAMCU					1
#define ENABLE_TEST_COMMANDS					1
#define ENABLE_CALENDAR_EVENTS					1
#define ENABLE_TEST_DRIVERS						1
#define ENABLE_DRIVER_BRIDGE					1
#define ENABLE_DRIVER_HTTPBUTTONS				1
#define ENABLE_ADVANCED_CHANNELTYPES_DISCOVERY	1
#define ENABLE_DRIVER_WEMO						1
#define ENABLE_DRIVER_HUE						1
#define ENABLE_DRIVER_CHARGINGLIMIT				1
#define ENABLE_DRIVER_BATTERY					1
#define ENABLE_DRIVER_BKPARTITIONS				1
#define ENABLE_DRIVER_PT6523					1
#define ENABLE_DRIVER_MAX6675					1
#define ENABLE_DRIVER_TEXTSCROLLER				1
#define ENABLE_TIME_SUNRISE_SUNSET				1
// parse things like $CH1 or $hour etc
#define ENABLE_EXPAND_CONSTANT					1
#define ENABLE_DRIVER_DHT						1
#define ENABLE_DRIVER_SM16703P					1
#define ENABLE_DRIVER_PIXELANIM					1
#define ENABLE_DRIVER_TMGN						1
#define ENABLE_DRIVER_DRAWERS					1
#define ENABLE_TASMOTA_JSON						1
#define ENABLE_DRIVER_DDP						1
#define ENABLE_DRIVER_DDPSEND					1
#define ENABLE_DRIVER_SSDP						1
#define ENABLE_DRIVER_ADCBUTTON					1
#define ENABLE_DRIVER_SM15155E					1
//#define ENABLE_DRIVER_IR						1
//#define ENABLE_DRIVER_IR2					1
#define ENABLE_DRIVER_CHARTS					1
#define ENABLE_DRIVER_WIDGET					1
#define ENABLE_DRIVER_OPENWEATHERMAP			1
#define ENABLE_DRIVER_MCP9808					1
#define ENABLE_DRIVER_KP18058					1
#define ENABLE_DRIVER_ADCSMOOTHER				1
#define ENABLE_DRIVER_SGP						1
#define ENABLE_DRIVER_SHIFTREGISTER				1
#define ENABLE_OBK_SCRIPTING					1
#define ENABLE_OBK_BERRY						1
#define ENABLE_DRIVER_DS1820_FULL				1
#define ENABLE_DRIVER_DMX						1
#define ENABLE_DRIVER_MQTTSERVER				1
//#define ENABLE_DRIVER_ARISTON					1

#elif PLATFORM_BEKEN

// ===== CORE =====
#define ENABLE_MQTT 1
#define ENABLE_LITTLEFS 1
#define ENABLE_NTP 1
#define ENABLE_OBK_SCRIPTING 1

// ===== NETWORK =====
#define ENABLE_HA_DISCOVERY 1
#define ENABLE_SEND_POSTANDGET 1
#define ENABLE_DRIVER_SSDP 1

// ===== BASIC IO =====
#define ENABLE_DRIVER_LED 1

// ===== IR BLASTER =====
#define ENABLE_DRIVER_IR 1
#define ENABLE_DRIVER_TINYIR_NEC 1

// ===== OPTIONAL =====
#define ENABLE_DRIVER_MAX72XX 1   // only if using MAX7219

// ===== OPTIONAL I2C =====
#define ENABLE_I2C 1

#if PLATFORM_BK7231N || PLATFORM_BK7231T
#define ENABLE_DRIVER_MDNS 1
#endif

#if PLATFORM_BEKEN_NEW
#define NEW_TCP_SERVER 1
#endif

#endif


#if ENABLE_TASMOTADEVICEGROUPS
#define ENABLE_HTTP_DGR 1
#endif

// ===== KEEP IR SAFE =====
// #if ENABLE_DRIVER_IRREMOTEESP
// #undef ENABLE_DRIVER_IR
// #endif

#endif
