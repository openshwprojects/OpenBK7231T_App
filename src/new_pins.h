#ifndef __NEW_PINS_H__
#define __NEW_PINS_H__
#pragma once
#include "new_common.h"
#include "hal/hal_wifi.h"

#define test12321321  321321321

#define INCLUDED_BY_NEW_PINS_H 1
#include "rolesNchannels.h"
#undef INCLUDED_BY_NEW_PINS_H

#define IS_PIN_DHT_ROLE(role) (((role)>=IOR_DHT11) && ((role)<=IOR_DHT22))
#define IS_PIN_TEMP_HUM_SENSOR_ROLE(role) (((role)==IOR_SHT3X_DAT) || ((role)==IOR_CHT83XX_DAT))
#define IS_PIN_AIR_SENSOR_ROLE(role) (((role)==IOR_SGP_DAT))

typedef enum channelType_e {
	//chandetail:{"name":"Default",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"Default channel type. This channel type is often used in simple devices and is suitable for relays. You don't need to use anything else for most of the non-TuyaMCU devices.",
	//chandetail:"enum":"ChType_Default",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_Default,
	//chandetail:{"name":"Error",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"This is used to indicate an error.",
	//chandetail:"enum":"ChType_Error",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_Error,
	//chandetail:{"name":"Temperature",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"This channel type represents a temperature in degrees. The temperature is shown as a read only, integer value on WWW panel.",
	//chandetail:"enum":"ChType_Temperature",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_Temperature,
	//chandetail:{"name":"Humidity",
	//chandetail:"title":"TODO",	
	//chandetail:"descr":"This channel type represents a humidity in percent. The humidity is shown as a read only, integer value on WWW panel.",
	//chandetail:"enum":"ChType_Humidity",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_Humidity,
	//chandetail:{"name":"Humidity_div10",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"This is also humidity, but in TuyaMCU format, multiplied by 10, so 554 is 55.4%. Main WWW panel displays it correctly. If you want multiplied value to be published to MQTT, check flags.",
	//chandetail:"enum":"ChType_Humidity_div10",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_Humidity_div10,
	//chandetail:{"name":"Temperature_div10",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"Just like humidity_div10, but for temperature.",
	//chandetail:"enum":"ChType_Temperature_div10",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_Temperature_div10,
	//chandetail:{"name":"Toggle",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"This channel will show ON/OFF toggle, so it is very much like default channel type, but the toggle will be always shown, even if channel is not used",
	//chandetail:"enum":"ChType_Toggle",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_Toggle,
	//chandetail:{"name":"Dimmer",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"A custom dimmer channel. This will have a 0-100 range and will show a slider. The slider value will be saved to channel, but nothing else is done automatically. You can map it to TuyaMCU dpID to get dimming working or script it however you like.",
	//chandetail:"enum":"ChType_Dimmer",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_Dimmer,
	//chandetail:{"name":"LowMidHigh",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"This channel has 3 values: 0, 1 and 2. This will show radio selection with those 3 options on the main WWW panel.",
	//chandetail:"enum":"ChType_LowMidHigh",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_LowMidHigh,
	//chandetail:{"name":"TextField",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"This is a custom textfield channel, where you can type any value. Used for testing, can be also used for time countdown on TuyaMCU devices. Text field will be shown on main WWW panel",
	//chandetail:"enum":"ChType_TextField",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_TextField,
	//chandetail:{"name":"ReadOnly",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"This channel is read only. It will just print its value on main WWW page. Of course, you can still write to it with console commands and scripts.",
	//chandetail:"enum":"ChType_ReadOnly",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_ReadOnly,
	//chandetail:{"name":"OffLowMidHigh",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"Like LowMidHigh, but with 4 options. Some of TuyaMCU fans might require that.",
	//chandetail:"enum":"ChType_OffLowMidHigh",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_OffLowMidHigh,
	//chandetail:{"name":"OffLowestLowMidHighHighest",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"Like LowMidHigh, but more options. Some of TuyaMCU fans might require that.",
	//chandetail:"enum":"ChType_OffLowestLowMidHighHighest",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_OffLowestLowMidHighHighest,
	//chandetail:{"name":"LowestLowMidHighHighest",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"Like LowMidHigh, but more options. Some of TuyaMCU fans might require that.",
	//chandetail:"enum":"ChType_LowestLowMidHighHighest",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_LowestLowMidHighHighest,
	//chandetail:{"name":"Dimmer256",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"Just like dimmer, but it's using 0-255 range. Everything else is the same.",
	//chandetail:"enum":"ChType_Dimmer256",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_Dimmer256,
	//chandetail:{"name":"Dimmer1000",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"Just like dimmer, but it's using 0-1000 range. Everything else is the same.",
	//chandetail:"enum":"ChType_Dimmer1000",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_Dimmer1000,
	//chandetail:{"name":"Frequency_div100",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"For TuyaMCU power metering. Not used for BL09** and CSE** sensors. Divider is used by TuyaMCU, because TuyaMCU sends always values as integers so we have to divide them before displaying on UI",
	//chandetail:"enum":"ChType_Frequency_div100",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_Frequency_div100,
	//chandetail:{"name":"Voltage_div10",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"For TuyaMCU power metering. Not used for BL09** and CSE** sensors. Divider is used by TuyaMCU, because TuyaMCU sends always values as integers so we have to divide them before displaying on UI",
	//chandetail:"enum":"ChType_Voltage_div10",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_Voltage_div10,
	//chandetail:{"name":"Power",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"For TuyaMCU power metering. Not used for BL09** and CSE** sensors. Divider is used by TuyaMCU, because TuyaMCU sends always values as integers so we have to divide them before displaying on UI",
	//chandetail:"enum":"ChType_Power",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_Power,
	//chandetail:{"name":"Current_div100",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"For TuyaMCU power metering. Not used for BL09** and CSE** sensors. Divider is used by TuyaMCU, because TuyaMCU sends always values as integers so we have to divide them before displaying on UI",
	//chandetail:"enum":"ChType_Current_div100",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_Current_div100,
	//chandetail:{"name":"ActivePower",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"For TuyaMCU power metering. Not used for BL09** and CSE** sensors. Divider is used by TuyaMCU, because TuyaMCU sends always values as integers so we have to divide them before displaying on UI",
	//chandetail:"enum":"ChType_ActivePower",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_ActivePower,
	//chandetail:{"name":"PowerFactor_div1000",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"For TuyaMCU power metering. Not used for BL09** and CSE** sensors. Divider is used by TuyaMCU, because TuyaMCU sends always values as integers so we have to divide them before displaying on UI",
	//chandetail:"enum":"ChType_PowerFactor_div1000",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_PowerFactor_div1000,
	//chandetail:{"name":"ReactivePower",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"For TuyaMCU power metering. Not used for BL09** and CSE** sensors. Divider is used by TuyaMCU, because TuyaMCU sends always values as integers so we have to divide them before displaying on UI",
	//chandetail:"enum":"ChType_ReactivePower",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_ReactivePower,
	//chandetail:{"name":"EnergyTotal_kWh_div1000",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"For TuyaMCU power metering. Not used for BL09** and CSE** sensors. Divider is used by TuyaMCU, because TuyaMCU sends always values as integers so we have to divide them before displaying on UI",
	//chandetail:"enum":"ChType_EnergyTotal_kWh_div1000",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_EnergyTotal_kWh_div1000,
	//chandetail:{"name":"EnergyExport_kWh_div1000",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"For TuyaMCU power metering. Not used for BL09** and CSE** sensors. Divider is used by TuyaMCU, because TuyaMCU sends always values as integers so we have to divide them before displaying on UI",
	//chandetail:"enum":"ChType_EnergyExport_kWh_div1000",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_EnergyExport_kWh_div1000,
	//chandetail:{"name":"EnergyToday_kWh_div1000",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"For TuyaMCU power metering. Not used for BL09** and CSE** sensors. Divider is used by TuyaMCU, because TuyaMCU sends always values as integers so we have to divide them before displaying on UI",
	//chandetail:"enum":"ChType_EnergyToday_kWh_div1000",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_EnergyToday_kWh_div1000,
	//chandetail:{"name":"Current_div1000",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"For TuyaMCU power metering. Not used for BL09** and CSE** sensors. Divider is used by TuyaMCU, because TuyaMCU sends always values as integers so we have to divide them before displaying on UI",
	//chandetail:"enum":"ChType_Current_div1000",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_Current_div1000,
	//chandetail:{"name":"EnergyTotal_kWh_div100",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"For TuyaMCU power metering. Not used for BL09** and CSE** sensors. Divider is used by TuyaMCU, because TuyaMCU sends always values as integers so we have to divide them before displaying on UI",
	//chandetail:"enum":"ChType_EnergyTotal_kWh_div100",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_EnergyTotal_kWh_div100,
	//chandetail:{"name":"OpenClosed",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"This will show an 'Open' or 'Closed' string on main WWW panel. Useful for door sensors, etc.",
	//chandetail:"enum":"ChType_OpenClosed",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_OpenClosed,
	//chandetail:{"name":"OpenClosed_Inv",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"Like OpenClosed, but values are inversed.",
	//chandetail:"enum":"ChType_OpenClosed_Inv",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_OpenClosed_Inv,
	//chandetail:{"name":"BatteryLevelPercent",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"This will show current value as a battery level percent on the main WWW panel.",
	//chandetail:"enum":"ChType_BatteryLevelPercent",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_BatteryLevelPercent,
	//chandetail:{"name":"OffDimBright",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"A 3 options radio button for lighting control.",
	//chandetail:"enum":"ChType_OffDimBright",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_OffDimBright,
	//chandetail:{"name":"LowMidHighHighest",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"Like LowMidHigh, but with 4 options. Some of TuyaMCU fans might require that.",
	//chandetail:"enum":"ChType_LowMidHighHighest",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_LowMidHighHighest,
	//chandetail:{"name":"OffLowMidHighHighest",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"Like LowMidHigh, but with 5 options. Some of TuyaMCU fans might require that.",
	//chandetail:"enum":"ChType_OffLowMidHighHighest",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_OffLowMidHighHighest,
	//chandetail:{"name":"Custom",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"A custom channel type that is still send to HA.",
	//chandetail:"enum":"ChType_Custom",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_Custom,
	//chandetail:{"name":"Power_div10",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"Just like power, but with one decimal place (but stored as integer, for TuyaMCU support)",
	//chandetail:"enum":"ChType_Power_div10",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_Power_div10,
	//chandetail:{"name":"ReadOnlyLowMidHigh",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"Like LowMidHigh, but just read only",
	//chandetail:"enum":"ChType_ReadOnlyLowMidHigh",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_ReadOnlyLowMidHigh,
	//chandetail:{"name":"SmokePercent",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"Smoke percentage",
	//chandetail:"enum":"ChType_SmokePercent",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_SmokePercent,
	//chandetail:{"name":"Illuminance",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"Illuminance in Lux",
	//chandetail:"enum":"ChType_Illuminance",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_Illuminance,
	//chandetail:{"name":"Toggle_Inv",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"Like a Toggle, but inverted states.",
	//chandetail:"enum":"ChType_Toggle_Inv",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_Toggle_Inv,
	//chandetail:{"name":"OffOnRemember",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"Radio buttons with 3 options: off, on and 'remember'. This is used for TuyaMCU memory state",
	//chandetail:"enum":"ChType_OffOnRemember",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_OffOnRemember,
	//chandetail:{"name":"Voltage_div100",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"For TuyaMCU power metering. Not used for BL09** and CSE** sensors. Divider is used by TuyaMCU, because TuyaMCU sends always values as integers so we have to divide them before displaying on UI",
	//chandetail:"enum":"ChType_Voltage_div100",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_Voltage_div100,
	//chandetail:{"name":"Temperature_div2",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"Just like ChType_Temperature_div10, but for multiplied by 0.5.",
	//chandetail:"enum":"ChType_Temperature_div2",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_Temperature_div2,
	//chandetail:{"name":"TimerSeconds",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"This will display time formatted to minutes, hours, etc.",
	//chandetail:"enum":"ChType_TimerSeconds",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_TimerSeconds,
	//chandetail:{"name":"Frequency_div10",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"For TuyaMCU power metering. Not used for BL09** and CSE** sensors. Divider is used by TuyaMCU, because TuyaMCU sends always values as integers so we have to divide them before displaying on UI",
	//chandetail:"enum":"ChType_Frequency_div10",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_Frequency_div10,
	//chandetail:{"name":"PowerFactor_div100",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"For TuyaMCU power metering. Not used for BL09** and CSE** sensors. Divider is used by TuyaMCU, because TuyaMCU sends always values as integers so we have to divide them before displaying on UI",
	//chandetail:"enum":"ChType_PowerFactor_div100",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_PowerFactor_div100,
	//chandetail:{"name":"Pressure_div100",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"Pressure in hPa, but divided by 100",
	//chandetail:"enum":"Pressure_div100",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_Pressure_div100,
	//chandetail:{"name":"Temperature_div100",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"Just like humidity_div100, but for temperature.",
	//chandetail:"enum":"ChType_Temperature_div100",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_Temperature_div100,
	//chandetail:{"name":"LeakageCurrent_div1000",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"Leakage current in mA",
	//chandetail:"enum":"ChType_LeakageCurrent_div1000",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_LeakageCurrent_div1000,
	//chandetail:{"name":"Power_div100",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"Just like power, but with one decimal place (but stored as integer, for TuyaMCU support)",
	//chandetail:"enum":"ChType_Power_div100",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_Power_div100,
	//chandetail:{"name":"Motion",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"Motion",
	//chandetail:"enum":"Motion",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_Motion,
	//chandetail:{"name":"ReadOnly_div10",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"This channel is read only.",
	//chandetail:"enum":"ChType_ReadOnly_div10",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_ReadOnly_div10,
	//chandetail:{"name":"ReadOnly_div100",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"This channel is read only.",
	//chandetail:"enum":"ChType_ReadOnly_div100",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_ReadOnly_div100,
	//chandetail:{"name":"ReadOnly_div1000",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"This channel is read only.",
	//chandetail:"enum":"ChType_ReadOnly_div1000",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_ReadOnly_div1000,
	//chandetail:{"name":"Ph",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"Ph Water Quality",
	//chandetail:"enum":"ChType_Ph",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_Ph,
	//chandetail:{"name":"Orp",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"Orp Water Quality",
	//chandetail:"enum":"ChType_Orp",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_Orp,
	//chandetail:{"name":"Tds",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"TDS Water Quality",
	//chandetail:"enum":"ChType_Tds",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_Tds,
	//chandetail:{"name":"Motion_n",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"Motion_n",
	//chandetail:"enum":"Motion_n",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_Motion_n,
	//chandetail:{"name":"Frequency_div1000",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"For TuyaMCU power metering. Not used for BL09** and CSE** sensors. Divider is used by TuyaMCU, because TuyaMCU sends always values as integers so we have to divide them before displaying on UI",
	//chandetail:"enum":"ChType_Frequency_div1000",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_Frequency_div1000,

	ChType_OpenStopClose,
	ChType_Percent,
	ChType_StopUpDown,
		
	//chandetail:{"name":"Max",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"This is the current total number of available channel types.",
	//chandetail:"enum":"ChType_Max",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_Max,
} channelType_t;


#if PLATFORM_BL602
#define PLATFORM_GPIO_MAX 24
#elif PLATFORM_XR809
#define PLATFORM_GPIO_MAX 24
#elif PLATFORM_XR806
#define PLATFORM_GPIO_MAX 27
#elif PLATFORM_XR872
#define PLATFORM_GPIO_MAX 36
#elif PLATFORM_W600
#define PLATFORM_GPIO_MAX 17
#elif PLATFORM_W800
#define PLATFORM_GPIO_MAX 44
#elif PLATFORM_LN882H
#define PLATFORM_GPIO_MAX 26
#elif PLATFORM_ESPIDF
#ifdef CONFIG_IDF_TARGET_ESP32C3
#define PLATFORM_GPIO_MAX 22
#elif CONFIG_IDF_TARGET_ESP32C2
#define PLATFORM_GPIO_MAX 21
#elif CONFIG_IDF_TARGET_ESP32S2
#define PLATFORM_GPIO_MAX 47
#elif CONFIG_IDF_TARGET_ESP32S3
#define PLATFORM_GPIO_MAX 49
#elif CONFIG_IDF_TARGET_ESP32C6
#define PLATFORM_GPIO_MAX 31
#elif CONFIG_IDF_TARGET_ESP32
#define PLATFORM_GPIO_MAX 40
#elif CONFIG_IDF_TARGET_ESP32C5
#define PLATFORM_GPIO_MAX 29
#elif CONFIG_IDF_TARGET_ESP32C61
#define PLATFORM_GPIO_MAX 22
#else
#define PLATFORM_GPIO_MAX 0
#endif
#elif PLATFORM_ESP8266
#define PLATFORM_GPIO_MAX 13
#elif PLATFORM_TR6260
#define PLATFORM_GPIO_MAX 25
#elif PLATFORM_RTL87X0C
#define PLATFORM_GPIO_MAX 24
#elif PLATFORM_RTL8710B
#define PLATFORM_GPIO_MAX 17
#elif PLATFORM_RTL8710A
#define PLATFORM_GPIO_MAX 20
#elif PLATFORM_RTL8720D || PLATFORM_RTL8721DA
#define PLATFORM_GPIO_MAX 64
#elif PLATFORM_RTL8720E
#define PLATFORM_GPIO_MAX 52
#elif PLATFORM_ECR6600
#define PLATFORM_GPIO_MAX 27
#elif PLATFORM_BK7252 || PLATFORM_BK7252N
#define PLATFORM_GPIO_MAX 40
#else
#define PLATFORM_GPIO_MAX 29
#endif

#define CHANNEL_MAX 64

// Special channel indexes
// They were created so we can have easy and seamless
// access to special variables internally.
// Futhermore, they can be very useful for scripting,
// because they can be plugged into "setChannel" command
#define SPECIAL_CHANNEL_BRIGHTNESS		129
#define SPECIAL_CHANNEL_LEDPOWER		130
#define SPECIAL_CHANNEL_BASECOLOR		131
#define SPECIAL_CHANNEL_TEMPERATURE		132
// RGBCW access (well, in reality, we just use RGB access and CW is derived from temp)
#define SPECIAL_CHANNEL_BASECOLOR_FIRST 133
#define SPECIAL_CHANNEL_BASECOLOR_RED	133
#define SPECIAL_CHANNEL_BASECOLOR_GREEN	134
#define SPECIAL_CHANNEL_BASECOLOR_BLUE	135
#define SPECIAL_CHANNEL_BASECOLOR_COOL	136
#define SPECIAL_CHANNEL_BASECOLOR_WARM	137
#define SPECIAL_CHANNEL_BASECOLOR_LAST	137
#define SPECIAL_CHANNEL_OBK_FREQUENCY 138

// note: real limit here is MAX_RETAIN_CHANNELS
#define SPECIAL_CHANNEL_FLASHVARS_FIRST	200
#define SPECIAL_CHANNEL_FLASHVARS_LAST	264


#if PLATFORM_W800 || PLATFORM_BK7252 || PLATFORM_BK7252N || PLATFORM_XR872

#define MAX_PIN_ROLES 48

typedef struct pinsState_s {
	// All above values are indexed by physical pin index
	// (so we assume we have maximum of 32 pins)
	byte roles[48];
	byte channels[48];
	// extra channels array - this is needed for
	// buttons, so button can toggle one relay on single click
	// and other relay on double click
	byte channels2[48];
	// This single field above, is indexed by CHANNEL INDEX
	// (not by pin index)
	byte channelTypes[CHANNEL_MAX];
} pinsState_t;

#elif PLATFORM_ESPIDF

#define MAX_PIN_ROLES 50

typedef struct pinsState_s
{
	// All above values are indexed by physical pin index
	// (so we assume we have maximum of 32 pins)
	byte roles[50];
	byte channels[50];
	// extra channels array - this is needed for
	// buttons, so button can toggle one relay on single click
	// and other relay on double click
	byte channels2[50];
	// This single field above, is indexed by CHANNEL INDEX
	// (not by pin index)
	byte channelTypes[CHANNEL_MAX];
} pinsState_t;

#elif PLATFORM_RTL8720D || PLATFORM_RTL8721DA || PLATFORM_RTL8720E

#define MAX_PIN_ROLES 64

typedef struct pinsState_s
{
	// All above values are indexed by physical pin index
	// (so we assume we have maximum of 32 pins)
	byte roles[64];
	byte channels[64];
	// extra channels array - this is needed for
	// buttons, so button can toggle one relay on single click
	// and other relay on double click
	byte channels2[64];
	// This single field above, is indexed by CHANNEL INDEX
	// (not by pin index)
	byte channelTypes[CHANNEL_MAX];
} pinsState_t;

#else

#define MAX_PIN_ROLES 32

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

#endif

#if MAX_PIN_ROLES < PLATFORM_GPIO_MAX
#error "MAX_PIN_ROLES < PLATFORM_GPIO_MAX, undefined behaviour"
#endif

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
#define OBK_FLAG_MQTT_HASS_ADD_RELAYS_AS_LIGHTS		34
#define OBK_FLAG_NOT_PUBLISH_AVAILABILITY			35
#define OBK_FLAG_DRV_DISABLE_AUTOSTART				36
#define OBK_FLAG_WIFI_FAST_CONNECT					37
#define OBK_FLAG_POWER_FORCE_ZERO_IF_RELAYS_OPEN	38
#define OBK_FLAG_MQTT_PUBLISH_ALL_CHANNELS			39
#define OBK_FLAG_MQTT_ENERGY_IN_KWH					40
#define OBK_FLAG_BUTTON_DISABLE_ALL					41
#define OBK_FLAG_DOORSENSOR_INVERT_STATE			42
#define OBK_FLAG_TUYAMCU_USE_QUEUE					43
#define OBK_FLAG_HTTP_DISABLE_AUTH_IN_SAFE_MODE		44
#define OBK_FLAG_DISCOVERY_DONT_MERGE_LIGHTS		45
#define OBK_FLAG_TUYAMCU_STORE_RAW_DATA				46
#define OBK_FLAG_TUYAMCU_STORE_ALL_DATA				47
#define OBK_FLAG_POWER_INVERT_AC					48
#define OBK_FLAG_HTTP_NO_ONOFF_WORDS				49
#define OBK_FLAG_MQTT_NEVERAPPENDGET				50
#define OBK_FLAG_WIFI_ENHANCED_FAST_CONNECT			51

#define OBK_TOTAL_FLAGS 52

#define LOGGER_FLAG_MQTT_DEDUPER					1
#define LOGGER_FLAG_POWER_SAVE						2
#define LOGGER_FLAG_EVERY_SECOND_PRINT				3
#define LOGGER_FLAG_PERIODIC_WIFI_INFO				4

#define LOGGER_TOTAL_FLAGS 8



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
	// 0x08
	uint32_t genericFlags;
	// 0x0C
	uint32_t genericFlags2;
	// 0x10
	unsigned short changeCounter;
	unsigned short otaCounter;
	// 0x14
	// target wifi credentials
	char wifi_ssid[64];
	// 0x54
	char wifi_pass[64];
	// 0x94
	// MQTT information for Home Assistant
	char mqtt_host[256];
	// ofs 0x194
	// note: #define CGF_MQTT_CLIENT_ID_SIZE			64
	char mqtt_clientId[CGF_MQTT_CLIENT_ID_SIZE];
	// ofs 0x1D4
	char mqtt_userName[64];
	// ofs 0x214
	char mqtt_pass[128];
	//mqtt_port at offs 0x00000294 
	int mqtt_port;
	// addon JavaScript panel is hosted on external server
	// at offs 0x298 
	char webappRoot[64];
	// TODO?
	byte mac[6];
	// at ofs: 0x2DE
	// NOTE: NO LONGER 4 byte aligned!!!
	// TODO?
	// #define CGF_SHORT_DEVICE_NAME_SIZE		32
	char shortDeviceName[CGF_SHORT_DEVICE_NAME_SIZE];
	// #define CGF_DEVICE_NAME_SIZE			64
	// at ofs: 0x2FE
	char longDeviceName[CGF_DEVICE_NAME_SIZE];

	//pins at offs 0x0000033E
	// 160 bytes
	pinsState_t pins;
	// startChannelValues at offs 0x000003DE
	// 64 * 2
	short startChannelValues[CHANNEL_MAX];
	// unused_fill at offs 0x0000045E 
	short unused_fill; // correct alignment
	// dgr_sendFlags at offs 0x00000460 
	int dgr_sendFlags;
	// dgr_recvFlags at offs 0x00000464 
	int dgr_recvFlags;
	// dgr_name at offs 0x00000468
	char dgr_name[16];
	// at offs 0x478
	char ntpServer[32];
	// 8 * 4 = 32 bytes
	cfgPowerMeasurementCalibration_t cal;
	// offs 0x000004B8
	// short press 1 means 100 ms short press time
	// So basically unit is 0.1 second
	byte buttonShortPress;
	// offs 0x000004B9
	byte buttonLongPress;
	// offs 0x000004BA
	byte buttonHoldRepeat;
	// offs 0x000004BB
	byte unused_fill1;

	// offset 0x000004BC
	unsigned long LFS_Size; // szie of LFS volume.  it's aligned against the end of OTA
	int loggerFlags;
#if PLATFORM_W800 || PLATFORM_BK7252 || PLATFORM_BK7252N || PLATFORM_XR872
	byte unusedSectorAB[51];
#elif PLATFORM_ESPIDF
	byte unusedSectorAB[43];
#elif PLATFORM_RTL8720D || PLATFORM_RTL8721DA || PLATFORM_RTL8720E
	byte unusedSectorAB;
#else    
	byte unusedSectorAB[99];
#endif    
	obkStaticIP_t staticIP;
	ledRemap_t ledRemap;
	led_corr_t led_corr;
	// alternate topic name for receiving MQTT commands
	// offset 0x00000554
	char mqtt_group[64];
	// offs 0x00000594
	byte unused_bytefill[3];
	// offs 0x00000597
	byte timeRequiredToMarkBootSuccessfull;
	//offs 0x00000598
	int ping_interval;
	int ping_seconds;
	// 0x000005A0
	char ping_host[64];
	// ofs 0x000005E0 (dec 1504)
	//char initCommandLine[512];
#if PLATFORM_W600 || PLATFORM_W800
#define ALLOW_SSID2 0
#define ALLOW_WEB_PASSWORD 0
	char initCommandLine[512];
#else
#define ALLOW_SSID2 1
#define ALLOW_WEB_PASSWORD 1
	char initCommandLine[1568];
	// offset 0x00000C00 (3072 decimal)
	char wifi_ssid2[64];
	// offset 0x00000C40 (3136 decimal)
	char wifi_pass2[68];
	// offset 0x00000C84 (3204 decimal)
	char webPassword[33];
	// offset 0x00000CA5 (3237 decimal)
	byte mqtt_use_tls;
	// offset 0x00000CA6 (3238 decimal)
	byte mqtt_verify_tls_cert;
	// offset 0x00000CA7 (3239 decimal)
	char mqtt_cert_file[20];
	// offset 0x00000CBB (3259 decimal)
	byte disable_web_server;
	// offset 0x00000CBC (3260 decimal)
#if PLATFORM_BEKEN
	obkFastConnectData_t fcdata;
	// offset 0x00000D0C (3340 decimal)
	char unused[244];
#else
	char unused[324];
#endif
#endif
} mainConfig_t;

// one sector is 4096 so it we still have some expand possibility
#define MAGIC_CONFIG_SIZE_V3		2016
#define MAGIC_CONFIG_SIZE_V4		3584

extern mainConfig_t g_cfg;

extern char g_enable_pins;
extern int g_initialPinStates;

#define CHANNEL_SET_FLAG_FORCE		1
#define CHANNEL_SET_FLAG_SKIP_MQTT	2
#define CHANNEL_SET_FLAG_SILENT		4

void PIN_ticks(void* param);

void PIN_DeepSleep_SetWakeUpEdge(int pin, byte edgeCode);
void PIN_DeepSleep_SetAllWakeUpEdges(byte edgeCode);

void PIN_set_wifi_led(int value);
void PIN_AddCommands(void);
void PINS_BeginDeepSleepWithPinWakeUp(unsigned int wakeUpTime);
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
void CHANNEL_Set_Ex(int ch, int iVal, int iFlags, int ausemovingaverage);
void CHANNEL_Set(int ch, int iVal, int iFlags);
void CHANNEL_SetSmart(int ch, float fVal, int iFlags);
void CHANNEL_Set_FloatPWM(int ch, float fVal, int iFlags);
void CHANNEL_Add(int ch, int iVal);
void CHANNEL_AddClamped(int ch, int iVal, int min, int max, int bWrapInsteadOfClamp);
int CHANNEL_Get(int ch);
float CHANNEL_GetFinalValue(int channel);
float CHANNEL_GetFloat(int ch);
int CHANNEL_GetRoleForOutputChannel(int ch);
bool CHANNEL_ShouldBePublished(int ch);
bool CHANNEL_IsPowerRelayChannel(int ch);
// See: enum channelType_t
void CHANNEL_SetType(int ch, int type);
int CHANNEL_GetType(int ch);
void CHANNEL_SetFirstChannelByTypeEx(int requiredType, int newVal, int ausemovingaverage);
void CHANNEL_SetFirstChannelByType(int requiredType, int newVal);
// CHANNEL_SET_FLAG_*
void CHANNEL_SetAll(int iVal, int iFlags);
void CHANNEL_SetStateOnly(int iVal);
int CHANNEL_HasChannelPinWithRole(int ch, int iorType);
int CHANNEL_HasChannelPinWithRoleOrRole(int ch, int iorType, int iorType2);
bool CHANNEL_IsInUse(int ch);
void Channel_SaveInFlashIfNeeded(int ch);
int CHANNEL_FindMaxValueForChannel(int ch);
int CHANNEL_FindIndexForType(int requiredType); 
int CHANNEL_FindIndexForPinType(int requiredType);
int CHANNEL_FindIndexForPinType2(int requiredType, int requiredType2);
// cmd_channels.c
bool CHANNEL_HasLabel(int ch);
const char* CHANNEL_GetLabel(int ch);
void CHANNEL_SetLabel(int ch, const char *s, int bHideTogglePrefix);
bool CHANNEL_ShouldAddTogglePrefixToUI(int ch);
bool CHANNEL_HasNeverPublishFlag(int ch);
//ledRemap_t *CFG_GetLEDRemap();

void PIN_get_Relay_PWM_Count(int* relayCount, int* pwmCount, int* dInputCount);
int h_isChannelPWM(int tg_ch);
int h_isChannelRelay(int tg_ch);
int h_isChannelDigitalInput(int tg_ch);

int CHANNEL_ParseChannelType(const char* s);
const char *ChannelType_GetTitle(int type);
const char *ChannelType_GetUnit(int type);
int ChannelType_GetDivider(int type);
int ChannelType_GetDecimalPlaces(int type);

int PIN_GetPWMIndexForPinIndex(int pin);

int PIN_ParsePinRoleName(const char* name);
const char* PIN_RoleToString(int role);
// return number of channels used for a role
// taken from code in http_fnc.c
int PIN_IOR_NofChan(int test);

extern const char* g_channelTypeNames[];

#if ALLOW_SSID2
int FV_GetStartupSSID_StoredValue(int adefault);
void FV_UpdateStartupSSIDIfChanged_StoredValue(int assidindex);
#endif

#ifdef ENABLE_BL_MOVINGAVG
float XJ_MovingAverage_float(float aprevvalue, float aactvalue);
int XJ_MovingAverage_int(int aprevvalue, int aactvalue);
#endif

#endif
