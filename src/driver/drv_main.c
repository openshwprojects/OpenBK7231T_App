#include "../i2c/drv_i2c_public.h"
#include "../logging/logging.h"
#include "drv_bl0937.h"
#include "drv_bl0942.h"
#include "drv_bl_shared.h"
#include "drv_neo6m.h"
#include "drv_cse7766.h"
#include "drv_ir.h"
#include "drv_rc.h"
#include "drv_local.h"
#include "drv_ntp.h"
#include "drv_deviceclock.h"
#include "drv_public.h"
#include "drv_ssdp.h"
#include "drv_test_drivers.h"
#include "drv_tuyaMCU.h"
#include "drv_girierMCU.h"
#include "drv_uart.h"
#include "drv_ds1820_simple.h"
#include "drv_ds1820_full.h"
#include "drv_ds1820_common.h"
#include "drv_ds3231.h"
#include "drv_hlw8112.h"


typedef struct driver_s {
	const char* name;
	void(*initFunc)();
	void(*onEverySecond)();
	void(*appendInformationToHTTPIndexPage)(http_request_t* request, int bPreState);
	void(*runQuickTick)();
	void(*stopFunc)();
	void(*onChannelChanged)(int ch, int val);
	void(*onHassDiscovery)(const char *topic);
	bool bLoaded;
} driver_t;


void TuyaMCU_RunEverySecond();
void GirierMCU_RunEverySecond();

// startDriver BL0937
static driver_t g_drivers[] = {
#if ENABLE_DRIVER_TUYAMCU
	//drvdetail:{"name":"TuyaMCU",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"TuyaMCU is a protocol used for communication between WiFI module and external MCU. This protocol is using usually RX1/TX1 port of BK chips. See [TuyaMCU dimmer example](https://www.elektroda.com/rtvforum/topic3929151.html), see [TH06 LCD humidity/temperature sensor example](https://www.elektroda.com/rtvforum/topic3942730.html), see [fan controller example](https://www.elektroda.com/rtvforum/topic3908093.html), see [simple switch example](https://www.elektroda.com/rtvforum/topic3906443.html)",
	//drvdetail:"requires":""}
	{ "TuyaMCU",                             // Driver Name
	TuyaMCU_Init,                            // Init
	TuyaMCU_RunEverySecond,                  // onEverySecond
	NULL,                                    // appendInformationToHTTPIndexPage
	TuyaMCU_RunFrame,                        // runQuickTick
	TuyaMCU_Shutdown,                        // stopFunction
	NULL,                                    // onChannelChanged
	NULL,                                    // onHassDiscovery
	false,                                   // loaded
	},
	//drvdetail:{"name":"tmSensor",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"The tmSensor must be used only when TuyaMCU is already started. tmSensor is a TuyaMcu Sensor, it's used for Low Power TuyaMCU communication on devices like TuyaMCU door sensor, or TuyaMCU humidity sensor. After device reboots, tmSensor uses TuyaMCU to request data update from the sensor and reports it on MQTT. Then MCU turns off WiFi module again and goes back to sleep. See an [example door sensor here](https://www.elektroda.com/rtvforum/topic3914412.html).",
	//drvdetail:"requires":""}
	{ "tmSensor",                            // Driver Name
	TuyaMCU_Sensor_Init,                     // Init
	TuyaMCU_Sensor_RunEverySecond,           // onEverySecond
	NULL,                                    // appendInformationToHTTPIndexPage
	NULL,                                    // runQuickTick
	NULL,                                    // stopFunction
	NULL,                                    // onChannelChanged
	NULL,                                    // onHassDiscovery
	false,                                   // loaded
	},
#endif
#ifdef ENABLE_DRIVER_GIRIERMCU
	//drvdetail:{"name":"GirierMCU",
	//drvdetail:"title":"GirierMCU",
	//drvdetail:"descr":"TODO",
	//drvdetail:"requires":""}
	{ "GirierMCU",                           // Driver Name
	GirierMCU_Init,                          // Init
	GirierMCU_RunEverySecond,                // onEverySecond
	NULL,                                    // appendInformationToHTTPIndexPage
	GirierMCU_RunFrame,                      // runQuickTick
	GirierMCU_Shutdown,                      // stopFunction
	NULL,                                    // onChannelChanged
	NULL,                                    // onHassDiscovery
	false,                                   // loaded
	},
#endif
#if ENABLE_DRIVER_TCA9554
	//drvdetail:{"name":"TCA9554",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"TCA9554.",
	//drvdetail:"requires":""}
	{ "TCA9554",                             // Driver Name
	TCA9554_Init,                            // Init
	TCA9554_OnEverySecond,                   // onEverySecond
	NULL,                                    // appendInformationToHTTPIndexPage
	NULL,                                    // runQuickTick
	NULL,                                    // stopFunction
	TCA9554_OnChannelChanged,                // onChannelChanged
	NULL,                                    // onHassDiscovery
	false,                                   // loaded
	},
#endif
#if ENABLE_DRIVER_DMX
	//drvdetail:{"name":"DMX",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"DMX.",
	//drvdetail:"requires":""}
	{ "DMX",                                 // Driver Name
	DMX_Init,                                // Init
	DMX_OnEverySecond,                       // onEverySecond
	NULL,                                    // appendInformationToHTTPIndexPage
	NULL,                                    // runQuickTick
	DMX_Shutdown,                            // stopFunction
	NULL,                                    // onChannelChanged
	NULL,                                    // onHassDiscovery
	false,                                   // loaded
	},
#endif
#if ENABLE_DRIVER_FREEZE
	//drvdetail:{"name":"Freeze",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"Freeze is a test driver for watchdog. Enabling this will freeze device main loop.",
	//drvdetail:"requires":""}
	{ "Freeze",                              // Driver Name
	Freeze_Init,                             // Init
	Freeze_OnEverySecond,                    // onEverySecond
	NULL,                                    // appendInformationToHTTPIndexPage
	Freeze_RunFrame,                         // runQuickTick
	NULL,                                    // stopFunction
	NULL,                                    // onChannelChanged
	NULL,                                    // onHassDiscovery
	false,                                   // loaded
	},
#endif
#if ENABLE_DRIVER_TESTSPIFLASH
	//drvdetail:{"name":"TESTSPIFLASH",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"TESTSPIFLASH",
	//drvdetail:"requires":""}
	{ "TESTSPIFLASH",                        // Driver Name
	DRV_InitFlashMemoryTestFunctions,        // Init
	NULL,                                    // onEverySecond
	NULL,                                    // appendInformationToHTTPIndexPage
	NULL,                                    // runQuickTick
	NULL,                                    // stopFunction
	NULL,                                    // onChannelChanged
	NULL,                                    // onHassDiscovery
	false,                                   // loaded
	},
#endif
#if ENABLE_DRIVER_PIR
	//drvdetail:{"name":"PIR",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"PIR",
	//drvdetail:"requires":""}
	{ "PIR",                                 // Driver Name
	PIR_Init,                                // Init
	PIR_OnEverySecond,                       // onEverySecond
	PIR_AppendInformationToHTTPIndexPage,    // appendInformationToHTTPIndexPage
	NULL,                                    // runQuickTick
	NULL,                                    // stopFunction
	PIR_OnChannelChanged,                    // onChannelChanged
	NULL,                                    // onHassDiscovery
	false,                                   // loaded
	},
#endif
#if ENABLE_DRIVER_PIXELANIM
	//drvdetail:{"name":"PixelAnim",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"PixelAnim provides a simple set of WS2812B animations",
	//drvdetail:"requires":""}
	{ "PixelAnim",                           // Driver Name
	PixelAnim_Init,                          // Init
	NULL,                                    // onEverySecond
	NULL,                                    // appendInformationToHTTPIndexPage
	PixelAnim_SetAnimQuickTick,              // runQuickTick
	NULL,                                    // stopFunction
	NULL,                                    // onChannelChanged
	NULL,                                    // onHassDiscovery
	false,                                   // loaded
	},
#endif
#if ENABLE_DRIVER_DRAWERS
	//drvdetail:{"name":"Drawers",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"WS2812B driver wrapper with REST API for [smart drawers project](https://www.elektroda.com/rtvforum/topic4054134.html)",
	//drvdetail:"requires":""}
	{ "Drawers",                             // Driver Name
	Drawers_Init,                            // Init
	NULL,                                    // onEverySecond
	NULL,                                    // appendInformationToHTTPIndexPage
	Drawers_QuickTick,                       // runQuickTick
	NULL,                                    // stopFunction
	NULL,                                    // onChannelChanged
	NULL,                                    // onHassDiscovery
	false,                                   // loaded
	},
#endif
#if ENABLE_DRIVER_HGS02
	//drvdetail:{"name":"HGS02",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"[HGS02](https://www.elektroda.com/rtvforum/viewtopic.php?p=21177061#21177061)",
	//drvdetail:"requires":""}
	{ "HGS02",                               // Driver Name
	HGS02_Init,                              // Init
	HGS02_RunEverySecond,                    // onEverySecond
	NULL,                                    // appendInformationToHTTPIndexPage
	NULL,                                    // runQuickTick
	NULL,                                    // stopFunction
	NULL,                                    // onChannelChanged
	NULL,                                    // onHassDiscovery
	false,                                   // loaded
	},
#endif
#if ENABLE_DRIVER_PINMUTEX
	//drvdetail:{"name":"PinMutex",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"PinMutex.",
	//drvdetail:"requires":""}
	{ "PinMutex",                            // Driver Name
	DRV_PinMutex_Init,                       // Init
	NULL,                                    // onEverySecond
	NULL,                                    // appendInformationToHTTPIndexPage
	DRV_PinMutex_RunFrame,                   // runQuickTick
	NULL,                                    // stopFunction
	NULL,                                    // onChannelChanged
	NULL,                                    // onHassDiscovery
	false,                                   // loaded
	},
#endif
#if ENABLE_DRIVER_GOSUNDSW2
	//drvdetail:{"name":"GosundSW2",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"GosundSW2",
	//drvdetail:"requires":""}
	{ "GosundSW2",                           // Driver Name
	DRV_GosundSW2_Init,                      // Init
	NULL,                                    // onEverySecond
	NULL,                                    // appendInformationToHTTPIndexPage
	DRV_GosundSW2_RunFrame,                  // runQuickTick
	NULL,                                    // stopFunction
	NULL,                                    // onChannelChanged
	NULL,                                    // onHassDiscovery
	false,                                   // loaded
	},
#endif
#if ENABLE_DRIVER_TCL
	//drvdetail:{"name":"TCL",
	//drvdetail:"title":"TCL",
	//drvdetail:"descr":"Driver for TCL-based air conditioners",
	//drvdetail:"requires":""}
	{ "TCL",                                 // Driver Name
	TCL_Init,                                // Init
	TCL_UART_RunEverySecond,                 // onEverySecond
	TCL_AppendInformationToHTTPIndexPage,    // appendInformationToHTTPIndexPage
	NULL,                                    // runQuickTick
	NULL,                                    // stopFunction
	NULL,                                    // onChannelChanged
	TCL_DoDiscovery,                         // onHassDiscovery
	false,                                   // loaded
	},
#endif
#if ENABLE_DRIVER_OPENWEATHERMAP
	//drvdetail:{"name":"OpenWeatherMap",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"OpenWeatherMap integration allows you to fetch current weather for your lat/long. You can later extract temperatura, humidity and pressure data and display it on main page.",
	//drvdetail:"requires":""}
	{ "OpenWeatherMap",                      // Driver Name
	DRV_OpenWeatherMap_Init,                 // Init
	NULL,                                    // onEverySecond
	OWM_AppendInformationToHTTPIndexPage,    // appendInformationToHTTPIndexPage
	NULL,                                    // runQuickTick
	NULL,                                    // stopFunction
	NULL,                                    // onChannelChanged
	NULL,                                    // onHassDiscovery
	false,                                   // loaded
	},
#endif
#if ENABLE_DRIVER_WIDGET
	//drvdetail:{"name":"Widget",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"Widget driver allows you to create custom HTML snippets that are displayed on main OBK page. Snippets are loaded from LittleFS file system and can use OBK REST API.",
	//drvdetail:"requires":""}
	{ "Widget",                              // Driver Name
	DRV_Widget_Init,                         // Init
	NULL,                                    // onEverySecond
	DRV_Widget_AddToHtmlPage,                // appendInformationToHTTPIndexPage
	NULL,                                    // runQuickTick
	NULL,                                    // stopFunction
	NULL,                                    // onChannelChanged
	NULL,                                    // onHassDiscovery
	false,                                   // loaded
	},
#endif
#if WINDOWS
	//drvdetail:{"name":"TestCharts",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"Development only driver - a sample of chart generation with chart.js.",
	//drvdetail:"requires":""}
	{ "TestCharts",                          // Driver Name
	NULL,                                    // Init
	NULL,                                    // onEverySecond
	DRV_Test_Charts_AddToHtmlPage,           // appendInformationToHTTPIndexPage
	NULL,                                    // runQuickTick
	NULL,                                    // stopFunction
	NULL,                                    // onChannelChanged
	NULL,                                    // onHassDiscovery
	false,                                   // loaded
	},
#endif
#if ENABLE_DRIVER_CHARTS
	//drvdetail:{"name":"Charts",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"Charts driver allows you to create a customizable chart directly on your device. See [tutorial](https://www.elektroda.com/rtvforum/topic4075289.html).",
	//drvdetail:"requires":""}
	{ "Charts",                              // Driver Name
	DRV_Charts_Init,                         // Init
	NULL,                                    // onEverySecond
	DRV_Charts_AddToHtmlPage,                // appendInformationToHTTPIndexPage
	NULL,                                    // runQuickTick
	NULL,                                    // stopFunction
	NULL,                                    // onChannelChanged
	NULL,                                    // onHassDiscovery
	false,                                   // loaded
	},
#endif
#if ENABLE_NTP
	//drvdetail:{"name":"NTP",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"NTP driver is required to get current time and date from web. Without it, there is no correct datetime. Put 'startDriver NTP' in short startup line or autoexec.bat to run it on start.",
	//drvdetail:"requires":""}
	{ "NTP",                                 // Driver Name
	NTP_Init,                                // Init
	NTP_OnEverySecond,                       // onEverySecond
	NTP_AppendInformationToHTTPIndexPage,    // appendInformationToHTTPIndexPage
	NULL,                                    // runQuickTick
   	NTP_Stop,                                // stopFunction
   	NULL,                                    // onChannelChanged
   	NULL,                                    // onHassDiscovery
   	false,                                   // loaded
	},
#endif
#if ENABLE_DRIVER_DS3231
	//drvdetail:{"name":"DS3231",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"Driver for DS3231 RTC.\nStart with \"startdriver DS3231 &lt;CLK-Pin&gt; &lt;DATA-Pin&gt; [&lt;optional sync&gt;]\".\nSync values: 0 - do nothing / 1: set device clock to RTC on driver start / 2: regulary (every minute) set device clock to RTC (so RTC is time source)",
	//drvdetail:"requires":""}
	{ "DS3231",                              // Driver Name
   	DS3231_Init,                             // Init
   	DS3231_OnEverySecond,                    // onEverySecond
   	DS3231_AppendInformationToHTTPIndexPage, // appendInformationToHTTPIndexPage
   	NULL,                                    // runQuickTick
   	DS3231_Stop,                             // stopFunction
   	NULL,                                    // onChannelChanged
   	NULL,                                    // onHassDiscovery
   	false,                                   // loaded
	},
#endif
#if ENABLE_DRIVER_HTTPBUTTONS
	//drvdetail:{"name":"HTTPButtons",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"This driver allows you to create custom, scriptable buttons on main WWW page. You can create those buttons in autoexec.bat and assign commands to them",
	//drvdetail:"requires":""}
	{ "HTTPButtons",                         // Driver Name
	DRV_InitHTTPButtons,                     // Init
	NULL,                                    // onEverySecond
	NULL,                                    // appendInformationToHTTPIndexPage
	NULL,                                    // runQuickTick
	NULL,                                    // stopFunction
	NULL,                                    // onChannelChanged
	NULL,                                    // onHassDiscovery
	false,                                   // loaded
	},
#endif
#if ENABLE_DRIVER_TESTPOWER
	//drvdetail:{"name":"TESTPOWER",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"This is a fake POWER measuring socket driver, only for testing",
	//drvdetail:"requires":""}
	{ "TESTPOWER",                           // Driver Name
	Test_Power_Init,                         // Init
	Test_Power_RunEverySecond,               // onEverySecond
	BL09XX_AppendInformationToHTTPIndexPage, // appendInformationToHTTPIndexPage
	NULL,                                    // runQuickTick
	NULL,                                    // stopFunction
	NULL,                                    // onChannelChanged
	NULL,                                    // onHassDiscovery
	false,                                   // loaded
	},
#endif
#if ENABLE_DRIVER_TESTLED
	//drvdetail:{"name":"TESTLED",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"This is a fake I2C LED driver, only for testing",
	//drvdetail:"requires":""}
	{ "TESTLED",                             // Driver Name
	Test_LED_Driver_Init,                    // Init
	Test_LED_Driver_RunEverySecond,          // onEverySecond
	NULL,                                    // appendInformationToHTTPIndexPage
	NULL,                                    // runQuickTick
	NULL,                                    // stopFunction
	Test_LED_Driver_OnChannelChanged,        // onChannelChanged
	NULL,                                    // onHassDiscovery
	false,                                   // loaded
	},
#endif
#if ENABLE_DRIVER_TESTUART
	//drvdetail:{"name":"TESTUART",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"g",
	//drvdetail:"requires":""}
	{ "TESTUART",                            // Driver Name
	Test_UART_Init,                          // Init
	Test_UART_RunEverySecond,                // onEverySecond
	Test_UART_AppendInformationToHTTPIndexPage, // appendInformationToHTTPIndexPage
	NULL,                                    // runQuickTick
	NULL,                                    // stopFunction
	NULL,                                    // onChannelChanged
	NULL,                                    // onHassDiscovery
	false,                                   // loaded
	},
#endif
#if ENABLE_TEST_COMMANDS
	//drvdetail:{"name":"Test",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"Self test of the device",
	//drvdetail:"requires":""}
	{ "Test",                                // Driver Name
	Test_Init,                               // Init
	NULL,                                    // onEverySecond
	Test_AppendInformationToHTTPIndexPage,   // appendInformationToHTTPIndexPage
	Test_RunQuickTick,                       // runQuickTick
	NULL,                                    // stopFunction
	NULL,                                    // onChannelChanged
	NULL,                                    // onHassDiscovery
	false,                                   // loaded
	},
#endif
#if ENABLE_SIMPLEEEPROM
	//drvdetail:{"name":"SimpleEEPROM",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"SimpleEEPROM",
	//drvdetail:"requires":""}
	{ "SimpleEEPROM",                        // Driver Name
	EEPROM_Init,                             // Init
	NULL,                                    // onEverySecond
	EEPROM_AppendInformationToHTTPIndexPage, // appendInformationToHTTPIndexPage
	NULL,                                    // runQuickTick
	NULL,                                    // stopFunction
	NULL,                                    // onChannelChanged
	NULL,                                    // onHassDiscovery
	false,                                   // loaded
	},
#endif
#if ENABLE_MULTIPINI2CSCANNER
	//drvdetail:{"name":"MultiPinI2CScanner",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"qq.",
	//drvdetail:"requires":""}
	{ "MultiPinI2CScanner",                  // Driver Name
	MultiPinI2CScanner_Init,                 // Init
	NULL,                                    // onEverySecond
	MultiPinI2CScanner_AppendInformationToHTTPIndexPage, // appendInformationToHTTPIndexPage
	MultiPinI2CScanner_RunFrame,             // runQuickTick
	NULL,                                    // stopFunction
	NULL,                                    // onChannelChanged
	NULL,                                    // onHassDiscovery
	false,                                   // loaded
	},
#endif
#if ENABLE_I2C
	//drvdetail:{"name":"I2C",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"Generic I2C, not used for LED drivers, but may be useful for displays or port expanders. Supports both hardware and software I2C.",
	//drvdetail:"requires":""}
	{ "I2C",                                 // Driver Name
	DRV_I2C_Init,                            // Init
	DRV_I2C_EverySecond,                     // onEverySecond
	NULL,                                    // appendInformationToHTTPIndexPage
	NULL,                                    // runQuickTick
	DRV_I2C_Shutdown,                        // stopFunction
	NULL,                                    // onChannelChanged
	NULL,                                    // onHassDiscovery
	false,                                   // loaded
	},
#endif
#if ENABLE_DRIVER_RN8209
	//drvdetail:{"name":"RN8209",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"WIP driver for power-metering chip RN8209 found in one of Zmai-90 versions.",
	//drvdetail:"requires":""}
	{ "RN8209",                              // Driver Name
	RN8209_Init,                             // Init
	RN8029_RunEverySecond,                   // onEverySecond
	BL09XX_AppendInformationToHTTPIndexPage, // appendInformationToHTTPIndexPage
	NULL,                                    // runQuickTick
	NULL,                                    // stopFunction
	NULL,                                    // onChannelChanged
	NULL,                                    // onHassDiscovery
	false,                                   // loaded
	},
#endif
#if ENABLE_DRIVER_BL0942
	//drvdetail:{"name":"BL0942",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"BL0942 is a power-metering chip which uses UART protocol for communication. It's usually connected to TX1/RX1 port of BK. You need to calibrate power metering once, just like in Tasmota. See [LSPA9 teardown example](https://www.elektroda.com/rtvforum/topic3887748.html). By default, it uses 4800 baud, but you can also enable it with baud 9600 by using 'startDriver BL0942 9600', see [related topic](https://www.elektroda.com/rtvforum/viewtopic.php?p=20957896#20957896)",
	//drvdetail:"requires":""}
	{ "BL0942",                              // Driver Name
	BL0942_UART_Init,                        // Init
	BL0942_UART_RunEverySecond,              // onEverySecond
	BL09XX_AppendInformationToHTTPIndexPage, // appendInformationToHTTPIndexPage
	NULL,                                    // runQuickTick
	NULL,                                    // stopFunction
	NULL,                                    // onChannelChanged
	NULL,                                    // onHassDiscovery
	false,                                   // loaded
	},
#endif
#if ENABLE_DRIVER_PWM_GROUP
	//drvdetail:{"name":"PWMG",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"PWM Groups (synchronized PWMs) driver for OpenBeken.",
	//drvdetail:"requires":""}
	{ "PWMG",                                // Driver Name
	PWMG_Init,                               // Init
	NULL,                                    // onEverySecond
	NULL,                                    // appendInformationToHTTPIndexPage
	NULL,                                    // runQuickTick
	NULL,                                    // stopFunction
	NULL,                                    // onChannelChanged
	NULL,                                    // onHassDiscovery
	false,                                   // loaded
	},
#endif
#if ENABLE_DRIVER_BL0942SPI
	//drvdetail:{"name":"BL0942SPI",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"BL0942 driver version for SPI protocol. It's usually connected to SPI1 port of BK. You need to calibrate power metering once, just like in Tasmota. See [PZIOT-E01 teardown example](https://www.elektroda.com/rtvforum/topic3945667.html). ",
	//drvdetail:"requires":""}
	{ "BL0942SPI",                           // Driver Name
	BL0942_SPI_Init,                         // Init
	BL0942_SPI_RunEverySecond,               // onEverySecond
	BL09XX_AppendInformationToHTTPIndexPage, // appendInformationToHTTPIndexPage
	NULL,                                    // runQuickTick
	NULL,                                    // stopFunction
	NULL,                                    // onChannelChanged
	NULL,                                    // onHassDiscovery
	false,                                   // loaded
	},
#endif
#if ENABLE_DRIVER_HLW8112SPI
	//drvdetail:{"name":"HLW8112SPI",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"TODO",
	//drvdetail:"requires":""}
	{ "HLW8112SPI",                          // Driver Name
	HLW8112SPI_Init,                         // Init
	HLW8112_RunEverySecond,                  // onEverySecond
	HLW8112_AppendInformationToHTTPIndexPage, // appendInformationToHTTPIndexPage
	NULL,                                    // runQuickTick
	HLW8112SPI_Stop,                         // stopFunction
	NULL,                                    // onChannelChanged
	HLW8112_OnHassDiscovery,                 // onHassDiscovery
	false,                                   // loaded
	},
#endif
#if ENABLE_DRIVER_CHARGINGLIMIT
	//drvdetail:{"name":"ChargingLimit",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"Mechanism to perform an action based on a max. delta value and max time. Used to control Electric Vehicle chargers. See [discussion](https://github.com/openshwprojects/OpenBK7231T_App/issues/892).",
	//drvdetail:"requires":""}
	{ "ChargingLimit",                       // Driver Name
	ChargingLimit_Init,                      // Init
	ChargingLimit_OnEverySecond,             // onEverySecond
	ChargingLimit_AppendInformationToHTTPIndexPage, // appendInformationToHTTPIndexPage
	NULL,                                    // runQuickTick
	NULL,                                    // stopFunction
	NULL,                                    // onChannelChanged
	NULL,                                    // onHassDiscovery
	false,                                   // loaded
	},
#endif
#if ENABLE_DRIVER_BL0937
	//drvdetail:{"name":"BL0937",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"BL0937 is a power-metering chip which uses custom protocol to report data. It requires setting 3 pins in pin config: CF, CF1 and SEL",
	//drvdetail:"requires":""}
	{ "BL0937",                              // Driver Name
	BL0937_Init,                             // Init
	BL0937_RunEverySecond,                   // onEverySecond
	BL09XX_AppendInformationToHTTPIndexPage, // appendInformationToHTTPIndexPage
	NULL,                                    // runQuickTick
	NULL,                                    // stopFunction
	NULL,                                    // onChannelChanged
	NULL,                                    // onHassDiscovery
	false,                                   // loaded
	},
#endif
#if ENABLE_DRIVER_CSE7761
	//drvdetail:{"name":"CSE7761",
	//drvdetail:"title":"CSE7761",
	//drvdetail:"descr":"Unfinished driver for CSE7761, a single-phase multi-purpose electric energy metering chip that incorporates three sigma delta ADCs, a power calculator, an energy frequency converter, one SPI interface, and one UART interface",
	//drvdetail:"requires":""}
	{ "CSE7761",                             // Driver Name
	CSE7761_Init,                            // Init
	CSE7761_RunEverySecond,                  // onEverySecond
	BL09XX_AppendInformationToHTTPIndexPage, // appendInformationToHTTPIndexPage
	NULL,                                    // runQuickTick
	NULL,                                    // stopFunction
	NULL,                                    // onChannelChanged
	NULL,                                    // onHassDiscovery
	false,                                   // loaded
	},
#endif
#if ENABLE_DRIVER_CSE7766
	//drvdetail:{"name":"CSE7766",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"CSE7766 is a power-metering chip which uses UART protocol for communication. It's usually connected to TX1/RX1 port of BK",
	//drvdetail:"requires":""}
	{ "CSE7766",                             // Driver Name
	CSE7766_Init,                            // Init
	CSE7766_RunEverySecond,                  // onEverySecond
	BL09XX_AppendInformationToHTTPIndexPage, // appendInformationToHTTPIndexPage
	NULL,                                    // runQuickTick
	NULL,                                    // stopFunction
	NULL,                                    // onChannelChanged
	NULL,                                    // onHassDiscovery
	false,                                   // loaded
	},
#endif
#if ENABLE_DRIVER_MAX6675
	//drvdetail:{"name":"MAX6675",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"Thermocouple driver for measuring high temperatures, see [presentation](https://www.elektroda.com/rtvforum/topic4055231.html)",
	//drvdetail:"requires":""}
	{ "MAX6675",                             // Driver Name
	MAX6675_Init,                            // Init
	MAX6675_RunEverySecond,                  // onEverySecond
	NULL,                                    // appendInformationToHTTPIndexPage
	NULL,                                    // runQuickTick
	NULL,                                    // stopFunction
	NULL,                                    // onChannelChanged
	NULL,                                    // onHassDiscovery
	false,                                   // loaded
	},
#endif
#if ENABLE_DRIVER_MAX31855
	//drvdetail:{"name":"MAX31855",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"T",
	//drvdetail:"requires":""}
	{ "MAX31855",                            // Driver Name
	MAX31855_Init,                           // Init
	MAX31855_RunEverySecond,                 // onEverySecond
	NULL,                                    // appendInformationToHTTPIndexPage
	NULL,                                    // runQuickTick
	NULL,                                    // stopFunction
	NULL,                                    // onChannelChanged
	NULL,                                    // onHassDiscovery
	false,                                   // loaded
	},
#endif
#if ENABLE_DRIVER_PT6523
	//drvdetail:{"name":"PT6523",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"Car radio LCD driver, see [teardown and presentation](https://www.elektroda.com/rtvforum/topic3983111.html)",
	//drvdetail:"requires":""}
	{ "PT6523",                              // Driver Name
	PT6523_Init,                             // Init
	PT6523_RunFrame,                         // onEverySecond
	NULL,                                    // appendInformationToHTTPIndexPage
	NULL,                                    // runQuickTick
	NULL,                                    // stopFunction
	NULL,                                    // onChannelChanged
	NULL,                                    // onHassDiscovery
	false,                                   // loaded
	},
#endif
#if ENABLE_DRIVER_TEXTSCROLLER
	//drvdetail:{"name":"TextScroller",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"Wrapper utility that can do text scrolling animation on implemented displays (WIP)",
	//drvdetail:"requires":""}
	{ "TextScroller",                        // Driver Name
	TS_Init,                                 // Init
	NULL,                                    // onEverySecond
	NULL,                                    // appendInformationToHTTPIndexPage
	TS_RunQuickTick,                         // runQuickTick
	NULL,                                    // stopFunction
	NULL,                                    // onChannelChanged
	NULL,                                    // onHassDiscovery
	false,                                   // loaded
	},
#endif
#if ENABLE_DRIVER_SM16703P
	//drvdetail:{"name":"SM16703P",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"SM16703P is an individually addressable LEDs controller like WS2812B. Currently SM16703P LEDs are supported through hardware SPI, LEDs data should be connected to P16 (MOSI), [here you can read](https://www.elektroda.com/rtvforum/topic4005865.html) how to break it out on CB2S.",
	//drvdetail:"requires":""}
	{ "SM16703P",                            // Driver Name
	SM16703P_Init,                           // Init
	NULL,                                    // onEverySecond
	NULL,                                    // appendInformationToHTTPIndexPage
	NULL,                                    // runQuickTick
	SM16703P_Shutdown,                       // stopFunction
	NULL,                                    // onChannelChanged
	NULL,                                    // onHassDiscovery
	false,                                   // loaded
	},
#endif
#if ENABLE_DRIVER_SM15155E
	//drvdetail:{"name":"SM15155E",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"SM15155E is a WS2812B-like single wire LED controller. It's also always using P16 (SPI out) on Beken. See [reverse-engineering topic](https://www.elektroda.com/rtvforum/topic4060227.html)",
	//drvdetail:"requires":""}
	{ "SM15155E",                            // Driver Name
	SM15155E_Init,                           // Init
	NULL,                                    // onEverySecond
	NULL,                                    // appendInformationToHTTPIndexPage
	NULL,                                    // runQuickTick
	NULL,                                    // stopFunction
	NULL,                                    // onChannelChanged
	NULL,                                    // onHassDiscovery
	false,                                   // loaded
	},
#endif
#if ENABLE_DRIVER_IRREMOTEESP
	//drvdetail:{"name":"IR",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"IRLibrary wrapper, so you can receive remote signals and send them. See [forum discussion here](https://www.elektroda.com/rtvforum/topic3920360.html), also see [LED strip and IR YT video](https://www.youtube.com/watch?v=KU0tDwtjfjw)",
	//drvdetail:"requires":""}
	{ "IR",                                  // Driver Name
	DRV_IR_Init,                             // Init
	NULL,                                    // onEverySecond
	NULL,                                    // appendInformationToHTTPIndexPage
	DRV_IR_RunFrame,                         // runQuickTick
	NULL,                                    // stopFunction
	NULL,                                    // onChannelChanged
	NULL,                                    // onHassDiscovery
	false,                                   // loaded
	},
#endif
#if ENABLE_DRIVER_IR
	//drvdetail:{"name":"IR",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"IRLibrary wrapper, so you can receive remote signals and send them. See [forum discussion here](https://www.elektroda.com/rtvforum/topic3920360.html), also see [LED strip and IR YT video](https://www.youtube.com/watch?v=KU0tDwtjfjw)",
	//drvdetail:"requires":""}
	{ "IR",                                  // Driver Name
	DRV_IR_Init,                             // Init
	NULL,                                    // onEverySecond
	NULL,                                    // appendInformationToHTTPIndexPage
	DRV_IR_RunFrame,                         // runQuickTick
	NULL,                                    // stopFunction
	NULL,                                    // onChannelChanged
	NULL,                                    // onHassDiscovery
	false,                                   // loaded
	},
#endif
#if ENABLE_DRIVER_RC
		//drvdetail:{"name":"RC",
		//drvdetail:"title":"TODO",
		//drvdetail:"descr":"",
		//drvdetail:"requires":""}
	{ "RC",                                  // Driver Name
	DRV_RC_Init,                             // Init
	NULL,                                    // onEverySecond
	RC_AppendInformationToHTTPIndexPage,                                    // appendInformationToHTTPIndexPage
	DRV_RC_RunFrame,                         // runQuickTick
	NULL,                                    // stopFunction
	NULL,                                    // onChannelChanged
	NULL,                                    // onHassDiscovery
	false,                                   // loaded
	},
#endif
#if ENABLE_DRIVER_IR2
	//drvdetail:{"name":"IR2",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"simple IR2 driver for sending captures from flipper zero",
	//drvdetail:"requires":""}
	{ "IR2",                                 // Driver Name
	DRV_IR2_Init,                            // Init
	NULL,                                    // onEverySecond
	NULL,                                    // appendInformationToHTTPIndexPage
	NULL,                                    // runQuickTick
	NULL,                                    // stopFunction
	NULL,                                    // onChannelChanged
	NULL,                                    // onHassDiscovery
	false,                                   // loaded
	},
#endif
#if ENABLE_DRIVER_DDPSEND
	//drvdetail:{"name":"DDPSend",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"DDPqqqqqqq. See [DDP topic](https://www.elektroda.com/rtvforum/topic4040325.html)",
	//drvdetail:"requires":""}
	{ "DDPSend",                             // Driver Name
	DRV_DDPSend_Init,                        // Init
	NULL,                                    // onEverySecond
	DRV_DDPSend_AppendInformationToHTTPIndexPage, // appendInformationToHTTPIndexPage
	DRV_DDPSend_RunFrame,                    // runQuickTick
	DRV_DDPSend_Shutdown,                    // stopFunction
	NULL,                                    // onChannelChanged
	NULL,                                    // onHassDiscovery
	false,                                   // loaded
	},
#endif
#if ENABLE_DRIVER_DDP
	//drvdetail:{"name":"DDP",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"DDP is a LED control protocol that is using UDP. You can use xLights or any other app to control OBK LEDs that way. See [DDP topic](https://www.elektroda.com/rtvforum/topic4040325.html)",
	//drvdetail:"requires":""}
	{ "DDP",                                 // Driver Name
	DRV_DDP_Init,                            // Init
	NULL,                                    // onEverySecond
	DRV_DDP_AppendInformationToHTTPIndexPage, // appendInformationToHTTPIndexPage
	DRV_DDP_RunFrame,                        // runQuickTick
	DRV_DDP_Shutdown,                        // stopFunction
	NULL,                                    // onChannelChanged
	NULL,                                    // onHassDiscovery
	false,                                   // loaded
	},
#endif
#if ENABLE_DRIVER_SSDP
	//drvdetail:{"name":"SSDP",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"SSDP is a discovery protocol, so BK devices can show up in, for example, Windows network section",
	//drvdetail:"requires":""}
	{ "SSDP",                                // Driver Name
	DRV_SSDP_Init,                           // Init
	DRV_SSDP_RunEverySecond,                 // onEverySecond
	NULL,                                    // appendInformationToHTTPIndexPage
	DRV_SSDP_RunQuickTick,                   // runQuickTick
	DRV_SSDP_Shutdown,                       // stopFunction
	NULL,                                    // onChannelChanged
	NULL,                                    // onHassDiscovery
	false,                                   // loaded
	},
#endif
#if ENABLE_TASMOTADEVICEGROUPS
	//drvdetail:{"name":"DGR",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"Tasmota Device groups driver. See [forum example](https://www.elektroda.com/rtvforum/topic3925472.html) and [video tutorial](https://www.youtube.com/watch?v=e1xcq3OUR5M&ab_channel=Elektrodacom)",
	//drvdetail:"requires":""}
	{ "DGR",                                 // Driver Name
	DRV_DGR_Init,                            // Init
	DRV_DGR_RunEverySecond,                  // onEverySecond
	DRV_DGR_AppendInformationToHTTPIndexPage, // appendInformationToHTTPIndexPage
	DRV_DGR_RunQuickTick,                    // runQuickTick
	DRV_DGR_Shutdown,                        // stopFunction
	DRV_DGR_OnChannelChanged,                // onChannelChanged
	NULL,                                    // onHassDiscovery
	false,                                   // loaded
	},
#endif
#if ENABLE_DRIVER_WEMO
	//drvdetail:{"name":"Wemo",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"Wemo emulation for Alexa. You must also start SSDP so it can run, because it depends on SSDP discovery.",
	//drvdetail:"requires":""}
	{ "Wemo",                                // Driver Name
	WEMO_Init,                               // Init
	NULL,                                    // onEverySecond
	WEMO_AppendInformationToHTTPIndexPage,   // appendInformationToHTTPIndexPage
	NULL,                                    // runQuickTick
	NULL,                                    // stopFunction
	NULL,                                    // onChannelChanged
	NULL,                                    // onHassDiscovery
	false,                                   // loaded
	},
#endif
#if ENABLE_DRIVER_HUE
	//drvdetail:{"name":"Hue",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"Hue emulation for Alexa. You must also start SSDP so it can run, because it depends on SSDP discovery.",
	//drvdetail:"requires":""}
	{ "Hue",                                 // Driver Name
	HUE_Init,                                // Init
	NULL,                                    // onEverySecond
	HUE_AppendInformationToHTTPIndexPage,    // appendInformationToHTTPIndexPage
	NULL,                                    // runQuickTick
	NULL,                                    // stopFunction
	NULL,                                    // onChannelChanged
	NULL,                                    // onHassDiscovery
	false,                                   // loaded
	},
#endif
#if defined(PLATFORM_BEKEN) || defined(WINDOWS)
	//drvdetail:{"name":"PWMToggler",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"PWMToggler is a custom abstraction layer that can run on top of raw PWM channels. It provides ability to turn off/on the PWM while keeping it's value, which is not possible by direct channel operations. It can be used for some custom devices with extra lights/lasers. See example [here](https://www.elektroda.com/rtvforum/topic3939064.html).",
	//drvdetail:"requires":""}
	{ "PWMToggler",                          // Driver Name
	DRV_InitPWMToggler,                      // Init
	NULL,                                    // onEverySecond
	DRV_Toggler_AppendInformationToHTTPIndexPage, // appendInformationToHTTPIndexPage
	DRV_Toggler_QuickTick,                   // runQuickTick
	NULL,                                    // stopFunction
	NULL,                                    // onChannelChanged
	NULL,                                    // onHassDiscovery
	false,                                   // loaded
	},
	//drvdetail:{"name":"DoorSensor",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"DoorSensor is using deep sleep to preserve battery. This is used for devices without TuyaMCU, where BK deep sleep and wakeup on GPIO is used. This drives requires you to set a DoorSensor pin. Change on door sensor pin wakes up the device. If there are no changes for some time, device goes to sleep. See example [here](https://www.elektroda.com/rtvforum/topic3960149.html). If your door sensor does not wake up in certain pos, please use DSEdge command (try all 3 options, default is 2). ",
	//drvdetail:"requires":""}
	{ "DoorSensor",                          // Driver Name
	DoorDeepSleep_Init,                      // Init
	DoorDeepSleep_OnEverySecond,             // onEverySecond
	DoorDeepSleep_AppendInformationToHTTPIndexPage, // appendInformationToHTTPIndexPage
	NULL,                                    // runQuickTick
	NULL,                                    // stopFunction
	DoorDeepSleep_OnChannelChanged,          // onChannelChanged
	NULL,                                    // onHassDiscovery
	false,                                   // loaded
	},
#endif
#if ENABLE_DRIVER_ADCBUTTON
	//drvdetail:{"name":"ADCButton",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"This allows you to connect multiple buttons on single ADC pin. Each button must have a different resistor value, this works by probing the voltage on ADC from a resistor divider. You need to select AB_Map first. See forum post for [details](https://www.elektroda.com/rtvforum/viewtopic.php?p=20541973#20541973).",
	//drvdetail:"requires":""}
	{ "ADCButton",                           // Driver Name
	DRV_ADCButton_Init,                      // Init
	NULL,                                    // onEverySecond
	NULL,                                    // appendInformationToHTTPIndexPage
	DRV_ADCButton_RunFrame,                  // runQuickTick
	NULL,                                    // stopFunction
	NULL,                                    // onChannelChanged
	NULL,                                    // onHassDiscovery
	false,                                   // loaded
	},
#endif
#if ENABLE_DRIVER_MAX72XX
	//drvdetail:{"name":"MAX72XX_Clock",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"Simple hardcoded driver for MAX72XX clock. Requires manual start of MAX72XX driver with MAX72XX setup and NTP start.",
	//drvdetail:"requires":""}
	{ "MAX72XX_Clock",                       // Driver Name
	DRV_MAX72XX_Clock_Init,                  // Init
	DRV_MAX72XX_Clock_OnEverySecond,         // onEverySecond
	NULL,                                    // appendInformationToHTTPIndexPage
	DRV_MAX72XX_Clock_RunFrame,              // runQuickTick
	NULL,                                    // stopFunction
	NULL,                                    // onChannelChanged
	NULL,                                    // onHassDiscovery
	false,                                   // loaded
	},
#endif
#if ENABLE_DRIVER_LED
	//drvdetail:{"name":"SM2135",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"SM2135 custom-'I2C' LED driver for RGBCW lights. This will start automatically if you set both SM2135 pin roles. This may need you to remap the RGBCW indexes with SM2135_Map command",
	//drvdetail:"requires":""}
	{ "SM2135",                              // Driver Name
	SM2135_Init,                             // Init
	NULL,                                    // onEverySecond
	NULL,                                    // appendInformationToHTTPIndexPage
	NULL,                                    // runQuickTick
	NULL,                                    // stopFunction
	NULL,                                    // onChannelChanged
	NULL,                                    // onHassDiscovery
	false,                                   // loaded
	},
	//drvdetail:{"name":"BP5758D",
	//drvdetail:"title":"TODO",	
	//drvdetail:"descr":"BP5758D custom-'I2C' LED driver for RGBCW lights. This will start automatically if you set both BP5758D pin roles. This may need you to remap the RGBCW indexes with BP5758D_Map command. This driver is used in some of BL602/Sonoff bulbs, see [video flashing tutorial here](https://www.youtube.com/watch?v=L6d42IMGhHw)",
	//drvdetail:"requires":""}
	{ "BP5758D",                             // Driver Name
	BP5758D_Init,                            // Init
	NULL,                                    // onEverySecond
	NULL,                                    // appendInformationToHTTPIndexPage
	NULL,                                    // runQuickTick
	NULL,                                    // stopFunction
	NULL,                                    // onChannelChanged
	NULL,                                    // onHassDiscovery
	false,                                   // loaded
	},
	//drvdetail:{"name":"BP1658CJ",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"BP1658CJ custom-'I2C' LED driver for RGBCW lights. This will start automatically if you set both BP1658CJ pin roles. This may need you to remap the RGBCW indexes with BP1658CJ_Map command",
	//drvdetail:"requires":""}
	{ "BP1658CJ",                            // Driver Name
	BP1658CJ_Init,                           // Init
	NULL,                                    // onEverySecond
	NULL,                                    // appendInformationToHTTPIndexPage
	NULL,                                    // runQuickTick
	NULL,                                    // stopFunction
	NULL,                                    // onChannelChanged
	NULL,                                    // onHassDiscovery
	false,                                   // loaded
	},
	//drvdetail:{"name":"SM2235",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"SM2335 andd SM2235 custom-'I2C' LED driver for RGBCW lights. This will start automatically if you set both SM2235 pin roles. This may need you to remap the RGBCW indexes with SM2235_Map command. This driver also works for SM2185N.",
	//drvdetail:"requires":""}
	{ "SM2235",                              // Driver Name
	SM2235_Init,                             // Init
	NULL,                                    // onEverySecond
	NULL,                                    // appendInformationToHTTPIndexPage
	NULL,                                    // runQuickTick
	NULL,                                    // stopFunction
	NULL,                                    // onChannelChanged
	NULL,                                    // onHassDiscovery
	false,                                   // loaded
	},
#endif
#if ENABLE_DRIVER_SSD1306
	//drvdetail:{"name":"SSD1306",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"SSD1306 OLEd 128x32 I2C display driver.",
	//drvdetail:"requires":""}
	{ "SSD1306",                              // Driver Name
		SSD1306_DRV_Init,                             // Init
		SSD1306_OnEverySecond,                    // onEverySecond
		NULL, // appendInformationToHTTPIndexPage
		NULL,                                    // runQuickTick
		NULL,                                    // stopFunction
		NULL,                                    // onChannelChanged
		NULL,                                    // onHassDiscovery
		false,                                   // loaded
	},
#endif
#if ENABLE_DRIVER_BMP280
	//drvdetail:{"name":"BMP280",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"BMP280 is a Temperature and Pressure sensor with I2C interface.",
	//drvdetail:"requires":""}
	{ "BMP280",                              // Driver Name
	BMP280_Init,                             // Init
	BMP280_OnEverySecond,                    // onEverySecond
	BMP280_AppendInformationToHTTPIndexPage, // appendInformationToHTTPIndexPage
	NULL,                                    // runQuickTick
	NULL,                                    // stopFunction
	NULL,                                    // onChannelChanged
	NULL,                                    // onHassDiscovery
	false,                                   // loaded
	},
#endif
#if ENABLE_DRIVER_MAX72XX
	//drvdetail:{"name":"MAX72XX",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"MAX72XX LED matrix display driver with font and simple script interface. See [protocol explanation](https://www.elektroda.pl/rtvforum/viewtopic.php?p=18040628#18040628)",
	//drvdetail:"requires":""}
	{ "MAX72XX",                             // Driver Name
	DRV_MAX72XX_Init,                        // Init
	NULL,                                    // onEverySecond
	NULL,                                    // appendInformationToHTTPIndexPage
	NULL,                                    // runQuickTick
	DRV_MAX72XX_Shutdown,                    // stopFunction
	NULL,                                    // onChannelChanged
	NULL,                                    // onHassDiscovery
	false,                                   // loaded
	},
#endif
#if ENABLE_DRIVER_BMPI2C
		//drvdetail:{"name":"BMPI2C",
		//drvdetail:"title":"TODO",
		//drvdetail:"descr":"Driver for BMP085, BMP180, BMP280, BME280, BME68X sensors with I2C interface.",
		//drvdetail:"requires":""}
	{ "BMPI2C",                              // Driver Name
	BMPI2C_Init,                             // Init
	BMPI2C_OnEverySecond,                    // onEverySecond
	BMPI2C_AppendInformationToHTTPIndexPage, // appendInformationToHTTPIndexPage
	NULL,                                    // runQuickTick
	NULL,                                    // stopFunction
	NULL,                                    // onChannelChanged
	NULL,                                    // onHassDiscovery
	false,                                   // loaded
	},
#endif
#if ENABLE_DRIVER_CHT83XX
	//drvdetail:{"name":"CHT83XX",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"CHT8305, CHT8310 and CHT8315 are a Temperature and Humidity sensors with I2C interface.",
	//drvdetail:"requires":""}
	{ "CHT83XX",                             // Driver Name
	CHT83XX_Init,                            // Init
	CHT83XX_OnEverySecond,                   // onEverySecond
	CHT83XX_AppendInformationToHTTPIndexPage, // appendInformationToHTTPIndexPage
	NULL,                                    // runQuickTick
	NULL,                                    // stopFunction
	NULL,                                    // onChannelChanged
	NULL,                                    // onHassDiscovery
	false,                                   // loaded
	},
#endif
#if ENABLE_DRIVER_MCP9808
	//drvdetail:{"name":"MCP9808",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"MCP9808 is a Temperature sensor with I2C interface and an external wakeup pin, see [docs](https://www.elektroda.pl/rtvforum/topic3988466.html).",
	//drvdetail:"requires":""}
	{ "MCP9808",                             // Driver Name
	MCP9808_Init,                            // Init
	MCP9808_OnEverySecond,                   // onEverySecond
	MCP9808_AppendInformationToHTTPIndexPage, // appendInformationToHTTPIndexPage
	NULL,                                    // runQuickTick
	NULL,                                    // stopFunction
	NULL,                                    // onChannelChanged
	NULL,                                    // onHassDiscovery
	false,                                   // loaded
	},
#endif
#if ENABLE_DRIVER_KP18058
	//drvdetail:{"name":"KP18058",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"KP18058 I2C LED driver. Supports also KP18068. Working, see reverse-engineering [topic](https://www.elektroda.pl/rtvforum/topic3991620.html)",
	//drvdetail:"requires":""}
	{ "KP18058",                             // Driver Name
	KP18058_Init,                            // Init
	NULL,                                    // onEverySecond
	NULL,                                    // appendInformationToHTTPIndexPage
	NULL,                                    // runQuickTick
	NULL,                                    // stopFunction
	NULL,                                    // onChannelChanged
	NULL,                                    // onHassDiscovery
	false,                                   // loaded
	},
#endif
#if ENABLE_DRIVER_ADCSMOOTHER
	//drvdetail:{"name":"ADCSmoother",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"ADCSmoother is used for 3-way stairs switches synchronized via extra wire.",
	//drvdetail:"requires":""}
	{ "ADCSmoother",                         // Driver Name
	DRV_ADCSmoother_Init,                    // Init
	NULL,                                    // onEverySecond
	NULL,                                    // appendInformationToHTTPIndexPage
	DRV_ADCSmoother_RunFrame,                // runQuickTick
	NULL,                                    // stopFunction
	NULL,                                    // onChannelChanged
	NULL,                                    // onHassDiscovery
	false,                                   // loaded
	},
#endif
#if ENABLE_DRIVER_SHT3X
	//drvdetail:{"name":"SHT3X",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"Humidity/temperature sensor. See [SHT Sensor tutorial topic here](https://www.elektroda.com/rtvforum/topic3958369.html), also see [this sensor teardown](https://www.elektroda.com/rtvforum/topic3945688.html)",
	//drvdetail:"requires":""}
	{ "SHT3X",                               // Driver Name
	SHT3X_Init,                              // Init
	SHT3X_OnEverySecond,                     // onEverySecond
	SHT3X_AppendInformationToHTTPIndexPage,  // appendInformationToHTTPIndexPage
	NULL,                                    // runQuickTick
	SHT3X_StopDriver,                        // stopFunction
	NULL,                                    // onChannelChanged
	NULL,                                    // onHassDiscovery
	false,                                   // loaded
	},
#endif
#if ENABLE_DRIVER_SGP
	//drvdetail:{"name":"SGP",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"SGP Air Quality sensor with I2C interface. See [this DIY sensor](https://www.elektroda.com/rtvforum/topic3967174.html) for setup information.",
	//drvdetail:"requires":""}
	{ "SGP",                                 // Driver Name
	SGP_Init,                                // Init
	SGP_OnEverySecond,                       // onEverySecond
	SGP_AppendInformationToHTTPIndexPage,    // appendInformationToHTTPIndexPage
	NULL,                                    // runQuickTick
	SGP_StopDriver,                          // stopFunction
	NULL,                                    // onChannelChanged
	NULL,                                    // onHassDiscovery
	false,                                   // loaded
	},
#endif
#if ENABLE_DRIVER_SHIFTREGISTER
	//drvdetail:{"name":"ShiftRegister",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"Simple Shift Register driver that allows you to map channels to shift register output. See [related topic](https://www.elektroda.com/rtvforum/viewtopic.php?p=20533505#20533505)",
	//drvdetail:"requires":""}
	{ "ShiftRegister",                       // Driver Name
	Shift_Init,                              // Init
	Shift_OnEverySecond,                     // onEverySecond
	NULL,                                    // appendInformationToHTTPIndexPage
	NULL,                                    // runQuickTick
	NULL,                                    // stopFunction
	Shift_OnChannelChanged,                  // onChannelChanged
	NULL,                                    // onHassDiscovery
	false,                                   // loaded
	},
#endif
#if ENABLE_DRIVER_AHT2X
	//drvdetail:{"name":"AHT2X",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"AHT Humidity/temperature sensor. Supported sensors are: AHT10, AHT2X, AHT30. See [presentation guide](https://www.elektroda.com/rtvforum/topic4052685.html)",
	//drvdetail:"requires":""}
	{ "AHT2X",                               // Driver Name
	AHT2X_Init,                              // Init
	AHT2X_OnEverySecond,                     // onEverySecond
	AHT2X_AppendInformationToHTTPIndexPage,  // appendInformationToHTTPIndexPage
	NULL,                                    // runQuickTick
	AHT2X_StopDriver,                        // stopFunction
	NULL,                                    // onChannelChanged
	NULL,                                    // onHassDiscovery
	false,                                   // loaded
	},
#endif
#if ENABLE_DRIVER_DS1820
	//drvdetail:{"name":"DS1820",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"Very simple driver for oneWire temperature sensor DS1820.",
	//drvdetail:"requires":""}
	{ "DS1820",                              // Driver Name
	DS1820_driver_Init,                      // Init
	DS1820_OnEverySecond,                    // onEverySecond
	DS1820_AppendInformationToHTTPIndexPage, // appendInformationToHTTPIndexPage
	NULL,                                    // runQuickTick
	NULL,                                    // stopFunction
	NULL,                                    // onChannelChanged
	NULL,                                    // onHassDiscovery
	false,                                   // loaded
	},
#endif
#if ENABLE_DRIVER_DS1820_FULL
	//drvdetail:{"name":"DS1820_FULL",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"Driver for oneWire temperature sensor DS18(B)20.",
	//drvdetail:"requires":""}
	{ "DS1820_FULL",                         // Driver Name
	DS1820_full_driver_Init,                 // Init
	DS1820_full_OnEverySecond,               // onEverySecond
	DS1820_full_AppendInformationToHTTPIndexPage, // appendInformationToHTTPIndexPage
	NULL,                                    // runQuickTick
	NULL,                                    // stopFunction
	NULL,                                    // onChannelChanged
	NULL,                                    // onHassDiscovery
	false,                                   // loaded
	},
#endif
#if ENABLE_DRIVER_HT16K33
	//drvdetail:{"name":"HT16K33",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"Driver for 16-segment LED display with I2C. See [protocol explanation](https://www.elektroda.pl/rtvforum/topic3984616.html)",
	//drvdetail:"requires":""}
	{ "HT16K33",                             // Driver Name
	HT16K33_Init,                            // Init
	NULL,                                    // onEverySecond
	NULL,                                    // appendInformationToHTTPIndexPage
	NULL,                                    // runQuickTick
	NULL,                                    // stopFunction
	NULL,                                    // onChannelChanged
	NULL,                                    // onHassDiscovery
	false,                                   // loaded
	},
#endif
	// Shared driver for TM1637, GN6932, TM1638 - TM_GN_Display_SharedInit
#if ENABLE_DRIVER_TMGN
	//drvdetail:{"name":"TM1637",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"Driver for 7-segment LED display with DIO/CLK interface. See [TM1637 information](https://www.elektroda.com/rtvforum/viewtopic.php?p=20468593#20468593)",
	//drvdetail:"requires":""}
	{ "TM1637",                              // Driver Name
	TM1637_Init,                             // Init
	NULL,                                    // onEverySecond
	NULL,                                    // appendInformationToHTTPIndexPage
	TMGN_RunQuickTick,                       // runQuickTick
	NULL,                                    // stopFunction
	NULL,                                    // onChannelChanged
	NULL,                                    // onHassDiscovery
	false,                                   // loaded
	},
	//drvdetail:{"name":"GN6932",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"Driver for 7-segment LED display with DIO/CLK/STB interface. See [this topic](https://www.elektroda.com/rtvforum/topic3971252.html) for details.",
	//drvdetail:"requires":""}
	{ "GN6932",                              // Driver Name
	GN6932_Init,                             // Init
	NULL,                                    // onEverySecond
	NULL,                                    // appendInformationToHTTPIndexPage
	TMGN_RunQuickTick,                       // runQuickTick
	NULL,                                    // stopFunction
	NULL,                                    // onChannelChanged
	NULL,                                    // onHassDiscovery
	false,                                   // loaded
	},
	//drvdetail:{"name":"TM1638",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"Driver for 7-segment LED display with DIO/CLK/STB interface. TM1638 is very similiar to GN6932 and TM1637. See [this topic](https://www.elektroda.com/rtvforum/viewtopic.php?p=20553628#20553628) for details.",
	//drvdetail:"requires":""}
	{ "TM1638",                              // Driver Name
	TM1638_Init,                             // Init
	NULL,                                    // onEverySecond
	NULL,                                    // appendInformationToHTTPIndexPage
	TMGN_RunQuickTick,                       // runQuickTick
	NULL,                                    // stopFunction
	NULL,                                    // onChannelChanged
	NULL,                                    // onHassDiscovery
	false,                                   // loaded
	},
	//drvdetail:{"name":"HD2015",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"Driver for 7-segment LED display with I2C-like interface. Seems to be compatible with TM1650. HD2015 is very similiar to GN6932 and TM1637. See [this topic](https://www.elektroda.com/rtvforum/topic4052946.html) for details.",
	//drvdetail:"requires":""}
	{ "HD2015",                              // Driver Name
	HD2015_Init,                             // Init
	NULL,                                    // onEverySecond
	NULL,                                    // appendInformationToHTTPIndexPage
	TMGN_RunQuickTick,                       // runQuickTick
	NULL,                                    // stopFunction
	NULL,                                    // onChannelChanged
	NULL,                                    // onHassDiscovery
	false,                                   // loaded
	},
#endif
#if ENABLE_DRIVER_BATTERY
	//drvdetail:{"name":"Battery",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"Custom mechanism to measure battery level with ADC and an optional relay. See [example here](https://www.elektroda.com/rtvforum/topic3959103.html).",
	//drvdetail:"requires":""}
	{ "Battery",                             // Driver Name
	Batt_Init,                               // Init
	Batt_OnEverySecond,                      // onEverySecond
	Batt_AppendInformationToHTTPIndexPage,   // appendInformationToHTTPIndexPage
	NULL,                                    // runQuickTick
	Batt_StopDriver,                         // stopFunction
	NULL,                                    // onChannelChanged
	NULL,                                    // onHassDiscovery
	false,                                   // loaded
	},
#endif
#if ENABLE_DRIVER_BKPARTITIONS
	//drvdetail:{"name":"BKPartitions",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"o.",
	//drvdetail:"requires":""}
	{ "BKPartitions",                        // Driver Name
	BKPartitions_Init,                       // Init
	NULL,                                    // onEverySecond
	NULL,                                    // appendInformationToHTTPIndexPage
	BKPartitions_QuickFrame,                 // runQuickTick
	NULL,                                    // stopFunction
	NULL,                                    // onChannelChanged
	NULL,                                    // onHassDiscovery
	false,                                   // loaded
	},
#endif
#if ENABLE_DRIVER_BRIDGE
	//drvdetail:{"name":"Bridge",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"A bridge relay driver, added for [TONGOU TO-Q-SY1-JWT Din Rail Switch](https://www.elektroda.com/rtvforum/topic3934580.html). See linked topic for info.",
	//drvdetail:"requires":""}
	{ "Bridge",                              // Driver Name
	Bridge_driver_Init,                      // Init
	NULL,                                    // onEverySecond
	NULL,                                    // appendInformationToHTTPIndexPage
	Bridge_driver_QuickFrame,                // runQuickTick
	Bridge_driver_DeInit,                    // stopFunction
	Bridge_driver_OnChannelChanged,          // onChannelChanged
	NULL,                                    // onHassDiscovery
	false,                                   // loaded
	},
#endif
#if ENABLE_DRIVER_UART_TCP
	//drvdetail:{"name":"UartTCP",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"UART to TCP bridge, mainly for WiFi Zigbee coordinators.",
	//drvdetail:"requires":""}
	{ "UartTCP",                             // Driver Name
	UART_TCP_Init,                           // Init
	NULL,                                    // onEverySecond
	NULL,                                    // appendInformationToHTTPIndexPage
	NULL,                                    // runQuickTick
	UART_TCP_Deinit,                         // stopFunction
	NULL,                                    // onChannelChanged
	NULL,                                    // onHassDiscovery
	false,                                   // loaded
	},
#endif
#if PLATFORM_TXW81X
	//drvdetail:{"name":"TXWCAM",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"TXW81X Camera.",
	//drvdetail:"requires":""}
	{ "TXWCAM",                              // Driver Name
	TXW_Cam_Init,                            // Init
	TXW_Cam_RunEverySecond,                  // onEverySecond
	NULL,                                    // appendInformationToHTTPIndexPage
	NULL,                                    // runQuickTick
	NULL,                                    // stopFunction
	NULL,                                    // onChannelChanged
	NULL,                                    // onHassDiscovery
	false,                                   // loaded
	},
#endif
#if ENABLE_DRIVER_NEO6M
	//drvdetail:{"name":"NEO6M",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"NEO6M is a GPS chip which uses UART protocol for communication. By default, it uses 9600 baud, but you can also enable it with other baud rates by using 'startDriver NEO6M <rate>'.",
	//drvdetail:"requires":""}
	{ "NEO6M",                               // Driver Name
	NEO6M_UART_Init,                         // Init
	NEO6M_UART_RunEverySecond,               // onEverySecond
	NEO6M_AppendInformationToHTTPIndexPage,  // appendInformationToHTTPIndexPage
	NULL,                                    // runQuickTick
	NULL,                                    // stopFunction
	NULL,                                    // onChannelChanged
	NULL,                                    // onHassDiscovery
	false,                                   // loaded
	},
#endif
	//{ "", NULL, NULL, NULL, NULL, NULL, NULL, NULL, false },
};


static const int g_numDrivers = sizeof(g_drivers) / sizeof(g_drivers[0]);

bool DRV_IsRunning(const char* name) {
	int i;

	for (i = 0; i < g_numDrivers; i++) {
		if (g_drivers[i].bLoaded) {
			if (!stricmp(name, g_drivers[i].name)) {
				return true;
			}
		}
	}
	return false;
}

static SemaphoreHandle_t g_mutex = 0;

bool DRV_Mutex_Take(int del) {
	int taken;

	if (g_mutex == 0)
	{
		g_mutex = xSemaphoreCreateMutex();
	}
	taken = xSemaphoreTake(g_mutex, del);
	if (taken == pdTRUE) {
		return true;
	}
	return false;
}
void DRV_Mutex_Free() {
	xSemaphoreGive(g_mutex);
}
void DRV_OnEverySecond() {
	int i;

	if (DRV_Mutex_Take(100) == false) {
		return;
	}
	for (i = 0; i < g_numDrivers; i++) {
		if (g_drivers[i].bLoaded) {
			if (g_drivers[i].onEverySecond != 0) {
				g_drivers[i].onEverySecond();
			}
		}
	}
#ifndef OBK_DISABLE_ALL_DRIVERS
	// unconditionally run TIME
	TIME_OnEverySecond();
#endif
	DRV_Mutex_Free();
}
void DRV_RunQuickTick() {
	int i;

	if (DRV_Mutex_Take(0) == false) {
		return;
	}
	for (i = 0; i < g_numDrivers; i++) {
		if (g_drivers[i].bLoaded) {
			if (g_drivers[i].runQuickTick != 0) {
				g_drivers[i].runQuickTick();
			}
		}
	}
	DRV_Mutex_Free();
}
void DRV_OnChannelChanged(int channel, int iVal) {
	int i;

	//if(DRV_Mutex_Take(100)==false) {
	//	return;
	//}
	for (i = 0; i < g_numDrivers; i++) {
		if (g_drivers[i].bLoaded) {
			if (g_drivers[i].onChannelChanged != 0) {
				g_drivers[i].onChannelChanged(channel, iVal);
			}
		}
	}
	//DRV_Mutex_Free();
}
// right now only used by simulator
void DRV_ShutdownAllDrivers() {
	int i;
	for (i = 0; i < g_numDrivers; i++) {
		if (g_drivers[i].bLoaded) {
			DRV_StopDriver(g_drivers[i].name);
		}
	}
}
void DRV_StopDriver(const char* name) {
	int i;

	if (DRV_Mutex_Take(100) == false) {
		return;
	}
	for (i = 0; i < g_numDrivers; i++) {
		if (*name == '*' || !stricmp(g_drivers[i].name, name)) {
			if (g_drivers[i].bLoaded) {
				if (g_drivers[i].stopFunc != 0) {
					g_drivers[i].stopFunc();
				}
				g_drivers[i].bLoaded = false;
				addLogAdv(LOG_INFO, LOG_FEATURE_MAIN, "Drv %s stopped.", g_drivers[i].name);
			}
			else {
				if (*name != '*') {
					addLogAdv(LOG_INFO, LOG_FEATURE_MAIN, "Drv %s not running.", name);
				}
			}
		}
	}
	DRV_Mutex_Free();
}
void DRV_StartDriver(const char* name) {
	int i;
	int bStarted;

	if (DRV_Mutex_Take(100) == false) {
		return;
	}
	bStarted = 0;
#if (ENABLE_DRIVER_DS1820) && (ENABLE_DRIVER_DS1820_FULL)
			bool twinrunning=false;
#endif
	for (i = 0; i < g_numDrivers; i++) {
		if (!stricmp(g_drivers[i].name, name)) {
#if (ENABLE_DRIVER_DS1820) && (ENABLE_DRIVER_DS1820_FULL)
			twinrunning=false;
			if (!stricmp("DS1820", name) && DRV_IsRunning("DS1820_FULL")){
				addLogAdv(LOG_ERROR, LOG_FEATURE_MAIN, "Drv DS1820_FULL is already loaded - can't start DS1820, too.\n", name);
				twinrunning=true;
				break;
			}
			if (!stricmp("DS1820_FULL", name) && DRV_IsRunning("DS1820")){
				addLogAdv(LOG_ERROR, LOG_FEATURE_MAIN, "Drv DS1820 is already loaded - can't start DS1820_FULL, too.\n", name);
				twinrunning=true;
				break;
			}
#endif
			if (g_drivers[i].bLoaded) {
				addLogAdv(LOG_INFO, LOG_FEATURE_MAIN, "Drv %s is already loaded.\n", name);
				bStarted = 1;
				break;

			}
			else {
				if (g_drivers[i].initFunc) {
					g_drivers[i].initFunc();
				}
				g_drivers[i].bLoaded = true;
				addLogAdv(LOG_INFO, LOG_FEATURE_MAIN, "Started %s.\n", name);
				bStarted = 1;
				break;
			}
		}
	}
#if (ENABLE_DRIVER_DS1820) && (ENABLE_DRIVER_DS1820_FULL)
	if (!bStarted && !twinrunning) {
#else
	if (!bStarted) {
#endif
		addLogAdv(LOG_INFO, LOG_FEATURE_MAIN, "Driver %s is not known in this build.\n", name);
		addLogAdv(LOG_INFO, LOG_FEATURE_MAIN, "Available drivers: ");
		for (i = 0; i < g_numDrivers; i++) {
			if (i == 0) {
				addLogAdv(LOG_INFO, LOG_FEATURE_MAIN, "%s", g_drivers[i].name);
			}
			else {
				addLogAdv(LOG_INFO, LOG_FEATURE_MAIN, ", %s", g_drivers[i].name);
			}
		}
	}
	DRV_Mutex_Free();
}
// startDriver DGR
// startDriver BL0942
// startDriver BL0937
static commandResult_t DRV_Start(const void* context, const char* cmd, const char* args, int cmdFlags) {
	Tokenizer_TokenizeString(args, 0);
	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	DRV_StartDriver(Tokenizer_GetArg(0));
	return CMD_RES_OK;
}
static commandResult_t DRV_Stop(const void* context, const char* cmd, const char* args, int cmdFlags) {
	Tokenizer_TokenizeString(args, 0);

	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}

	DRV_StopDriver(Tokenizer_GetArg(0));
	return CMD_RES_OK;
}

void DRV_Generic_Init() {
	//cmddetail:{"name":"startDriver","args":"[DriverName]",
	//cmddetail:"descr":"Starts driver",
	//cmddetail:"fn":"DRV_Start","file":"driver/drv_main.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("startDriver", DRV_Start, NULL);
	//cmddetail:{"name":"stopDriver","args":"[DriverName]",
	//cmddetail:"descr":"Stops driver",
	//cmddetail:"fn":"DRV_Stop","file":"driver/drv_main.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("stopDriver", DRV_Stop, NULL);
#ifndef OBK_DISABLE_ALL_DRIVERS
	// init TIME unconditionally on start
	TIME_Init();
#endif
}

void DRV_OnHassDiscovery(const char *topic) {
	int i;

	if (DRV_Mutex_Take(100) == false) {
		return;
	}
	for (i = 0; i < g_numDrivers; i++) {
		if (g_drivers[i].bLoaded) {
			if (g_drivers[i].onHassDiscovery) {
				g_drivers[i].onHassDiscovery(topic);
			}
		}
	}
	DRV_Mutex_Free();

}
void DRV_AppendInformationToHTTPIndexPage(http_request_t* request, int bPreState) {
	int i, j;
	int c_active = 0;

	if (DRV_Mutex_Take(100) == false) {
		return;
	}
#ifndef OBK_DISABLE_ALL_DRIVERS
	TIME_AppendInformationToHTTPIndexPage(request, bPreState);
#endif
	for (i = 0; i < g_numDrivers; i++) {
		if (g_drivers[i].bLoaded) {
			c_active++;
			if (g_drivers[i].appendInformationToHTTPIndexPage) {
				g_drivers[i].appendInformationToHTTPIndexPage(request, bPreState);
			}
		}
	}
	DRV_Mutex_Free();

	if (bPreState == false) {
		hprintf255(request, "<h5>%i drivers active", c_active);
		if (c_active > 0) {
			j = 0;// printed 0 names so far
			// generate active drivers list in (  )
			hprintf255(request, " (");
			for (i = 0; i < g_numDrivers; i++) {
				if (g_drivers[i].bLoaded) {
					// if at least one name printed, add separator
					if (j != 0) {
						hprintf255(request, ", ");
					}
					hprintf255(request, g_drivers[i].name);
					// one more name printed
					j++;
				}
			}
			hprintf255(request, ")");
		}
		hprintf255(request, ", total: %i</h5>", g_numDrivers);
	}
}

bool DRV_IsMeasuringPower() {
#ifndef OBK_DISABLE_ALL_DRIVERS
	return DRV_IsRunning("BL0937") || DRV_IsRunning("BL0942")
		|| DRV_IsRunning("CSE7766") || DRV_IsRunning("TESTPOWER")
		|| DRV_IsRunning("BL0942SPI") || DRV_IsRunning("RN8209");
		// || DRV_IsRunning("HLW8112SPI"); TODO messup ha config if enabled
#else
	return false;
#endif
}
bool DRV_IsMeasuringBattery() {
#ifndef OBK_DISABLE_ALL_DRIVERS
	return DRV_IsRunning("Battery");
#else
	return false;
#endif
}

bool DRV_IsSensor() {
#ifndef OBK_DISABLE_ALL_DRIVERS
	return DRV_IsRunning("SHT3X") || DRV_IsRunning("CHT83XX") || DRV_IsRunning("SGP") || DRV_IsRunning("AHT2X") || DRV_IsRunning("DS1820") || DRV_IsRunning("DS1820_full");
#else
	return false;
#endif
}
