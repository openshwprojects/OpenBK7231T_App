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

See our guides in Russian: [BK7231N/T34](https://www.v-elite.ru/t34), and [BL602 RGB](https://www.v-elite.ru/bl602rgb), and [Youtube guide for BK7231/T34](https://www.youtube.com/watch?v=BnmSWZchK-E)

If you want to get some generic information about BK7231 modules, available datasheets, pinout, peripherals, [consult our docs topic](https://www.elektroda.com/rtvforum/topic3951016.html).

# [Supported Devices/Templates List](https://openbekeniot.github.io/webapp/devicesList.html) (Get 🏆[free SD Card](https://www.elektroda.com/rtvforum/topic3950844.html)🏆 for submitting new one!)
We have our own interactive devices database that is maintained by users.
The database is also accessible from inside our firmware (but requires internet connection to fetch).
Have a not listed device? HELP US, submit a teardown [here](https://www.elektroda.com/rtvforum/posting.php?mode=newtopic&f=51) and 🏆**get free SD card and gadgets set**🏆 ! Thanks to cooperation with [Elektroda.com](https://www.elektroda.com/), if you submit a detailed teardown/article/review, we can send you [this set of gadgets](https://obrazki.elektroda.pl/1470574200_1670833596.jpg) for free (🚚shipping with normal letter🚚).
NOTE: Obviously almost any device with supported chip (BK7231, BL602, W600, etc is potentially supported and it's not possible to list all available devices in the market, so feel free to try even if your device is not listed - *we are [here](https://www.elektroda.com/rtvforum/forum390.html) to help and guide you step by step*!)

# [Our Youtube Channel](https://www.youtube.com/@elektrodacom) (See step by step guides for flashing and setup)
We have our own Youtube channel with OBK-related  guides. Please see our playlists:
- [flashing guides playlist] (https://www.youtube.com/playlist?list=PLzbXEc2ebpH0CZDbczAXT94BuSGrd_GoM)
- [generic setup hints, tricks, tips] (https://www.youtube.com/playlist?list=PLzbXEc2ebpH0I8m_Cfbqv1MTlQuBKYvlx)
You can help us by giving like, a comment and subscribe!

# Features

OpenBeken features:
- Tasmota-like setup, configuration and experience on all supported platforms (supports common Tasmota JSON over http and MQTT, etc)
- OTA firmware upgrade system (for BK, W*00, BL602); to use OTA, drag and drop proper OTA file on OTA field on new Web App Javascript Console
- Online [builds for all platforms](https://github.com/openshwprojects/OpenBK7231T_App/releases) via Github, also supports [Docker builds](https://github.com/openshwprojects/OpenBK7231T_App/tree/main/docker)
- MQTT compatibility with Home Assistant (with both Yaml generator and [HA Discovery](https://youtu.be/pkcspey25V4)) 
- Support for multiple relays, buttons, leds, inputs and PWMs, everything fully scriptable
- [Driver system](https://github.com/openshwprojects/OpenBK7231T_App/blob/main/docs/drivers.md) for custom peripherals, including [TuyaMCU](https://www.elektroda.com/rtvforum/topic3898502.html), I2C bus and [BL0942](https://www.elektroda.com/rtvforum/topic3887748.html), BL0937 power metering chips, Motor Driver Bridge.
- Hardware and software I2C, supports multiple I2C devices, like TC74 temperature sensor, MCP23017 port expander, PCF8574T LCD 2x16 (or other?), etc
- Hardware and software SPI, support for SPI BL0942, etc
- NTP time from network (can be used with [TH06](https://www.elektroda.com/rtvforum/topic3942730.html) and other TuyaMCU devices), can run any script on selected weekday hour:minute:second
- basic support for [TuyaMCU Battery Powered devices protocol](https://www.elektroda.com/rtvforum/topic3914412.html) (TuyaMCU enables WiFi module only to report the state, eg. for door sensors, water sensors)
- [RGBCW LED lighting control](https://www.youtube.com/watch?v=YQdR7r6lXRY) compatible with Home Assistant (both PWM LEDs, SM2135 LEDs and BP5758 LEDs)
- LittleFS integration for large files (you can write scripts there, you can host a page there with REST interface control of device)
- Command line system for starting and configuring drivers, for controlling channels, etc
- Short startup command (up to 512 characters) storage in flash config, so you can easily init your drivers (eg. BL0942) without LittleFS
- Advanced scripting and events system (allows you to mirror Tasmota rules, for example catch button click, double click, hold)
- Easily configurable via commands (see [tutorial](https://www.elektroda.com/rtvforum/topic3947241.html))
- Thanks to keeping Tasmota standard, OBK has basic compatibility with [ioBroker](https://www.youtube.com/watch?v=x4p3JHXbK1E&ab_channel=Elektrodacom) and similiar systems through TELE/STAT/CMND MQTT packets, Tasmota Control app is also supported
- DDP lighting protocol support ("startDriver DDP" in autoexec.bat/short startup command), works with xLights
- Automatic reconnect when WiFi network goes out
- and much more

There is also a bit more outdated [WIKI](https://github.com/openshwprojects/OpenBK7231T_App/wiki/Wiki-Home)

# Building

OpenBeken supports online builds for all platforms (BK7231T, BK7231N, XR809, BL602, W800), but if you want to compile it yourself, see  [BUILDING.md](https://github.com/openshwprojects/OpenBK7231T_App/blob/main/BUILDING.md)

# Flashing

See  [FLASHING.md](https://github.com/openshwprojects/OpenBK7231T_App/blob/main/FLASHING.md)
  
# Safe mode
  
  Device is counting full boots (full boot is a boot after which device worked for 30 seconds). If you power off and on device multiple times, it will enter open access point mode and safe mode (safe mode means even pin systems are not initialized). Those modes are used to recover devices from bad configs and errors.
    
# [Docs - Console Commands, Flags, Constants, Pin Roles, Channel Types, FAQ, autoexec.bat examples](https://github.com/openshwprojects/OpenBK7231T_App/blob/main/docs)
   
# Simple TCP command server for scripting
  
  You can enable a simple TCP server in device Generic/Flags option, which will listen by default on port 100. Server can accept single connection at time from Putty in RAW mode (raw TCP connection) and accepts text commands for OpenBeken console. In future, we may add support for multiple connections at time. Server will close connection if client does nothing for some time.
  
  There are some extra short commands for TCP console:
- GetChannel [index] 
- GetReadings - returns voltage, current and power
- ShortName
the commands above return a single ASCII string as a reply so it's easy to parse.
  
# MQTT topics of published variables

Some MQTT variables are being published only at the startup, some are published periodically (if you enable "broadcast every N seconds" flag, default time is one minute, customizable with command mqtt_broadcastInterval), some are published only when a given value is changed. Below is the table of used publish topics (TODO: add full descriptions)

Hint: in HA, you can use MQTT wildcard to listen to multiple publishes. OBK_DEV_NAME/#

Publishes send by OBK device:
| Topic        | Sample Value  | Description  |
| ------------- |:-------------:| -------------:|
| OBK_DEV_NAME/INDEX/get | 1 | send when a given channel is changed. INDEX is a number representing channel index. Some channels are not published by default, see flags. You can have any variable in channel, even a custom, fully scriptable counter |
| OBK_DEV_NAME/connected | "online" | Send on connect. |
| OBK_DEV_NAME/sockets |"5" | Send on connect and every N seconds (default: 60, if enabled) |
| OBK_DEV_NAME/rssi |"-70" | Send on connect and every N seconds (default: 60, if enabled) |
| OBK_DEV_NAME/uptime | "653" |Send on connect and every N seconds (default: 60, if enabled) |
| OBK_DEV_NAME/freeheap |"95168" | Send on connect and every N seconds (default: 60, if enabled) | 
| OBK_DEV_NAME/ip | "192.168.0.123" |Send on connect and every N seconds (default: 60, if enabled) | 
| OBK_DEV_NAME/datetime | "" |Send on connect and every N seconds (default: 60, if enabled) |
| OBK_DEV_NAME/mac | "84:e3:42:65:d1:87 " |Send on connect and every N seconds (default: 60, if enabled) |
| OBK_DEV_NAME/build | "Build on Nov 12 2022 12:39:44 version 1.0.0" |Send on connect and every N seconds (default: 60, if enabled) | 
| OBK_DEV_NAME/host |"obk_t_fourRelays" | Send on connect and every N seconds (default: 60, if enabled) |
| OBK_DEV_NAME/voltage/get |"221" | voltage from BL0942/BL0937 etc, just like current and power |
| OBK_DEV_NAME/led_enableAll/get | "1" |send when LED On/Off changes or when periodic broadcast is enabled |
| OBK_DEV_NAME/led_basecolor_rgb/get |"FFAABB" | send when LED color changes or when periodic broadcast is enabled.  |
| OBK_DEV_NAME/led_dimmer/get |"100" | send when LED dimmer changes or when periodic broadcast is enabled |
| OBK_DEV_NAME/YOUR_TOPIC/get | YOUR_VALUE | You can publish anything with 'publish [YOUR_TOPIC] [YOUR_VALUE]' command |
| tele/OBK_DEV_NAME/STATE | Tasmota JSON | OBK can publish Tasmota STATE style message if you enable TELE/etc publishes in options. This is used for compatibility with ioBroker, etc |
| stat/OBK_DEV_NAME/RESULT | Tasmota JSON | See above. You can also see [related self test code for details](https://github.com/openshwprojects/OpenBK7231T_App/blob/main/src/selftest/selftest_tasmota.c) |
| tele/OBK_DEV_NAME/SENSOR | { "Time": "1970-01-01T00:00:00", "ENERGY": { "Power": 0, "ApparentPower": 0, "ReactivePower": 0, "Factor": 0,  "Voltage": 249.932449,   "Current": 0,"ConsumptionTotal": 255.346664,"ConsumptionLastHour": 0 }} | See above. Published by power metering devices, BL0937, BL0942, etc). Make sure NTP is running to get time. |
| [similiar tasmota messages] | Tasmota JSON | See above. See [related self test code for details](https://github.com/openshwprojects/OpenBK7231T_App/blob/main/src/selftest/selftest_tasmota.c) |
| OBK_DEV_NAME/RESULT | { "IrReceived": { "Protocol": "Samsung", "Bits": 32, "Data": "0xEE110707" } } | Sent if Tasmota syntax IR publish is enabled in flags. NOTE: we may fix it and add tele prefix soon? |

  
 Publishes received by OBK device:
 | Topic        | Sample Value  | Description  |
| ------------- |:-------------:| -------------:|
| OBK_DEV_NAME/INDEX/set | "1" | Sets the channel of INDEX to given value. This can set relays and also provide DIRECT PWM access. If channel is mapped to TuyaMCU, TuyaMCU will also be updated |
| cmnd/OBK_DEV_NAME/COMMAND_TEXT | COMMAND_ARGUMENTS | That way you can execute any command supported by OpenBeken, just like in Tasmota |
| OBK_DEV_NAME/INDEX/get | no payload | You can send an empty 'get' publish to OBK device to request update on the state of given channel. OBK will reply back with the same topic but with payload representing given channel value. |
| OBK_DEV_NAME/VARIABLE/get | no payload | You can send an empty publish with VARIABLE="led_dimmer"/"led_enableAll", etc etc, to query publish of given variable manually. OBK device will reply with publishing given variable. |

# HomeAssistant Integration
MQTT based integration with Home Assistant is possible in 2 ways from the Home Assistant configuration page (`Config > Generate Home Assistant cfg`).

One can paste the generated [yaml](https://www.home-assistant.io/docs/configuration/yaml/) configuration into Home Assistant configuration manually.

Or add the devices automatically via discovery. To do click the `Start Home Assistant Discovery` button which sends outs [MQTT discovery](https://www.home-assistant.io/docs/mqtt/discovery/) messages, one for each entity (switch, light).
* The discovery topic should match the `discovery_prefix` defined in Home Assistant, the default value is `homeassistant`.
* More details about Home Assistant discovery can be found [here](https://www.home-assistant.io/docs/mqtt/discovery/).

Note: Currently, discovery is only implemented for light and relay entities.  
  
# Futher reading
  
For technical insights and generic SDK information related to Beken, WinnerMicro, Bouffallo Lab and XRadio modules, please refer:
  
https://www.elektroda.com/rtvforum/topic3850712.html
  
https://www.elektroda.com/rtvforum/topic3866123.html
  
https://www.elektroda.com/rtvforum/topic3806769.html
  
# Support project
  
If you want to support project, please donate at: https://www.paypal.com/paypalme/openshwprojects
  
Special thanks for Tasmota/Esphome/etc contributors for making a great reference for implementing Tuya module drivers 
