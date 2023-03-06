# Introduction

OpenBK7231T/OpenBeken is a Tasmota/Esphome replacement for new Tuya modules featuring MQTT and Home Assistant compatibility.
This repository is named "OpenBK7231T_App", but now it's a multiplatform app, supporting build for multiple separate chips:
- [BK7231T](https://www.elektroda.com/rtvforum/topic3951016.html) ([WB3S](https://developer.tuya.com/en/docs/iot/wb3s-module-datasheet?id=K9dx20n6hz5n4), [WB2S](https://developer.tuya.com/en/docs/iot/wb2s-module-datasheet?id=K9ghecl7kc479), WB2L, etc)
- [BK7231N](https://www.elektroda.com/rtvforum/topic3951016.html) ([CB2S](https://developer.tuya.com/en/docs/iot/cb2s-module-datasheet?id=Kafgfsa2aaypq), [CB2L](https://developer.tuya.com/en/docs/iot/cb2l-module-datasheet?id=Kai2eku1m3pyl), [WB2L_M1](https://www.elektroda.com/rtvforum/topic3903356.html), etc)
- T34 ([T34 is based on BK7231N](https://developer.tuya.com/en/docs/iot/t34-module-datasheet?id=Ka0l4h5zvg6j8))
- BL2028N ([BL2028N is a Belon version of BK7231N](https://www.elektroda.com/rtvforum/viewtopic.php?p=20262533#20262533))
- [XR809](https://www.elektroda.com/rtvforum/topic3806769.html) ([XR3](https://developer.tuya.com/en/docs/iot/xr3-datasheet?id=K98s9168qi49g), etc)
- [BL602](https://www.elektroda.com/rtvforum/topic3889041.html) ([SM-028_V1.3 etc](https://www.elektroda.com/rtvforum/topic3945435.html))
- W800 (W800-C400, WinnerMicro WiFi & Bluetooth), W801
- [W600](https://www.elektroda.com/rtvforum/viewtopic.php?p=20252619#20252619) (WinnerMicro chip), W601 ([WIS600, ESP-01W](https://www.elektroda.com/rtvforum/topic3950611.html), [TW-02](https://www.elektroda.com/rtvforum/viewtopic.php?p=20239610#20239610), [TW-03](https://www.elektroda.com/rtvforum/topic3929601.html), etc)

Please use automatically compiled binaries from the Releases tab. To build yourself for a given platform, just checkout first our version of SDK and then checkout this app repository into it, details later.

[Russian guide for BK7231N/T34](https://www.v-elite.ru/t34)<br>
[Russian guide for BL602](https://www.v-elite.ru/bl602rgb)<br>
[Russian Youtube guide for BK7231/T34](https://www.youtube.com/watch?v=BnmSWZchK-E)<br>

If you want to get some generic information about BK7231 modules, pinout, peripherals, [consult our docs topic](https://www.elektroda.com/rtvforum/topic3951016.html).

# [Supported Devices/Templates List](https://openbekeniot.github.io/webapp/devicesList.html) (Get 🏆[free SD Card](https://www.elektroda.com/rtvforum/topic3950844.html)🏆 for submitting new one!)
We have our own interactive devices database that is maintained by users.
The database is also accessible from inside our firmware (but requires internet connection to fetch).
Have a not listed device? HELP US, submit a teardown [here](https://www.elektroda.com/rtvforum/posting.php?mode=newtopic&f=51) and 🏆**get free SD card and gadgets set**🏆 ! Thanks to cooperation with [Elektroda.com](https://www.elektroda.com/), if you submit a detailed teardown/article/review, we can send you [this set of gadgets](https://obrazki.elektroda.pl/1470574200_1670833596.jpg) for free (🚚shipping with normal letter🚚).
NOTE: Obviously almost any device with supported chip (BK7231, BL602, W600, etc is potentially supported and it's not possible to list all available devices in the market, so feel free to try even if your device is not listed - *we are [here](https://www.elektroda.com/rtvforum/forum390.html) to help and guide you step by step*!)


# Features

OpenBeken features:
- Tasmota-like setup, configuration and experience on all supported platforms (supports common Tasmota JSON over http and MQTT, etc)
- OTA firwmare upgrade system (for BK, W*00, BL602); to use OTA, drag and drop RBL file (always RBL file) on OTA field on new Web App Javascript Console
- MQTT compatibility with Home Assistant (with both Yaml generator and [HA Discovery](https://youtu.be/pkcspey25V4)) 
- Support for multiple relays, buttons, leds, inputs and PWMs, everything fully scriptable
- Driver system for custom peripherals, including [TuyaMCU](https://www.elektroda.com/rtvforum/topic3898502.html), I2C bus and [BL0942](https://www.elektroda.com/rtvforum/topic3887748.html), BL0937 power metering chips, Motor Driver Bridge.
- Supports multiple I2C devices, like TC74 temperature sensor, MCP23017 port expander, PCF8574T LCD 2x16 (or other?), etc
- NTP time from network (can be used with [TH06](https://www.elektroda.com/rtvforum/topic3942730.html) and other TuyaMCU devices), can run any script on selected weekday hour:minute:second
- basic support for [TuyaMCU Battery Powered devices protocol](https://www.elektroda.com/rtvforum/topic3914412.html) (TuyaMCU enables WiFi module only to report the state, eg. for door sensors, water sensors)
- RGBCW LED lighting control compatible with Home Assistant (both PWM LEDs, SM2135 LEDs and BP5758 LEDs)
- LittleFS integration for large files (resides in OTA memory, so you have to backup it every time you OTA)
- Command line system for starting and configuring drivers
- Short startup command (up to 512 characters) storage in flash config, so you can easily init your drivers (eg. BL0942) without LittleFS
- Advanced scripting and events system (allows you to mirror Tasmota rules, for example catch button click, double click, hold)
- Easily configurable via commands (see [tutorial](https://www.elektroda.com/rtvforum/topic3947241.html))
- Thanks to keeping Tasmota standard, OBK has basic compatibility with [ioBroker](https://www.youtube.com/watch?v=x4p3JHXbK1E&ab_channel=Elektrodacom) and similiar systems through TELE/STAT/CMND MQTT packets
- DDP lighting protocol support ("startDriver DDP" in autoexec.bat/short startup command), works with xLights
- Automatic reconnect when WiFi network goes out
- and much more

# Wiki 

For more Information refer to the [WIKI](https://github.com/openshwprojects/OpenBK7231T_App/wiki/Wiki-Home)

# Building

OpenBeken supports online builds for all platforms (BK7231T, BK7231N, XR809, BL602, W800), but if you want to compile it yourself, see  [BUILDING.md](https://github.com/openshwprojects/OpenBK7231T_App/blob/main/BUILDING.md)

# Flashing

See  [FLASHING.md](https://github.com/openshwprojects/OpenBK7231T_App/blob/main/FLASHING.md)
  
# Safe mode
  
  Device is counting full boots (full boot is a boot after which device worked for 30 seconds). If you power off and on device multiple times, it will enter open access point mode and safe mode (safe mode means even pin systems are not initialized). Those modes are used to recover devices from bad configs and errors.
    
# [Docs - Console Commands, Flags, Constants, Pin Roles, Channel Types, FAQ](https://github.com/openshwprojects/OpenBK7231T_App/blob/main/docs)

      
# Console Command Examples

| Command        | Description  |
| ------------- | -----:|
| addRepeatingEvent 15 -1 SendGet http://192.168.0.112/cm?cmnd=Power0%20Toggle | This will send a Tasmota HTTP Toggle command every 15 seconds to given device. Repeats value here is "-1" because we want this event to stay forever. |
| addEventHandler OnClick 8 SendGet http://192.168.0.112/cm?cmnd=Power0%20Toggle     | This will send a Tasmota HTTP Toggle command to given device when a button on pin 8 is clicked (pin 8, NOT channel 8) |
| addChangeHandler Channel1 != 0  SendGet http://192.168.0.112/cm?cmnd=Power0%20On | This will set a Tasmota HTTP Power0 ON command when a channel 1 value become non-zero |
| addChangeHandler Channel1 == 0  SendGet http://192.168.0.112/cm?cmnd=Power0%20Off | This will set a Tasmota HTTP Power0 OFF command when a channel 1 value become zero |
| addChangeHandler Channel1 == 1 addRepeatingEvent 60 1 setChannel 1 0 | This will create a new repeating events with 1 repeat count and 60 seconds delay everytime Channel1 becomes 1. Basically, it will automatically turn off the light 60 seconds after you turn it on. TODO: clear previous event instance? |
| addRepeatingEvent 5 -1 led_enableAll !$led_enableAll | This simple timer will toggle LED state every 5 seconds. -1 hear means infinite repeats. The ! stands for negation and $led_enableAll is a constant that you can read to get 0 or 1. It works like $CH11, $CH4 etc (any number) for accessing channel value |

# Example configurations (example autoexec.bat files for LittleFS system)


[Configuration for EDM-01AA-EU dimmer with TuyaMCU](https://www.elektroda.com/rtvforum/topic3929151.html)

```
startDriver TuyaMCU
setChannelType 1 toggle
setChannelType 2 dimmer
tuyaMcu_setBaudRate 115200
tuyaMcu_setDimmerRange 1 1000
// linkTuyaMCUOutputToChannel dpId verType tgChannel
linkTuyaMCUOutputToChannel 1 bool 1
linkTuyaMCUOutputToChannel 2 val 2
```

[Configuration for QIACHIP Universal WIFI Ceiling Fan Light Remote Control Kit - BK7231N - CB2S with TuyaMCU](https://www.elektroda.com/rtvforum/topic3895301.html)

```
// start MCU driver
startDriver TuyaMCU
// let's say that channel 1 is dpid1 - fan on/off
setChannelType 1 toggle
// map dpid1 to channel1, var type 1 (boolean)
linkTuyaMCUOutputToChannel 1 1 1
// let's say that channel 2 is dpid9 - light on/off
setChannelType 2 toggle
// map dpid9 to channel2, var type 1 (boolean)
linkTuyaMCUOutputToChannel 9 1 2
//channel 3 is dpid3 - fan speed
setChannelType 3 LowMidHigh
// map dpid3 to channel3, var type 4 (enum)
linkTuyaMCUOutputToChannel 3 4 3
//dpId 17 = beep on/off
setChannelType 4 toggle
linkTuyaMCUOutputToChannel 17 1 4
//
//
//dpId 6, dataType 4-DP_TYPE_ENUM = set timer
setChannelType 5 TextField
linkTuyaMCUOutputToChannel 6 4 5
//
//
//dpId 7, dataType 2-DP_TYPE_VALUE = timer remaining
setChannelType 6 ReadOnly
linkTuyaMCUOutputToChannel 7 2 6
```
      
# Scripting engine
  
  Our scripting engine is able to process script files from LittleFS file system. Each script execution acts like a separate thread (but that's not a real RTOS thread, of course) and can process console commands, conditionals and delays.
  
  To create script, go to secondary web panel (JavaScript App), go to LittleFS tab, click "create file" and enter some content. Use buttons to stop all scripts or to save and run the one you are writing. From a console, you can also use 'startScript fileName label' syntax.
  
<br><b>Script example 1:</b><br>
Loop demo. Features a 'goto' script command (for use within script)
and, obviously, a label.<br>
Requirements: <br>
- channel 1 - output relay<br>

```
again:
	echo "Step 1"
	setChannel 1 0
	echo "Step 2"
	delay_s 2
	echo "Step 3"
	setChannel 1 1
	echo "Step 4"
	delay_s 2
	goto again
```

 <br><b>Script example 2:</b> <br>
Loop & if demo<br>
This example shows how you can use a dummy channel as a variable to create a loop<br>
Requirements: <br>
 - channel 1 - output relay<br>
 - channel 11 - loop variable counter<br>

```
restart:
	// Channel 11 is a counter variable and starts at 0
	setChannel 11 0
again:

	// If channel 11 value reached 10, go to done
	if $CH11>=10 then goto done
	// otherwise toggle channel 1, wait and loop
	toggleChannel 1
	addChannel 11 1
	delay_ms 250
	goto again
done:
	toggleChannel 1
	delay_s 1
	toggleChannel 1
	delay_s 1
	toggleChannel 1
	delay_s 1
	toggleChannel 1
	delay_s 1
	goto restart
``` 
  
 
 <br><b>Script example 3:</b> <br>
Thread cancelation demo and exclude self demo<br>
  This example shows how you can create a script thread with an unique ID and use this ID to cancel the thread later<br>
 Requirements: <br>
 - channel 1 - output relay<br>
 - pin 8 - button<br>
 - pin 9 - button<br>

```
// 'this' is a special keyword - it mean search for script/label in this file
// 123 and 456 are unique script thread names
addEventHandler OnClick 8 startScript this label1 123
addEventHandler OnClick 9 startScript this label2 456

label1:
	// stopScript ID bExcludeSelf
	// this will stop all other instances
	stopScript 456 1
	stopScript 123 1
	setChannel 1 0
	delay_s 1
	setChannel 1 1
	delay_s 1
	setChannel 1 0
	delay_s 1
	setChannel 1 1
	delay_s 1
	setChannel 1 0
	delay_s 1
	setChannel 1 1
	delay_s 1
	exit;
label2:
	// stopScript ID bExcludeSelf
	// this will stop all other instances
	stopScript 456 1
	stopScript 123 1
	setChannel 1 0
	delay_s 0.25
	setChannel 1 1
	delay_s 0.25
	setChannel 1 0
	delay_s 0.25
	setChannel 1 1
	delay_s 0.25
	setChannel 1 0
	delay_s 0.25
	setChannel 1 1
	delay_s 0.25
	setChannel 1 0
	delay_s 0.25
	exit;
```  

 
 <br><b>Script example 4:</b> <br>
Using channel value as a variable demo<br>
Requirements: <br>
- channel 1 - output relay<br>
- channel 11 - you may use it as ADC, or just use setChannel 11 100 or setChannel 11 500 in console to change delay<br>

```

// set default value
setChannel 11 500
// if you don't have ADC, use this to force-display 11 as a slider on GUI
setChannelType 11 dimmer1000

looper:
	setChannel 1 0
	delay_ms $CH11
	setChannel 1 1
	delay_ms $CH11
	goto looper
```  

# Channel Types

Channel types are often not required and don't have to be configured, but in some cases they are required for better device control from OpenBeken web panel. Channel types describes the kind of value stored in channel, for example, if you have a Tuya Fan Controller with 3 speeds control,  you can set the channel type to LowMidHigh and it will display the correct UI radiobutton on OpenBeken panel.

Some channels have "_div10" or "_div100" sufixes. This is for TuyaMCU. This is needed because TuyaMCU sends values as integers, so it sends, for example, 215 for 21.5C temperature, and we store it internally as 215 and only convert to float for display.

[See full list here](https://github.com/openshwprojects/OpenBK7231T_App/blob/main/docs/channelTypes.md)
  
# Simple TCP command server for scripting
  
  You can enable a simple TCP server in device Generic/Flags option, which will listen by default on port 100. Server can accept single connection at time from Putty in RAW mode (raw TCP connection) and accepts text commands for OpenBeken console. In future, we may add support for multiple connections at time. Server will close connection if client does nothing for some time.
  
  There are some extra short commands for TCP console:
- GetChannel [index] 
- GetReadings - returns voltage, current and power
- ShortName
the commands above return a single ASCII string as a reply so it's easy to parse.
  
# MQTT topics of published variables

Some MQTT variables are being published only at the startup, some are published periodically (if you enable "broadcast every minute" flag), some are published only when a given value is changed. Below is the table of used publish topics (TODO: add full descriptions)

Hint: in HA, you can use MQTT wildcard to listen to publishes. OBK_DEV_NAME/#

Publishes send by OBK device:
| Topic        | Sample Value  | Description  |
| ------------- |:-------------:| -------------:|
| OBK_DEV_NAME/connected | "online" | Send on connect. |
| OBK_DEV_NAME/sockets |"5" | Send on connect and every minute (if enabled) |
| OBK_DEV_NAME/rssi |"-70" | Send on connect and every minute (if enabled) |
| OBK_DEV_NAME/uptime | "653" |Send on connect and every minute (if enabled) |
| OBK_DEV_NAME/freeheap |"95168" | Send on connect and every minute (if enabled) | 
| OBK_DEV_NAME/ip | "192.168.0.123" |Send on connect and every minute (if enabled) | 
| OBK_DEV_NAME/datetime | "" |Send on connect and every minute (if enabled) |
| OBK_DEV_NAME/mac | "84:e3:42:65:d1:87 " |Send on connect and every minute (if enabled) |
| OBK_DEV_NAME/build | "Build on Nov 12 2022 12:39:44 version 1.0.0" |Send on connect and every minute (if enabled) | 
| OBK_DEV_NAME/host |"obk_t_fourRelays" | Send on connect and every minute (if enabled) |
| OBK_DEV_NAME/voltage/get |"221" | voltage from BL0942/BL0937 etc |
| OBK_DEV_NAME/led_enableAll/get | "1" |send when LED On/Off changes or when periodic broadcast is enabled |
| OBK_DEV_NAME/led_basecolor_rgb/get |"FFAABB" | send when LED color changes or when periodic broadcast is enabled.  |
| OBK_DEV_NAME/led_dimmer/get |"100" | send when LED dimmer changes or when periodic broadcast is enabled |
  
  
 Publishes received by OBK device:
 | Topic        | Sample Value  | Description  |
| ------------- |:-------------:| -------------:|
| OBK_DEV_NAME/INDEX/set | "1" | Sets the channel of INDEX to given value. This can set relays and also provide DIRECT PWM access. If channel is mapped to TuyaMCU, TuyaMCU will also be updated |
| todo |"100" | tooodo |

# RGBCW Tuya 5 PWMs LED bulb control compatible with Home Assistant
  
  RGBCW light bulbs are now supported and they are compatible with HA by rgb_command_template, brightness_value_template, color_temp_value_template commands. Please follow the guide below showing how to flash, setup and pair them with HA by MQTT:
  
  https://www.elektroda.com/rtvforum/topic3880540.html#19938487
  
# TuyaMCU example with QIACHIP Universal WIFI Ceiling Fan Light Remote Control Kit - BK7231N - CB2S
  
  Home Assistant and Node Red. Please refer to this example:
  
  https://www.elektroda.com/rtvforum/viewtopic.php?p=20031489#20031489
  
# TuyaMCU example with Tuya 5 Speed Fan Controller by TEQOOZ - BK7231T - WB3S
  
  Home Assistant Yaml examples. Please refer to this example:
  
  https://www.elektroda.com/rtvforum/topic3908093.html
  
# TuyaMCU WB2S "Moes House" Dimmer configuration example with Home Assistant, see here:
  
https://www.elektroda.com/rtvforum/topic3898502.html
  
# I2C drivers system with support for MCP23017 bus expander, TC74 temperature sensor and more
  
  Detailed description and step by step tutorial on Elektroda.com is coming soon.
  
# BL0942 power metering plug (UART communication mode) support and calibration process
    
Please refer to this step by step guide:
  https://www.elektroda.com/rtvforum/topic3887748.html
  
# ZN268131 example, a smart switch that allows you to connect a bistable button
    
https://www.elektroda.com/rtvforum/topic3895572.html
    
# HomeAssistant Integration
MQTT based integration with Home Assistant is possible in 2 ways from the Home Assistant configuration page (`Config > Generate Home Assistant cfg`).

One can paste the generated [yaml](https://www.home-assistant.io/docs/configuration/yaml/) configuration into Home Assistant configuration manually.

Or add the devices automatically via discovery. To do click the `Start Home Assistant Discovery` button which sends outs [MQTT discovery](https://www.home-assistant.io/docs/mqtt/discovery/) messages, one for each entity (switch, light).
* The discovery topic should match the `discovery_prefix` defined in Home Assistant, the default value is `homeassistant`.
* More details about Home Assistant discovery can be found [here](https://www.home-assistant.io/docs/mqtt/discovery/).


Note: Currently, discovery is only implemented for light and relay entities.


# Detailed flashing guides along with device teardowns
  
 I have prepared several detailed teardowns and flashing guides for multiple supported devices. Each comes in two languages - English and Polish.
  
  Outdoor two relays smart switch CCWFIO232PK (BK7231T): 
  https://www.elektroda.com/rtvforum/topic3875654.html
  https://www.elektroda.pl/rtvforum/viewtopic.php?p=19906670#19906670
  
  Australian Double Power Point DETA 6922HA-Series 2 (BK7231T) (English only) 
  https://community.home-assistant.io/t/detailed-guide-on-how-to-flash-the-new-tuya-beken-chips-with-openbk7231t/437276

  Qiachip Smart Switch module (BK7231N/CB2S): 
  https://www.elektroda.com/rtvforum/topic3874289.html
  https://www.elektroda.pl/rtvforum/viewtopic.php?t=3874289&highlight=
  
  Tuya RGBCW 12W light bulb (raw PWMs, no I2C, BK7231N)
  https://www.elektroda.com/rtvforum/topic3880540.html#19938487
  https://www.elektroda.pl/rtvforum/viewtopic.php?t=3880540&highlight=
  
  LSPA9 CB2S + BL0942 power metering plug:
  https://www.elektroda.com/rtvforum/topic3887748.html
  https://www.elektroda.pl/rtvforum/viewtopic.php?t=3887748&highlight=
  
  XR3/XR809 water sensor guide:
  https://www.elektroda.com/rtvforum/topic3890640.html
  
  https://www.elektroda.pl/rtvforum/viewtopic.php?t=3890640&highlight=
  
  SM2135 LED configuration
  https://www.elektroda.com/rtvforum/topic3906898.html
  
  https://www.elektroda.pl/rtvforum/viewtopic.php?t=3906898
  
  
# Futher reading
  
For technical insights and generic SDK information related to Bekken and XRadio modules, please refer:
  
https://www.elektroda.com/rtvforum/topic3850712.html
  
https://www.elektroda.com/rtvforum/topic3866123.html
  
https://www.elektroda.com/rtvforum/topic3806769.html
  
# Support project
  
If you want to support project, please donate at: https://www.paypal.com/paypalme/openshwprojects
  
Special thanks for Tasmota/Esphome/etc contributors for making a great reference for implementing Tuya module drivers 
