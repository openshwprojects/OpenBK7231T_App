# Introduction

OpenBK7231T/OpenBeken is a Tasmota/Esphome replacement for new Tuya modules featuring MQTT and Home Assistant compatibility.
This repository is named "OpenBK7231T_App", but now it's a multiplatform app, supporting build for multiple separate chips:
- [BK7231T](https://www.elektroda.com/rtvforum/topic3951016.html) ([WB3S](https://developer.tuya.com/en/docs/iot/wb3s-module-datasheet?id=K9dx20n6hz5n4), [WB2S](https://developer.tuya.com/en/docs/iot/wb2s-module-datasheet?id=K9ghecl7kc479), WB2L, etc)
- [BK7231N](https://www.elektroda.com/rtvforum/topic3951016.html) ([CB2S](https://developer.tuya.com/en/docs/iot/cb2s-module-datasheet?id=Kafgfsa2aaypq), [CB2L](https://developer.tuya.com/en/docs/iot/cb2l-module-datasheet?id=Kai2eku1m3pyl), [WB2L_M1](https://www.elektroda.com/rtvforum/topic3903356.html), etc)
- [BK7231M](https://www.elektroda.com/rtvforum/topic4058227.html), this is a non-Tuya version of BK7231N with 00000000 keys, also sometimes in BL2028 flavour
- T34 ([T34 is based on BK7231N](https://developer.tuya.com/en/docs/iot/t34-module-datasheet?id=Ka0l4h5zvg6j8)), see [flashing trick](https://www.elektroda.com/rtvforum/topic4036975.html)
- BL2028N ([BL2028N is a Belon version of BK7231N](https://www.elektroda.com/rtvforum/viewtopic.php?p=20262533#20262533))
- [XR809](https://www.elektroda.com/rtvforum/topic3806769.html) ([XR3](https://developer.tuya.com/en/docs/iot/xr3-datasheet?id=K98s9168qi49g), etc)
- [BL602](https://www.elektroda.com/rtvforum/topic3889041.html) ([SM-028_V1.3 etc](https://www.elektroda.com/rtvforum/topic3945435.html)), see also [BL602 flash OBK via OTA tutorial](https://www.elektroda.com/rtvforum/topic4050297.html) (Magic Home devices only)
- [LF686](https://www.leapfive.com/wp-content/uploads/2020/09/LF686-Datasheet.pdf) (flash it [as BL602](https://www.elektroda.com/rtvforum/topic4024917.html))
- W800 (W800-C400, WinnerMicro WiFi & Bluetooth), W801
- [W600](https://www.elektroda.com/rtvforum/viewtopic.php?p=20252619#20252619) (WinnerMicro chip), W601 ([WIS600, ESP-01W](https://www.elektroda.com/rtvforum/topic3950611.html), [TW-02](https://www.elektroda.com/rtvforum/viewtopic.php?p=20239610#20239610), [TW-03](https://www.elektroda.com/rtvforum/topic3929601.html), etc)
- [LN882H](https://www.elektroda.com/rtvforum/topic4027545.html) by Lightning Semi - [datasheet](https://www.elektroda.com/rtvforum/topic4027545.html), see [flashing how-to](https://www.elektroda.com/rtvforum/topic4028087.html), see [sample device teardown and flashing](https://www.elektroda.com/rtvforum/topic4032240.html), see [new flash tool](https://www.elektroda.com/rtvforum/topic4045532.html), see [dev board](https://www.elektroda.com/rtvforum/topic4050274.html)
- Windows, via [simulator](https://www.elektroda.com/rtvforum/topic4046056.html)
- ESP32 (WIP)

Please use automatically compiled binaries from the Releases tab. To build yourself for a given platform, just checkout first our version of SDK and then checkout this app repository into it, details later.

See our guides in Russian: [BK7231N/T34](https://www.v-elite.ru/t34), and [BL602 RGB](https://www.v-elite.ru/bl602rgb), and [Youtube guide for BK7231/T34](https://www.youtube.com/watch?v=BnmSWZchK-E)

If you want to get some generic information about BK7231 modules, available datasheets, pinout, peripherals, [consult our docs topic](https://www.elektroda.com/rtvforum/topic3951016.html).


# [Supported Devices/Templates List](https://openbekeniot.github.io/webapp/devicesList.html) Now with 600+ entries! (Get 🏆[free SD Card](https://www.elektroda.com/rtvforum/topic3950844.html)🏆 for submitting new one!)
We have our own interactive devices database that is maintained by users.
The database is also accessible from inside our firmware (but requires internet connection to fetch).
Have a not listed device? HELP US, submit a teardown [here](https://www.elektroda.com/rtvforum/posting.php?mode=newtopic&f=51) and 🏆**get free SD card and gadgets set**🏆 ! Thanks to cooperation with [Elektroda.com](https://www.elektroda.com/), if you submit a detailed teardown/article/review, we can send you [this set of gadgets](https://obrazki.elektroda.pl/1470574200_1670833596.jpg) for free (🚚shipping with normal letter🚚).
NOTE: Obviously almost any device with supported chip (BK7231, BL602, W600, etc is potentially supported and it's not possible to list all available devices in the market, so feel free to try even if your device is not listed - *we are [here](https://www.elektroda.com/rtvforum/forum390.html) to help and guide you step by step*!)

# [Our Youtube Channel](https://www.youtube.com/@elektrodacom) (See step by step guides for flashing and setup)
We have our own Youtube channel with OBK-related  guides. Please see our playlists:
- [flashing guides playlist](https://www.youtube.com/playlist?list=PLzbXEc2ebpH0CZDbczAXT94BuSGrd_GoM)
- [generic setup hints, tricks, tips](https://www.youtube.com/playlist?list=PLzbXEc2ebpH0I8m_Cfbqv1MTlQuBKYvlx)

You can help us by giving like, a comment and subscribe!

# Features

OpenBeken features:
- Tasmota-like setup, configuration and experience on all supported platforms (supports [common Tasmota JSON over http](https://www.youtube.com/watch?v=OhdkEJ-SuTU) and MQTT, etc)
- OTA firmware upgrade system (for BK, W*00, BL602, LN); to use OTA, [drag and drop](https://www.youtube.com/watch?v=OPcppowaxaA) proper OTA file on OTA field on new Web App Javascript Console
- Online [builds for all platforms](https://github.com/openshwprojects/OpenBK7231T_App/releases) via Github, configurable [per-user build system](https://www.elektroda.com/rtvforum/topic4033833.html), also supports [Docker builds](https://github.com/openshwprojects/OpenBK7231T_App/tree/main/docker)
- MQTT compatibility with Home Assistant (with both Yaml generator and [HA Discovery](https://youtu.be/pkcspey25V4)) 
- Support for multiple relays, buttons, leds, inputs and PWMs, everything fully scriptable
- [Driver system](https://github.com/openshwprojects/OpenBK7231T_App/blob/main/docs/drivers.md) for custom peripherals, including [TuyaMCU](https://www.elektroda.com/rtvforum/topic4038151.html) (see [Dimmer tutorial](https://www.elektroda.com/rtvforum/topic3898502.html)), I2C bus and [BL0942](https://www.elektroda.com/rtvforum/topic3887748.html), BL0937 power metering chips, Motor Driver Bridge.
- Hardware and software I2C, supports multiple I2C devices, like TC74 temperature sensor, MCP23017 port expander, PCF8574T LCD 2x16 (or other?), etc
- Hardware and software SPI, support for SPI BL0942, etc
- NTP time from network (can be used with [TH06](https://www.elektroda.com/rtvforum/topic3942730.html) and other TuyaMCU devices), can run any script on selected weekday hour:minute:second
- Dedicated [TuyaMCU support](https://www.elektroda.com/rtvforum/topic4038151.html) with extra TuyaMCU analyzer tool for decoding new devices ([tutorial here](https://www.elektroda.com/rtvforum/topic3970199.html), code [repository here](https://github.com/openshwprojects/TuyaMCUAnalyzer))
- support for [TuyaMCU Battery Powered devices protocol](https://www.elektroda.com/rtvforum/topic3914412.html) (TuyaMCU enables WiFi module only to report the state, eg. for door sensors, water sensors)
- [RGBCW LED lighting control](https://www.youtube.com/watch?v=YQdR7r6lXRY) compatible with Home Assistant (including PWM LEDs, and SM2135, BP5758, [SM15155](https://www.elektroda.com/rtvforum/topic4060227.html) etc )
- LittleFS integration for scripts and large files (you can [write scripts there](https://www.youtube.com/watch?v=kXi8S12tmC8), you can host a page there with [REST interface control](https://www.elektroda.com/rtvforum/topic3971355.html) of device)
- Command line system for starting and configuring drivers, for controlling channels, etc
- Short startup command (up to 512 characters) storage in flash config, so you can easily init your drivers (eg. BL0942) without LittleFS
- Advanced scripting and events system (allows you to mirror Tasmota rules, for example catch button click, double click, hold)
- Easily configurable via commands (see [tutorial](https://www.elektroda.com/rtvforum/topic3947241.html))
- Thanks to keeping Tasmota standard, OBK has basic compatibility with [ioBroker](https://www.youtube.com/watch?v=x4p3JHXbK1E&ab_channel=Elektrodacom) and similar systems through TELE/STAT/CMND MQTT packets, Tasmota Control app is also supported
- [DDP lighting protocol support](https://www.elektroda.com/rtvforum/topic4040325.html) ("startDriver DDP" in autoexec.bat/short startup command), works with xLights, 
- Can be scripted to even [work with shutters](https://www.elektroda.com/rtvforum/topic3972935.html), see also [second shutters script](https://www.elektroda.com/rtvforum/viewtopic.php?p=20910126#20910126)
- Password-protected Web security [see tutorial](https://www.elektroda.com/rtvforum/topic4021160.html)
- Advanced deep sleep with GPIO/timer wakeup and [hybrid power save systems](https://youtu.be/eupL16eB7BA), fully scriptable, can be configured to last longer than Tuya
- Supports automatic GPIO setup with [Tuya GPIO extraction](https://www.youtube.com/watch?v=WunlqIMAdgw), [cloudcutter templates](https://www.elektroda.com/rtvforum/topic3973669.html), can also import/export [OpenBeken templates](https://openbekeniot.github.io/webapp/devicesList.html), you can also use [GPIODoctor to find out quickly GPIO roles](https://www.elektroda.com/rtvforum/topic3976371.html)
- Advanced and custom drivers like [synchronized PWM groups with configurable dead time](https://www.elektroda.com/rtvforum/topic4025665.html)
- WS2812B support, see [scripting tutorial](https://www.elektroda.com/rtvforum/topic4036716.html)
- LFS and REST API allows you to create and host a custom HTML+CSS+JS page on device with a custom GUI/display of channels/TuyaMCU dpIDs, see [tutorial](https://www.elektroda.com/rtvforum/topic3971355.html) and see [sample page](https://www.elektroda.com/rtvforum/viewtopic.php?p=20932186#20932186) , and see [final version of custom TOMPD-63-WIFI page](https://www.elektroda.com/rtvforum/topic4040354.html)
- can control 'smart lab organiser drawers' with a custom Drawers driver, see [full presentation](https://www.elektroda.com/rtvforum/topic4054134.html)
- DHT11 (and family) sensors support, BMP280 support, initial DS18B20 support
- Can draw customizable charts directly on device, see [tutorial](https://www.elektroda.com/rtvforum/topic4075289.html)
- Can run on Windows with device simulator/schematic drawer, see [tutorial](https://www.elektroda.com/rtvforum/topic4046056.html)
- and much more

There is also a bit more outdated [WIKI](https://github.com/openshwprojects/OpenBK7231T_App/wiki/Wiki-Home)

# Building

OpenBeken supports [online builds](https://www.elektroda.com/rtvforum/viewtopic.php?p=20946719#20946719) for all platforms (BK7231T, BK7231N, XR809, BL602, W800), but if you want to compile it yourself, see  [BUILDING.md](https://github.com/openshwprojects/OpenBK7231T_App/blob/main/BUILDING.md)

# Developer guides
- online builds system [guide](https://www.elektroda.com/rtvforum/viewtopic.php?p=20946719#20946719)
- how to [create custom obk driver](https://www.elektroda.com/rtvforum/topic4056286.html)
- how to [analyze unknown protocol with Salae logic analyzer](https://www.elektroda.com/rtvforum/topic4035491.html)
- obk [simulator short presentation](https://www.elektroda.com/rtvforum/topic4046056.html)
- how to [make changes to SDK during (automated) build process](https://www.elektroda.com/rtvforum/topic4081556.html)

# Flashing

See [our GUI easy flash tool](https://github.com/openshwprojects/BK7231GUIFlashTool), also see [FLASHING.md](https://github.com/openshwprojects/OpenBK7231T_App/blob/main/FLASHING.md)
 
# [Docs - MQTT topics, Console Commands, Flags, Constants, Pin Roles, Channel Types, FAQ, autoexec.bat examples](https://github.com/openshwprojects/OpenBK7231T_App/blob/main/docs)
   
# Further reading
  
For technical insights and generic SDK information related to Beken, WinnerMicro, Bouffallo Lab and XRadio modules, please refer:
  
https://www.elektroda.com/rtvforum/topic3850712.html
  
https://www.elektroda.com/rtvforum/topic3866123.html
  
https://www.elektroda.com/rtvforum/topic3806769.html
  
# Support project
  
If you want to support project, please donate at: https://www.paypal.com/paypalme/openshwprojects
  
Special thanks for Tasmota/Esphome/etc contributors for making a great reference for implementing Tuya module drivers 
