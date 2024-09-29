#include "../i2c/drv_i2c_public.h"
#include "../logging/logging.h"
#include "drv_bl0937.h"
#include "drv_bl0942.h"
#include "drv_bl_shared.h"
#include "drv_cse7766.h"
#include "drv_ir.h"
#include "drv_local.h"
#include "drv_ntp.h"
#include "drv_public.h"
#include "drv_ssdp.h"
#include "drv_test_drivers.h"
#include "drv_tuyaMCU.h"
#include "drv_uart.h"
#include "drv_ds1820_simple.h"


typedef struct driver_s {
	const char* name;
	void(*initFunc)();
	void(*onEverySecond)();
	void(*appendInformationToHTTPIndexPage)(http_request_t* request);
	void(*runQuickTick)();
	void(*stopFunc)();
	void(*onChannelChanged)(int ch, int val);
	bool bLoaded;
} driver_t;


void TuyaMCU_RunEverySecond();

// startDriver BL0937
static driver_t g_drivers[] = {
#if ENABLE_DRIVER_TUYAMCU
	//drvdetail:{"name":"TuyaMCU",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"TuyaMCU is a protocol used for communication between WiFI module and external MCU. This protocol is using usually RX1/TX1 port of BK chips. See [TuyaMCU dimmer example](https://www.elektroda.com/rtvforum/topic3929151.html), see [TH06 LCD humidity/temperature sensor example](https://www.elektroda.com/rtvforum/topic3942730.html), see [fan controller example](https://www.elektroda.com/rtvforum/topic3908093.html), see [simple switch example](https://www.elektroda.com/rtvforum/topic3906443.html)",
	//drvdetail:"requires":""}
	{ "TuyaMCU",	TuyaMCU_Init,		TuyaMCU_RunEverySecond,			NULL, TuyaMCU_RunFrame, NULL, NULL, false },
	//drvdetail:{"name":"tmSensor",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"The tmSensor must be used only when TuyaMCU is already started. tmSensor is a TuyaMcu Sensor, it's used for Low Power TuyaMCU communication on devices like TuyaMCU door sensor, or TuyaMCU humidity sensor. After device reboots, tmSensor uses TuyaMCU to request data update from the sensor and reports it on MQTT. Then MCU turns off WiFi module again and goes back to sleep. See an [example door sensor here](https://www.elektroda.com/rtvforum/topic3914412.html).",
	//drvdetail:"requires":""}
	{ "tmSensor",	TuyaMCU_Sensor_Init, TuyaMCU_Sensor_RunEverySecond,	NULL, NULL, NULL, NULL, false },
#endif
#if ENABLE_DRIVER_FREEZE
	//drvdetail:{"name":"FREEZE",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"Freeze is a test driver for watchdog. Enabling this will freeze device main loop.",
	//drvdetail:"requires":""}
	{ "Freeze",		Freeze_Init,			Freeze_OnEverySecond,			NULL, Freeze_RunFrame, NULL, NULL, false },
#endif
#if ENABLE_DRIVER_PIXELANIM
	//drvdetail:{"name":"PixelAnim",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"PixelAnim provides a simple set of WS2812B animations",
	//drvdetail:"requires":""}
	{ "PixelAnim",		PixelAnim_Init,			NULL,			NULL, PixelAnim_SetAnimQuickTick, NULL, NULL, false },
#endif
#if ENABLE_DRIVER_DRAWERS
	//drvdetail:{"name":"Drawers",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"WS2812B driver wrapper with REST API for [smart drawers project](https://www.elektroda.com/rtvforum/topic4054134.html)",
	//drvdetail:"requires":""}
	{ "Drawers",		Drawers_Init,			NULL,			NULL, Drawers_QuickTick, NULL, NULL, false },
#endif
#if ENABLE_DRIVER_HGS02
	//drvdetail:{"name":"HGS02",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"[HGS02](https://www.elektroda.com/rtvforum/viewtopic.php?p=21177061#21177061)",
	//drvdetail:"requires":""}
	{ "HGS02",		HGS02_Init,			HGS02_RunEverySecond,			NULL, NULL, NULL, NULL, false },
#endif

#if WINDOWS
	//drvdetail:{"name":"TestCharts",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"Development only driver - a sample of chart generation with chart.js.",
	//drvdetail:"requires":""}
	{ "TestCharts",		NULL,			NULL,			DRV_Test_Charts_AddToHtmlPage, NULL, NULL, NULL, false },
#endif
#if ENABLE_DRIVER_CHARTS
	//drvdetail:{"name":"Charts",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"Charts driver allows you to create a customizable chart directly on your device. See [tutorial](https://www.elektroda.com/rtvforum/topic4075289.html).",
	//drvdetail:"requires":""}
	{ "Charts",		DRV_Charts_Init,			NULL,			DRV_Charts_AddToHtmlPage, NULL, NULL, NULL, false },
#endif
#if ENABLE_NTP
	//drvdetail:{"name":"NTP",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"NTP driver is required to get current time and date from web. Without it, there is no correct datetime. Put 'startDriver NTP' in short startup line or autoexec.bat to run it on start.",
	//drvdetail:"requires":""}
	{ "NTP",		NTP_Init,			NTP_OnEverySecond,			NTP_AppendInformationToHTTPIndexPage, NULL, NULL, NULL, false },
#endif
#if ENABLE_HTTPBUTTONS
	//drvdetail:{"name":"HTTPButtons",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"This driver allows you to create custom, scriptable buttons on main WWW page. You can create those buttons in autoexec.bat and assign commands to them",
	//drvdetail:"requires":""}
	{ "HTTPButtons",	DRV_InitHTTPButtons, NULL, NULL, NULL, NULL, NULL, false },
#endif
#if ENABLE_TEST_DRIVERS
	//drvdetail:{"name":"TESTPOWER",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"This is a fake POWER measuring socket driver, only for testing",
	//drvdetail:"requires":""}
	{ "TESTPOWER",	Test_Power_Init,	 Test_Power_RunEverySecond,		BL09XX_AppendInformationToHTTPIndexPage, NULL, NULL, NULL, false },
	//drvdetail:{"name":"TESTLED",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"This is a fake I2C LED driver, only for testing",
	//drvdetail:"requires":""}
	{ "TESTLED",	Test_LED_Driver_Init, Test_LED_Driver_RunEverySecond, NULL, NULL, NULL, Test_LED_Driver_OnChannelChanged, false },
#endif
#if ENABLE_I2C
	//drvdetail:{"name":"I2C",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"Generic I2C, not used for LED drivers, but may be useful for displays or port expanders. Supports both hardware and software I2C.",
	//drvdetail:"requires":""}
	{ "I2C",		DRV_I2C_Init,		DRV_I2C_EverySecond,		NULL, NULL, NULL, NULL, false },
#endif
#if ENABLE_DRIVER_BL0942
	//drvdetail:{"name":"RN8209",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"Bqqqqqqqqqq ",
	//drvdetail:"requires":""}
	{ "RN8209", RN8209_Init, RN8029_RunEverySecond, BL09XX_AppendInformationToHTTPIndexPage, NULL, NULL, NULL, false },
#endif
#if ENABLE_DRIVER_BL0942
	//drvdetail:{"name":"BL0942",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"BL0942 is a power-metering chip which uses UART protocol for communication. It's usually connected to TX1/RX1 port of BK. You need to calibrate power metering once, just like in Tasmota. See [LSPA9 teardown example](https://www.elektroda.com/rtvforum/topic3887748.html). By default, it uses 4800 baud, but you can also enable it with baud 9600 by using 'startDriver BL0942 9600', see [related topic](https://www.elektroda.com/rtvforum/viewtopic.php?p=20957896#20957896)",
	//drvdetail:"requires":""}
	{ "BL0942",		BL0942_UART_Init,	BL0942_UART_RunEverySecond,		BL09XX_AppendInformationToHTTPIndexPage, NULL, NULL, NULL, false },
#endif
#if ENABLE_DRIVER_PWM_GROUP
	//drvdetail:{"name":"PWMG",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":" ",
	//drvdetail:"requires":""}
	{ "PWMG",		PWMG_Init,	NULL,		NULL, NULL, NULL, NULL, false },
#endif
#if ENABLE_DRIVER_BL0942SPI
	//drvdetail:{"name":"BL0942SPI",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"BL0942 driver version for SPI protocol. It's usually connected to SPI1 port of BK. You need to calibrate power metering once, just like in Tasmota. See [PZIOT-E01 teardown example](https://www.elektroda.com/rtvforum/topic3945667.html). ",
	//drvdetail:"requires":""}
	{ "BL0942SPI",	BL0942_SPI_Init,	BL0942_SPI_RunEverySecond,		BL09XX_AppendInformationToHTTPIndexPage, NULL, NULL, NULL, false },
#endif
#if ENABLE_DRIVER_CHARGINGLIMIT
	//drvdetail:{"name":"ChargingLimit",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"Mechanism to perform an action based on a max. delta value and max time. Used to control Electric Vehicle chargers. See [discussion](https://github.com/openshwprojects/OpenBK7231T_App/issues/892).",
	//drvdetail:"requires":""}
	{ "ChargingLimit", ChargingLimit_Init, ChargingLimit_OnEverySecond, ChargingLimit_AppendInformationToHTTPIndexPage, NULL, NULL, NULL, false },
#endif
#if ENABLE_DRIVER_BL0937
	//drvdetail:{"name":"BL0937",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"BL0937 is a power-metering chip which uses custom protocol to report data. It requires setting 3 pins in pin config: CF, CF1 and SEL",
	//drvdetail:"requires":""}
	{ "BL0937",		BL0937_Init,		BL0937_RunEverySecond,			BL09XX_AppendInformationToHTTPIndexPage, NULL, NULL, NULL, false },
#endif
#if ENABLE_DRIVER_CSE7766
	//drvdetail:{"name":"CSE7766",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"CSE7766 is a power-metering chip which uses UART protocol for communication. It's usually connected to TX1/RX1 port of BK",
	//drvdetail:"requires":""}
	{ "CSE7766",	CSE7766_Init,		CSE7766_RunEverySecond,			BL09XX_AppendInformationToHTTPIndexPage, NULL, NULL, NULL, false },
#endif
#if ENABLE_DRIVER_MAX6675
	//drvdetail:{"name":"MAX6675",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"Thermocouple driver for measuring high temperatures, see [presentation](https://www.elektroda.com/rtvforum/topic4055231.html)",
	//drvdetail:"requires":""}
	{ "MAX6675",	MAX6675_Init,		MAX6675_RunEverySecond,			NULL, NULL, NULL, NULL, false },
#endif
#if ENABLE_DRIVER_PT6523
	//drvdetail:{"name":"PT6523",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"Car radio LCD driver, see [teardown and presentation](https://www.elektroda.com/rtvforum/topic3983111.html)",
	//drvdetail:"requires":""}
	{ "PT6523",	PT6523_Init,		PT6523_RunFrame,			NULL, NULL, NULL, NULL, false },
#endif
#if ENABLE_DRIVER_TEXTSCROLLER
	//drvdetail:{"name":"TextScroller",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"BQQQK",
	//drvdetail:"requires":""}
	{ "TextScroller",	TS_Init,		NULL,			NULL, TS_RunQuickTick, NULL, NULL, false },
#endif
#if ENABLE_DRIVER_SM16703P
	//drvdetail:{"name":"SM16703P",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"SM16703P is an individually addressable LEDs controller like WS2812B. Currently SM16703P LEDs are supported through hardware SPI, LEDs data should be connected to P16 (MOSI), [here you can read](https://www.elektroda.com/rtvforum/topic4005865.html) how to break it out on CB2S.",
	//drvdetail:"requires":""}
	{ "SM16703P",	SM16703P_Init,		NULL,						NULL, NULL, NULL, NULL, false },
#endif
#if ENABLE_DRIVER_SM15155E
	//drvdetail:{"name":"SM15155E",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"SM15155E is a WS2812B-like single wire LED controller. It's also always using P16 (SPI out) on Beken. See [reverse-engineering topic](https://www.elektroda.com/rtvforum/topic4060227.html)",
	//drvdetail:"requires":""}
	{ "SM15155E",	SM15155E_Init,		NULL,						NULL, NULL, NULL, NULL, false },
#endif
#if PLATFORM_BEKEN
	//drvdetail:{"name":"IR",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"IRLibrary wrapper, so you can receive remote signals and send them. See [forum discussion here](https://www.elektroda.com/rtvforum/topic3920360.html), also see [LED strip and IR YT video](https://www.youtube.com/watch?v=KU0tDwtjfjw)",
	//drvdetail:"requires":""}
	{ "IR",			DRV_IR_Init,		 NULL,						NULL, DRV_IR_RunFrame, NULL, NULL, false },
#endif
#if ENABLE_DRIVER_IR2
	{ "IR2",			DRV_IR2_Init,		 NULL,						NULL, NULL, NULL, NULL, false },
#endif
		
#if ENABLE_DRIVER_DDP
	//drvdetail:{"name":"DDP",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"DDP is a LED control protocol that is using UDP. You can use xLights or any other app to control OBK LEDs that way. See [DDP topic](https://www.elektroda.com/rtvforum/topic4040325.html)",
	//drvdetail:"requires":""}
	{ "DDP",		DRV_DDP_Init,		NULL,						DRV_DDP_AppendInformationToHTTPIndexPage, DRV_DDP_RunFrame, DRV_DDP_Shutdown, NULL, false },
#endif
#if ENABLE_DRIVER_SSDP
	//drvdetail:{"name":"SSDP",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"SSDP is a discovery protocol, so BK devices can show up in, for example, Windows network section",
	//drvdetail:"requires":""}
	{ "SSDP",		DRV_SSDP_Init,		DRV_SSDP_RunEverySecond,	NULL, DRV_SSDP_RunQuickTick, DRV_SSDP_Shutdown, NULL, false },
#endif
#if ENABLE_TASMOTADEVICEGROUPS
	//drvdetail:{"name":"DGR",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"Tasmota Device groups driver. See [forum example](https://www.elektroda.com/rtvforum/topic3925472.html) and [video tutorial](https://www.youtube.com/watch?v=e1xcq3OUR5M&ab_channel=Elektrodacom)",
	//drvdetail:"requires":""}
	{ "DGR",		DRV_DGR_Init,		DRV_DGR_RunEverySecond,		DRV_DGR_AppendInformationToHTTPIndexPage, DRV_DGR_RunQuickTick, DRV_DGR_Shutdown, DRV_DGR_OnChannelChanged, false },
#endif
#if ENABLE_DRIVER_WEMO
	//drvdetail:{"name":"Wemo",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"Wemo emulation for Alexa. You must also start SSDP so it can run, because it depends on SSDP discovery.",
	//drvdetail:"requires":""}
	{ "Wemo",		WEMO_Init,		NULL,		WEMO_AppendInformationToHTTPIndexPage, NULL, NULL, NULL, false },
#endif
#if ENABLE_DRIVER_HUE
	//drvdetail:{"name":"Hue",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"Hue emulation for Alexa. You must also start SSDP so it can run, because it depends on SSDP discovery.",
	//drvdetail:"requires":""}
	{ "Hue",		HUE_Init,		NULL,		HUE_AppendInformationToHTTPIndexPage, NULL, NULL, NULL, false },
#endif
#if defined(PLATFORM_BEKEN) || defined(WINDOWS)
	//drvdetail:{"name":"PWMToggler",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"PWMToggler is a custom abstraction layer that can run on top of raw PWM channels. It provides ability to turn off/on the PWM while keeping it's value, which is not possible by direct channel operations. It can be used for some custom devices with extra lights/lasers. See example [here](https://www.elektroda.com/rtvforum/topic3939064.html).",
	//drvdetail:"requires":""}
	{ "PWMToggler",	DRV_InitPWMToggler, NULL, DRV_Toggler_AppendInformationToHTTPIndexPage, DRV_Toggler_QuickTick, NULL, NULL, false },
	//drvdetail:{"name":"DoorSensor",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"DoorSensor is using deep sleep to preserve battery. This is used for devices without TuyaMCU, where BK deep sleep and wakeup on GPIO is used. This drives requires you to set a DoorSensor pin. Change on door sensor pin wakes up the device. If there are no changes for some time, device goes to sleep. See example [here](https://www.elektroda.com/rtvforum/topic3960149.html). If your door sensor does not wake up in certain pos, please use DSEdge command (try all 3 options, default is 2). ",
	//drvdetail:"requires":""}
	{ "DoorSensor",		DoorDeepSleep_Init,		DoorDeepSleep_OnEverySecond,	DoorDeepSleep_AppendInformationToHTTPIndexPage, NULL, NULL, DoorDeepSleep_OnChannelChanged, false },
#endif
#if ENABLE_DRIVER_ADCBUTTON
	//drvdetail:{"name":"ADCButton",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"This allows you to connect multiple buttons on single ADC pin. Each button must have a different resistor value, this works by probing the voltage on ADC from a resistor divider. You need to select AB_Map first. See forum post for [details](https://www.elektroda.com/rtvforum/viewtopic.php?p=20541973#20541973).",
	//drvdetail:"requires":""}
	{ "ADCButton",		DRV_ADCButton_Init,		NULL,	NULL, DRV_ADCButton_RunFrame, NULL, NULL, false },
#endif
#if ENABLE_DRIVER_MAX72XX
	//drvdetail:{"name":"MAX72XX_Clock",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"Simple hardcoded driver for MAX72XX clock. Requires manual start of MAX72XX driver with MAX72XX setup and NTP start.",
	//drvdetail:"requires":""}
	{ "MAX72XX_Clock",		DRV_MAX72XX_Clock_Init,		DRV_MAX72XX_Clock_OnEverySecond,	NULL, DRV_MAX72XX_Clock_RunFrame, NULL, NULL, false },
#endif
#if ENABLE_DRIVER_LED
	//drvdetail:{"name":"SM2135",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"SM2135 custom-'I2C' LED driver for RGBCW lights. This will start automatically if you set both SM2135 pin roles. This may need you to remap the RGBCW indexes with SM2135_Map command",
	//drvdetail:"requires":""}
	{ "SM2135",		SM2135_Init,		NULL,			NULL, NULL, NULL, NULL, false },
	//drvdetail:{"name":"BP5758D",
	//drvdetail:"title":"TODO",	
	//drvdetail:"descr":"BP5758D custom-'I2C' LED driver for RGBCW lights. This will start automatically if you set both BP5758D pin roles. This may need you to remap the RGBCW indexes with BP5758D_Map command. This driver is used in some of BL602/Sonoff bulbs, see [video flashing tutorial here](https://www.youtube.com/watch?v=L6d42IMGhHw)",
	//drvdetail:"requires":""}
	{ "BP5758D",	BP5758D_Init,		NULL,			NULL, NULL, NULL, NULL, false },
	//drvdetail:{"name":"BP1658CJ",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"BP1658CJ custom-'I2C' LED driver for RGBCW lights. This will start automatically if you set both BP1658CJ pin roles. This may need you to remap the RGBCW indexes with BP1658CJ_Map command",
	//drvdetail:"requires":""}
	{ "BP1658CJ",	BP1658CJ_Init,		NULL,			NULL, NULL, NULL, NULL, false },
	//drvdetail:{"name":"SM2235",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"SM2335 andd SM2235 custom-'I2C' LED driver for RGBCW lights. This will start automatically if you set both SM2235 pin roles. This may need you to remap the RGBCW indexes with SM2235_Map command. This driver also works for SM2185N.",
	//drvdetail:"requires":""}
	{ "SM2235",		SM2235_Init,		NULL,			NULL, NULL, NULL, NULL, false },
#endif

#if ENABLE_DRIVER_BMP280
	//drvdetail:{"name":"BMP280",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"BMP280 is a Temperature and Pressure sensor with I2C interface.",
	//drvdetail:"requires":""}
	{ "BMP280", BMP280_Init, BMP280_OnEverySecond, BMP280_AppendInformationToHTTPIndexPage, NULL, NULL, NULL, false },
#endif
#if ENABLE_DRIVER_MAX72XX
	//drvdetail:{"name":"MAX72XX",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"MAX72XX LED matrix display driver with font and simple script interface. See [protocol explanation](https://www.elektroda.pl/rtvforum/viewtopic.php?p=18040628#18040628)",
	//drvdetail:"requires":""}
	{ "MAX72XX",	DRV_MAX72XX_Init,		NULL,		NULL, NULL, NULL, NULL, false },
#endif
#if ENABLE_DRIVER_BMPI2C
		//drvdetail:{"name":"BMPI2C",
		//drvdetail:"title":"TODO",
		//drvdetail:"descr":"Driver for BMP085, BMP180, BMP280, BME280, BME68X sensors with I2C interface.",
		//drvdetail:"requires":""}
	{ "BMPI2C", BMPI2C_Init, BMPI2C_OnEverySecond, BMPI2C_AppendInformationToHTTPIndexPage, NULL, NULL, NULL, false },
#endif
#if ENABLE_DRIVER_CHT83XX
	//drvdetail:{"name":"CHT83XX",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"CHT8305, CHT8310 and CHT8315 are a Temperature and Humidity sensors with I2C interface.",
	//drvdetail:"requires":""}
	{ "CHT83XX",	CHT83XX_Init,		CHT83XX_OnEverySecond,		CHT83XX_AppendInformationToHTTPIndexPage, NULL, NULL, NULL, false },
#endif
#if defined(PLATFORM_BEKEN) || defined(WINDOWS) || defined(PLATFORM_ESPIDF)
	//drvdetail:{"name":"MCP9808",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"MCP9808 is a Temperature sensor with I2C interface and an external wakeup pin, see [docs](https://www.elektroda.pl/rtvforum/topic3988466.html).",
	//drvdetail:"requires":""}
	{ "MCP9808",	MCP9808_Init,		MCP9808_OnEverySecond,		MCP9808_AppendInformationToHTTPIndexPage, NULL, NULL, NULL, false },
	//drvdetail:{"name":"KP18058",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"KP18058 I2C LED driver. Supports also KP18068. Working, see reverse-engineering [topic](https://www.elektroda.pl/rtvforum/topic3991620.html)",
	//drvdetail:"requires":""}
	{ "KP18058",		KP18058_Init,		NULL,			NULL, NULL, NULL, NULL, false },
	//drvdetail:{"name":"ADCSmoother",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"qq",
	//drvdetail:"requires":""}
	{ "ADCSmoother", DRV_ADCSmoother_Init, NULL, NULL, DRV_ADCSmoother_RunFrame, NULL, NULL, false },
	//drvdetail:{"name":"SHT3X",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"Humidity/temperature sensor. See [SHT Sensor tutorial topic here](https://www.elektroda.com/rtvforum/topic3958369.html), also see [this sensor teardown](https://www.elektroda.com/rtvforum/topic3945688.html)",
	//drvdetail:"requires":""}
	{ "SHT3X",	    SHT3X_Init,		SHT3X_OnEverySecond,		SHT3X_AppendInformationToHTTPIndexPage, NULL, SHT3X_StopDriver, NULL, false },
	//drvdetail:{"name":"SGP",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"SGP Air Quality sensor with I2C interface. See [this DIY sensor](https://www.elektroda.com/rtvforum/topic3967174.html) for setup information.",
	//drvdetail:"requires":""}
	{ "SGP",	    SGP_Init,		SGP_OnEverySecond,		SGP_AppendInformationToHTTPIndexPage, NULL, SGP_StopDriver, NULL, false },
	//drvdetail:{"name":"ShiftRegister",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"Simple Shift Register driver that allows you to map channels to shift register output. See [related topic](https://www.elektroda.com/rtvforum/viewtopic.php?p=20533505#20533505)",
	//drvdetail:"requires":""}
	{ "ShiftRegister",	    Shift_Init,		Shift_OnEverySecond,		NULL, NULL, NULL, Shift_OnChannelChanged, false },
#endif
#if ENABLE_DRIVER_AHT2X
	//drvdetail:{"name":"AHT2X",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"AHT Humidity/temperature sensor. Supported sensors are: AHT10, AHT2X, AHT30. See [presentation guide](https://www.elektroda.com/rtvforum/topic4052685.html)",
	//drvdetail:"requires":""}
	{ "AHT2X",	AHT2X_Init,	AHT2X_OnEverySecond,	AHT2X_AppendInformationToHTTPIndexPage,	NULL,	AHT2X_StopDriver,	NULL,	false },
#endif
#if ENABLE_DRIVER_DS1820
	//drvdetail:{"name":"DS1820",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"Very simple driver for oneWire temperature sensor DS1820.",
	//drvdetail:"requires":""}
	{ "DS1820",     DS1820_driver_Init, DS1820_OnEverySecond,                       DS1820_AppendInformationToHTTPIndexPage, NULL, NULL, NULL,  false },
#endif
#if ENABLE_DRIVER_HT16K33
	//drvdetail:{"name":"HT16K33",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"Driver for 16-segment LED display with I2C. See [protocol explanation](https://www.elektroda.pl/rtvforum/topic3984616.html)",
	//drvdetail:"requires":""}
	{ "HT16K33",	HT16K33_Init,		NULL,		NULL, NULL,NULL,  NULL, false },
#endif
	// Shared driver for TM1637, GN6932, TM1638 - TM_GN_Display_SharedInit
#if ENABLE_DRIVER_TMGN
	//drvdetail:{"name":"TM1637",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"Driver for 7-segment LED display with DIO/CLK interface. See [TM1637 information](https://www.elektroda.com/rtvforum/viewtopic.php?p=20468593#20468593)",
	//drvdetail:"requires":""}
	{ "TM1637",	TM1637_Init,		NULL,		NULL,  TMGN_RunQuickTick,NULL, NULL, false },
	//drvdetail:{"name":"GN6932",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"Driver for 7-segment LED display with DIO/CLK/STB interface. See [this topic](https://www.elektroda.com/rtvforum/topic3971252.html) for details.",
	//drvdetail:"requires":""}
	{ "GN6932",	GN6932_Init,		NULL,		NULL, TMGN_RunQuickTick, NULL, NULL, false },
	//drvdetail:{"name":"TM1638",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"Driver for 7-segment LED display with DIO/CLK/STB interface. TM1638 is very similiar to GN6932 and TM1637. See [this topic](https://www.elektroda.com/rtvforum/viewtopic.php?p=20553628#20553628) for details.",
	//drvdetail:"requires":""}
	{ "TM1638",	TM1638_Init,		NULL,		NULL, TMGN_RunQuickTick,NULL,  NULL, false },
	//drvdetail:{"name":"HD2015",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"Driver for 7-segment LED display with I2C-like interface. Seems to be compatible with TM1650. HD2015 is very similiar to GN6932 and TM1637. See [this topic](https://www.elektroda.com/rtvforum/topic4052946.html) for details.",
	//drvdetail:"requires":""}
	{ "HD2015",	HD2015_Init,		NULL,		NULL, TMGN_RunQuickTick,NULL,  NULL, false },
#endif
#if ENABLE_DRIVER_BATTERY
	//drvdetail:{"name":"Battery",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"Custom mechanism to measure battery level with ADC and an optional relay. See [example here](https://www.elektroda.com/rtvforum/topic3959103.html).",
	//drvdetail:"requires":""}
	{ "Battery",	Batt_Init,		Batt_OnEverySecond,		Batt_AppendInformationToHTTPIndexPage, NULL, Batt_StopDriver, NULL, false },
#endif
#if ENABLE_DRIVER_BRIDGE
	//drvdetail:{"name":"Bridge",
	//drvdetail:"title":"TODO",
	//drvdetail:"descr":"A bridge relay driver, added for [TONGOU TO-Q-SY1-JWT Din Rail Switch](https://www.elektroda.com/rtvforum/topic3934580.html). See linked topic for info.",
	//drvdetail:"requires":""}
	{ "Bridge",     Bridge_driver_Init, NULL,                       NULL, Bridge_driver_QuickFrame, Bridge_driver_DeInit, Bridge_driver_OnChannelChanged, false }
#endif
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
	for (i = 0; i < g_numDrivers; i++) {
		if (!stricmp(g_drivers[i].name, name)) {
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
	if (!bStarted) {
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
}
void DRV_AppendInformationToHTTPIndexPage(http_request_t* request) {
	int i, j;
	int c_active = 0;

	if (DRV_Mutex_Take(100) == false) {
		return;
	}
	for (i = 0; i < g_numDrivers; i++) {
		if (g_drivers[i].bLoaded) {
			c_active++;
			if (g_drivers[i].appendInformationToHTTPIndexPage) {
				g_drivers[i].appendInformationToHTTPIndexPage(request);
			}
		}
	}
	DRV_Mutex_Free();

	hprintf255(request, "<h5>%i drivers active", c_active);
	if (c_active > 0) {
		j = 0;// printed 0 names so far
		// generate active drivers list in (  )
		hprintf255(request, " (");
		for (i = 0; i < g_numDrivers; i++) {
			if (g_drivers[i].bLoaded) {
				// if at least one name printed, add separator
				if (j != 0) {
					hprintf255(request, ",");
				}
				hprintf255(request, g_drivers[i].name);
				// one more name printed
				j++;
			}
		}
		hprintf255(request, ")");
	}
	hprintf255(request, ", total %i</h5>", g_numDrivers);
}

bool DRV_IsMeasuringPower() {
#ifndef OBK_DISABLE_ALL_DRIVERS
	return DRV_IsRunning("BL0937") || DRV_IsRunning("BL0942")
		|| DRV_IsRunning("CSE7766") || DRV_IsRunning("TESTPOWER")
		|| DRV_IsRunning("BL0942SPI") || DRV_IsRunning("RN8209");
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
	return DRV_IsRunning("SHT3X") || DRV_IsRunning("CHT83XX") || DRV_IsRunning("SGP") || DRV_IsRunning("AHT2X");
#else
	return false;
#endif
}
