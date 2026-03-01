# Introduction

OpenBK7231T/OpenBeken is a Tasmota/ESPHome alternative for modern Tuya-based modules, offering MQTT and Home Assistant compatibility.
Although this repository is named `OpenBK7231T_App`, it has evolved into a multi-platform application, supporting builds for multiple chipsets from various vendors, including ESWIN, Transa Semi, Lightning Semi, Espressif, Beken, WinnerMicro, Xradiotech/Allwinner, Realtek, and Bouffalo Lab.

-   [BK7231T](https://www.elektroda.com/rtvforum/topic3951016.html) (eg [WB3S](https://developer.tuya.com/en/docs/iot/wb3s-module-datasheet?id=K9dx20n6hz5n4), [WB2S](https://developer.tuya.com/en/docs/iot/wb2s-module-datasheet?id=K9ghecl7kc479), WB2L)
-   [BK7231N](https://www.elektroda.com/rtvforum/topic3951016.html) (eg [CB2S](https://developer.tuya.com/en/docs/iot/cb2s-module-datasheet?id=Kafgfsa2aaypq), [CB2L](https://developer.tuya.com/en/docs/iot/cb2l-module-datasheet?id=Kai2eku1m3pyl), [WB2L_M1](https://www.elektroda.com/rtvforum/topic3903356.html))
-   [BK7231M](https://www.elektroda.com/rtvforum/topic4058227.html), this is a non-Tuya version of BK7231N with `00000000` keys, sometimes labelled as Belon BL2028. [example1](https://www.elektroda.com/rtvforum/topic4056377.html) [example2](https://www.elektroda.com/rtvforum/topic4086986.html)
-   T34 (flash as if [standard BK7231N](https://developer.tuya.com/en/docs/iot/t34-module-datasheet?id=Ka0l4h5zvg6j8)), see [flashing trick](https://www.elektroda.com/rtvforum/topic4036975.html)
-   BL2028N ([BL2028N is a Belon version of BK7231N](https://www.elektroda.com/rtvforum/viewtopic.php?p=20262533#20262533))
-   BK7238 (eg XH-CB3S) (see [tutorial for 1$ board](https://www.elektroda.com/rtvforum/topic4106397.html#21440428))
-   BK7231S/BK7231U - BK7231S is the original non-Tuya version of BK7231T. BK7231U is a version intended for cameras/displays. Usually comes with 00000000 keys. If flashing via SPI, can be updated to use BK7231T firmware built with zero keys. (eg C-8133U [LED driver boards](https://www.elektroda.com/rtvforum/topic4071547.html), HLK-B30 [Dev Board](https://fccid.io/2AD56HLK-B30/User-Manual/User-Manual-4894335.pdf))
-   [XR809](https://www.elektroda.com/rtvforum/topic3806769.html) (eg [XR3](https://developer.tuya.com/en/docs/iot/xr3-datasheet?id=K98s9168qi49g)) ([Example](https://www.elektroda.com/rtvforum/topic4063735.html))
-   XR806 (eg T103C-HL, [WXU](https://developer.tuya.com/en/docs/iot/wxu-module-datasheet?id=Kc2xk9qlk04so), Allwinner [Dev Board](https://www.elektroda.com/rtvforum/topic4109971.html), see [development thread](https://www.elektroda.com/rtvforum/topic4118139.html))
-   [BL602](https://www.elektroda.com/rtvforum/topic3889041.html) (eg [SM-028_V1.3](https://www.elektroda.com/rtvforum/topic3945435.html), [BW2L](https://www.elektroda.com/rtvforum/topic4111244.html)), see also [BL602 flash OBK via OTA tutorial](https://www.elektroda.com/rtvforum/topic4050297.html) (Magic Home devices only). For 2MB BL602 use [latest toml](https://www.elektroda.com/rtvforum/viewtopic.php?p=21583616#21583616) in BLDevCube.
-   [LF686](https://www.leapfive.com/wp-content/uploads/2020/09/LF686-Datasheet.pdf) (LeapFive BL602. Flash as if [standard BL602](https://www.elektroda.com/rtvforum/topic4024917.html)). BL602 factory firmware backup/restore [guide1](https://www.elektroda.com/rtvforum/topic4108535.html) [guide2](https://www.elektroda.com/rtvforum/topic4051649.html). For 2MB BL602 use [latest toml](https://www.elektroda.com/rtvforum/viewtopic.php?p=21583616#21583616) in BLDevCube.
-   [TG7100C](https://openbouffalo.org/index.php/TG7100C) (T-Mall BL602. Flash as if [standard BL602](https://www.elektroda.com/rtvforum/topic4024917.html)). BL602 factory firmware backup/restore [guide1](https://www.elektroda.com/rtvforum/topic4108535.html) [guide2](https://www.elektroda.com/rtvforum/topic4051649.html) (eg [TL7100C](https://fcc.report/FCC-ID/2AYHW-TL7100C/5154231.pdf)) ([Example](https://www.elektroda.com/rtvforum/topic4025259.html)). For 2MB BL602 use [latest toml](https://www.elektroda.com/rtvforum/viewtopic.php?p=21583616#21583616) in BLDevCube.
-   W800 (W800-C400, WinnerMicro WiFi & Bluetooth), W801, W803
-   [W600](https://www.elektroda.com/rtvforum/viewtopic.php?p=20252619#20252619) (WinnerMicro chip), W601 ([WIS600, ESP-01W](https://www.elektroda.com/rtvforum/topic3950611.html), [TW-02](https://www.elektroda.com/rtvforum/viewtopic.php?p=20239610#20239610), [TW-03](https://www.elektroda.com/rtvforum/topic3929601.html), etc)
-   [T6605](https://www.elektroda.com/rtvforum/viewtopic.php?p=21721681#21721681) Flash as WinnerMicro W600
-   [LN882H](https://www.elektroda.com/rtvforum/topic4027545.html) by Lightning Semi - [datasheet](https://www.elektroda.com/rtvforum/topic4027545.html), see [flashing how-to](https://www.elektroda.com/rtvforum/topic4028087.html), see [sample device teardown and flashing](https://www.elektroda.com/rtvforum/topic4032240.html), see [new flash tool](https://www.elektroda.com/rtvforum/topic4045532.html), see [dev board](https://www.elektroda.com/rtvforum/topic4050274.html)
-   Windows, via [simulator](https://www.elektroda.com/rtvforum/topic4046056.html)
-   ESP32 (original), ESP32-S2, ESP32-S3, ESP32-C2, ESP32-C3, ESP32-C6 (working well - guide to be released soon, [development topic](https://www.elektroda.com/rtvforum/topic4074860.html))
-   ESP8266/ESP8285 (1MB variant without OTA, 2MB or more with OTA)
-   RTL8711AM (Ameba1 family, with SDRAM only. Can't be flashed via UART, only JTAG or SPI) (eg [WRG1](https://developer.tuya.com/en/docs/iot/wrg1-datasheet?id=K97rig6mscj8e), see [development thread](https://www.elektroda.com/rtvforum/viewtopic.php?p=21452754#21452754))
-   RTL8710B (AmebaZ family) (eg [T102_V1.0](https://fccid.io/2ASKS-T102), [W302/T102_V1.0](https://fcc.report/FCC-ID/2AU7O-T102V11), [T112_V1.1](https://fccid.io/2AU7O-T102V11), [WR2](https://developer.tuya.com/en/docs/iot/wifiwr2module?id=K9605tko0juc3), [WR3E](https://developer.tuya.com/en/docs/iot/wr3e-module-datasheet?id=K9elwlqbfosbc), BW14)
-   RTL8710C/RTL8720C (AmebaZ2 family) (eg BW15, W701, W701H, W701M, W701P, [WBR2, WBR3](https://www.elektroda.com/rtvforum/topic4104395.html), [CR3L](https://developer.tuya.com/en/docs/iot/cr3l-module-datasheet?id=Ka3gl6ria8f1t), [CRG1](https://developer.tuya.com/en/docs/iot/CRG1-Module-Datasheet?id=Kbtesqh678sbe)) (see [guide](https://www.elektroda.com/rtvforum/topic4097185.html))
-   RTL8720D (AmebaD family) (eg [BW16](https://fcc.report/FCC-ID/2AHMR-BW16), BW16E, W701D, W701DH, [WBR3D](https://developer.tuya.com/en/docs/iot/wbr3d-module-datasheet?id=K9dukbbnmuw4h), [WBR3T](https://developer.tuya.com/en/docs/iot/wbr3t-module-datasheet?id=K9qs708w94ox8))
-   RTL872xCSM/RTL8720CS (AmebaCS family, firmware is compatible with RTL8720D) (eg [WBRG1](https://developer.tuya.com/en/docs/iot/wbrg1-module-datasheet?id=Ka015vo8tfztz) (see [Zigbee gateway guide](https://www.elektroda.com/rtvforum/topic4127578.html)), [WBR3N](https://developer.tuya.com/en/docs/iot/wbr3n-datasheet?id=K9qskxwpcqyaq), [WBR3S](https://developer.tuya.com/en/docs/iot/wbr3s-module-datasheet?id=K9qrt2je8wqxo))
-   RTL87x1DA (AmebaDplus family, not compatible with AmebaD/RTL8720D) (eg [BW20](https://fcc.report/FCC-ID/2ATPO-BW20/), [WR11-U](https://developer.tuya.com/en/docs/iot/wr11-u-module-datasheet?id=Kedlt4ye35tmv), [WR11-2S](https://developer.tuya.com/en/docs/iot/WR11-2S-Moudule-Datasheet?id=Kee3bl7v2agiy))
-   RTL87x0E (AmebaLite family) (eg [PKM8710ECF](https://fccid.io/2BASB-PKM8710ECF))
-   TR6260 (eg [HLK-M20](https://fccid.io/2AD56HLK-M20), XY-WE2S-A V1.1) (see [guide](https://www.elektroda.com/rtvforum/topic4093752.html))
-   ECR6600 (eg [AXYU](https://developer.tuya.com/en/docs/iot/AXYU?id=Kb0rwbv5b7aiy), [AXY2S](https://developer.tuya.com/en/docs/iot/AXY2S?id=Kb1aztk507fxf), [WG236](https://www.skylabmodule.com/product/wifi6-802-11axbluetooth-ble-5-1-combo-module-wg236), [DSM-036](https://www.dusuniot.com/product-specification/dsm-036-wi-fi6-and-ble-dual-cloud-module]), CDI-WX56600A-00, [BL-M6600XT1](https://jkrorwxhkqmllp5m-static.micyjz.com/BL-M6600XT1_datasheet_V1.0.1.0_230817-aidllBpoKqpljSRnkmnkjlnjp.pdf?dp=GvUApKfKKUAU), [HF-LPT6200](http://www.hi-flying.com/hf-lpt6200) (see [guide](https://www.elektroda.com/rtvforum/topic4111822.html))
-   TXW81X (eg TXW817-810, see [development thread](https://www.elektroda.com/rtvforum/topic4033757.html))
-   RDA5981 (eg RDA5981AM, RDA5981BM, WRD2L, TYWRD3S, HLK-M50, see [development thread](https://www.elektroda.com/rtvforum/topic4105474.html), see [guide](https://www.elektroda.com/rtvforum/topic4148573.html))
-   LN8825B (eg SCW-T503)

Please use automatically compiled binaries from the Releases tab. To build OpenBeken yourself for any supported platform, fork our version of the submodule SDK first, and then check out this app repository alongside it. Details further down. Alternatively consider using the easier [override method.](https://www.elektroda.com/rtvforum/topic4082682.html)

See our guides in Russian: [BK7231N/T34](https://www.v-elite.ru/t34), and [BL602 RGB](https://www.v-elite.ru/bl602rgb), and [Youtube guide for BK7231/T34](https://www.youtube.com/watch?v=BnmSWZchK-E)

For general information about the BK7231 family, available datasheets, pinout, peripherals, [consult our docs topic](https://www.elektroda.com/rtvforum/topic3951016.html).

# TXW81X Camera Sensor Support

OpenTXW81x camera sensor support is defined at build time (see [`project_config.h`](https://github.com/NonPIayerCharacter/OpenTXW81X/blob/main/project/project_config.h)).

Supported sensors (enabled in the current config):

-   GalaxyCore: GC0308, GC0309, GC0311, GC0312, GC0328, GC0329
-   OmniVision: OV2640, OV7660, OV7670, OV7725
-   BYD: BF3A03, BF30A2, BF2013, BF3703, BF3720
-   Hynix: HI704
-   SuperPix: SP0A19, SP0828, SP0A20, SP0718
-   Other: XC7011_GC1054, XC7011_H63, XC7016_H63, XCG532

# In Progress

To varying degrees, support for the following is in development.

-   XR872/XF16 (eg [A9 camera](https://www.elektroda.com/rtvforum/topic4074636.html))
-   BK7252U (eg [Tuya doorbell](https://www.elektroda.com/rtvforum/topic4073760.html), eg [A9 camera PCB](https://www.elektroda.com/rtvforum/topic4123266.html))
-   BK7252N (eg [A9 camera](https://www.elektroda.com/rtvforum/topic4118499.html))

# [Supported Devices/Templates List](https://openbekeniot.github.io/webapp/devicesList.html) Now with 800+ entries! (Get 🏆[free SD Card](https://www.elektroda.com/rtvforum/topic3950844.html)🏆 for submitting new one!)

We have our own interactive devices database that is maintained by users.
The database is also accessible from inside our firmware (but requires internet connection to fetch).
Have device not yet listed? HELP US, submit a teardown [here](https://www.elektroda.com/rtvforum/posting.php?mode=newtopic&f=51) and 🏆**get free SD card and gadgets set**🏆 ! Thanks to cooperation with [Elektroda.com](https://www.elektroda.com/), if you submit a _detailed_ teardown/article/review, we can send you [this set of gadgets](https://obrazki.elektroda.pl/1470574200_1670833596.jpg) for free (🚚shipping with normal letter🚚).
NOTE: Obviously almost any device with supported chip (BK7231, BL602, W600, etc is potentially supported and it's not possible to list all available devices in the market, so feel free to try, even if your device is not listed - _we are [here](https://www.elektroda.com/rtvforum/forum390.html) to help and guide you step-by-step_!)

# [Our Youtube Channel](https://www.youtube.com/@elektrodacom) (See step-by-step guides for flashing and setup)

We have our own Youtube channel with OBK-related guides. Please see our playlists:

-   [flashing guides playlist](https://www.youtube.com/playlist?list=PLzbXEc2ebpH0CZDbczAXT94BuSGrd_GoM)
-   [generic setup hints, tricks, tips](https://www.youtube.com/playlist?list=PLzbXEc2ebpH0I8m_Cfbqv1MTlQuBKYvlx)

You can help us by giving like, a comment and subscribe!

# Features

OpenBeken features:

-   Tasmota-like setup, configuration and experience on all supported platforms (supports [common Tasmota JSON over http](https://www.youtube.com/watch?v=OhdkEJ-SuTU) and MQTT, etc)
-   OTA firmware upgrade system (except XR809); to use OTA, [drag and drop](https://www.youtube.com/watch?v=OPcppowaxaA) proper OTA file on OTA field on new Web App Javascript Console
-   Online [builds for all platforms](https://github.com/openshwprojects/OpenBK7231T_App/releases) via Github, configurable [per-user build system](https://www.elektroda.com/rtvforum/topic4033833.html), also supports [Docker builds](https://github.com/openshwprojects/OpenBK7231T_App/tree/main/docker)
-   MQTT compatibility with Home Assistant (with both Yaml generator and [HA Discovery](https://youtu.be/pkcspey25V4))
-   Support for multiple relays, buttons, leds, inputs and PWMs, everything fully scriptable
-   [Driver system](https://github.com/openshwprojects/OpenBK7231T_App/blob/main/docs/drivers.md) for custom peripherals, including [TuyaMCU](https://www.elektroda.com/rtvforum/topic4038151.html) (see [Dimmer tutorial](https://www.elektroda.com/rtvforum/topic3898502.html)), I2C bus and [BL0942](https://www.elektroda.com/rtvforum/topic3887748.html), BL0937 power metering chips, Motor Driver Bridge
-   Hardware and software I2C support. For example AHT30, CHT8305, BME680, TC74 temperature and humidity sensors, MCP23017 port expander, PCF8574T LCD 2x16.
-   Hardware and software SPI support. For example BL0942SPI.
-   NTP time from network (can be used with [TH06](https://www.elektroda.com/rtvforum/topic3942730.html) and other TuyaMCU devices), can run any script on selected weekday hour:minute:second
-   Dedicated [TuyaMCU support](https://www.elektroda.com/rtvforum/topic4038151.html) with extra TuyaMCU analyzer tool for decoding new devices ([tutorial here](https://www.elektroda.com/rtvforum/topic3970199.html), code [repository here](https://github.com/openshwprojects/TuyaMCUAnalyzer))
-   Support for [TuyaMCU Battery Powered devices protocol](https://www.elektroda.com/rtvforum/topic3914412.html) (TuyaMCU enables WiFi module only to report the state, eg. for door sensors, water sensors)
-   [RGBCW LED lighting control](https://www.youtube.com/watch?v=YQdR7r6lXRY) compatible with Home Assistant (including PWM LEDs, and SM2135, BP5758, [SM15155](https://www.elektroda.com/rtvforum/topic4060227.html))
-   LittleFS integration for scripts and large files (you can [write scripts there](https://www.youtube.com/watch?v=kXi8S12tmC8), you can host a page there with [REST interface control](https://www.elektroda.com/rtvforum/topic3971355.html) of device)
-   Command line system for starting and configuring drivers, for controlling channels, etc
-   Short startup command (up to 512 characters) storage in flash config, so you can easily init your drivers (eg. BL0942) without LittleFS
-   Advanced scripting and events system (allows you to mirror Tasmota rules, for example catch button click, double click, hold)
-   Easily configurable via commands (see [tutorial](https://www.elektroda.com/rtvforum/topic3947241.html))
-   Thanks to keeping Tasmota standard, OBK has basic compatibility with [ioBroker](https://www.youtube.com/watch?v=x4p3JHXbK1E&ab_channel=Elektrodacom) and similar systems through TELE/STAT/CMND MQTT packets, Tasmota Control app is also supported
-   [DDP lighting protocol support](https://www.elektroda.com/rtvforum/topic4040325.html) ("startDriver DDP" in autoexec.bat/short startup command), works with xLights,
-   Can be scripted to even [work with shutters](https://www.elektroda.com/rtvforum/topic3972935.html), see also [second shutters script](https://www.elektroda.com/rtvforum/viewtopic.php?p=20910126#20910126)
-   Password-protected Web security [see tutorial](https://www.elektroda.com/rtvforum/topic4021160.html)
-   Advanced deep sleep with GPIO/timer wakeup and [hybrid power save systems](https://youtu.be/eupL16eB7BA), fully scriptable, can be configured to last longer than Tuya
-   Supports automatic GPIO setup with [Tuya GPIO extraction](https://www.youtube.com/watch?v=WunlqIMAdgw), [cloudcutter templates](https://www.elektroda.com/rtvforum/topic3973669.html), can also import/export [OpenBeken templates](https://openbekeniot.github.io/webapp/devicesList.html), you can also use [GPIODoctor to find out GPIO roles quickly](https://www.elektroda.com/rtvforum/topic3976371.html)
-   Advanced and custom drivers like [synchronized PWM groups with configurable dead time](https://www.elektroda.com/rtvforum/topic4025665.html)
-   WS2812B support, see [scripting tutorial](https://www.elektroda.com/rtvforum/topic4036716.html)
-   LFS and REST API allows you to create and host a custom HTML+CSS+JS page on device with a custom GUI/display of channels/TuyaMCU dpIDs, see [tutorial](https://www.elektroda.com/rtvforum/topic3971355.html) and see [sample page](https://www.elektroda.com/rtvforum/viewtopic.php?p=20932186#20932186) , and see [final version of custom TOMPD-63-WIFI page](https://www.elektroda.com/rtvforum/topic4040354.html)
-   can control 'smart lab organiser drawers' with a custom drawers driver, see [full presentation](https://www.elektroda.com/rtvforum/topic4054134.html)
-   Sensors: DHT11, DHT12, DHT21, DHT22, AHT10, AHT2X, AHT3X, SHT3X, CHT83XX, DS18B20, BMP280, BME280, BME680
-   Can draw customizable charts directly on device, see [tutorial](https://www.elektroda.com/rtvforum/topic4075289.html)
-   Berry scripting option (not enabled by default)
-   Can run on Windows with device simulator/schematic drawer, see [tutorial](https://www.elektroda.com/rtvforum/topic4046056.html)
-   Per-platform code build [self-checks](https://www.elektroda.com/rtvforum/topic4109775.html)
-   Full conversion for Tuya Matter devices [example1](https://www.elektroda.com/rtvforum/topic4074263.html), [example2](https://www.elektroda.com/rtvforum/topic4106753.html)
-   and much more

There is also a bit more outdated [WIKI](https://github.com/openshwprojects/OpenBK7231T_App/wiki/Wiki-Home)

# Building

OpenBeken supports [online builds](https://www.elektroda.com/rtvforum/viewtopic.php?p=20946719#20946719) for all platforms (BK7231T, BK7231N, BK7238, XR809, BL602, W600, W800, ESP32, RTL8710A, RTL8710B, RTL87X0C, RTL8720D/CS, TR6260, ECR6600, LN882H), but if you want to compile it yourself, see [BUILDING.md](https://github.com/openshwprojects/OpenBK7231T_App/blob/main/BUILDING.md)

# Developer Guides

-   online builds system [guide](https://www.elektroda.com/rtvforum/viewtopic.php?p=20946719#20946719)
-   how to [create custom OBK driver](https://www.elektroda.com/rtvforum/topic4056286.html)
-   how to [analyze unknown protocol with Salae logic analyzer](https://www.elektroda.com/rtvforum/topic4035491.html)
-   OBK [simulator short presentation](https://www.elektroda.com/rtvforum/topic4046056.html)
-   how to [make changes to SDK during (automated) build process](https://www.elektroda.com/rtvforum/topic4081556.html)

# Flashing

See our [GUI easy flash tool](https://github.com/openshwprojects/BK7231GUIFlashTool) for BK7231N, BK7231T, BK7231M, BK7238, BK7252N, BK7252U, BL2028N, T34, BL602, ECR6600, LN882H, RDA5981, RTL8710B, RTL8710C, RTL8720D / RTL8720CS, W800, and W600 (write-only). Also see [FLASHING.md](https://github.com/openshwprojects/OpenBK7231T_App/blob/main/FLASHING.md)

Repository of flash tools for [all other supported platforms](https://github.com/openshwprojects/FlashTools/tree/main)

TXW81X requires either CK-Link or STM32F103 (64kb flash or more). See [flashing guide](https://www.elektroda.com/rtvforum/topic4123724.html)

# [Docs - MQTT topics, Console Commands, Flags, Constants, Pin Roles, Channel Types, FAQ, autoexec.bat examples](https://github.com/openshwprojects/OpenBK7231T_App/blob/main/docs)

# Further Reading

For technical insights and generic SDK information related to Beken, WinnerMicro, Bouffalo Lab and Xradio modules, please refer to:

https://www.elektroda.com/rtvforum/topic3850712.html

https://www.elektroda.com/rtvforum/topic3866123.html

https://www.elektroda.com/rtvforum/topic3806769.html

# Support Project

❤️ Love the project? Please consider supporting it with a donation: https://www.paypal.com/paypalme/openshwprojects ❤️

Special thanks to all open-source contributors whose work has served as a valuable reference and inspiration for the development of this project.
