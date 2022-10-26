# Introduction

OpenBK7231T/OpenBeken is a Tasmota/Esphome replacement for new Tuya modules featuring MQTT and Home Assistant compatibility.
This repository is named "OpenBK7231T_App", but now it's a multiplatform app, supporting build for multiple separate chips:
- BK7231T ([WB3S](https://developer.tuya.com/en/docs/iot/wb3s-module-datasheet?id=K9dx20n6hz5n4), [WB2S](https://developer.tuya.com/en/docs/iot/wb2s-module-datasheet?id=K9ghecl7kc479), WB2L, etc)
- BK7231N ([CB2S](https://developer.tuya.com/en/docs/iot/cb2s-module-datasheet?id=Kafgfsa2aaypq), [CB2L](https://developer.tuya.com/en/docs/iot/cb2l-module-datasheet?id=Kai2eku1m3pyl), WB2L_M1, etc)
- T34 ([T34 is based on BK7231N](https://developer.tuya.com/en/docs/iot/t34-module-datasheet?id=Ka0l4h5zvg6j8))
- XR809 ([XR3](https://developer.tuya.com/en/docs/iot/xr3-datasheet?id=K98s9168qi49g), etc)
- BL602
- W800 (W800-C400, WinnerMicro WiFi & Bluetooth), W801
- W600 (WinnerMicro chip), W601

Please use automatically compiled binaries from the Releases tab. To build yourself for a given platform, just checkout first our version of SDK and then checkout this app repository into it, details later.

# [Supported Devices List](https://openbekeniot.github.io/webapp/devicesList.html) (Teardown + Flashing Guide + Detailed photos + Template)
We have our own interactive devices database that is maintained by users.
The database is also accessible from inside our firmware (but requires internet connection to fetch).
Have a not listed device? HELP US, submit a teardown [here](https://www.elektroda.com/rtvforum/forum507.html).
NOTE: Obviously almost any device with supported chip (BK7231, BL602, W600, etc is potentially supported and it's not possible to list all available devices in the market, so feel free to try even if your device is not listed - *we are here to help and guide you step by step*!)

# Features

OpenBeken features:
- Tasmota-like setup, configuration and experience on all supported platforms
- OTA firwmare upgrade system (for Beken chips); to use OTA, drag and drop RBL file (always RBL file) on OTA field on new Web App Javascript Console
- MQTT compatibility with Home Assistant
- Support for multiple relays, buttons, leds, inputs and PWMs
- Driver system for custom peripherals, including TuyaMCU, I2C bus and BL0942, BL0937 power metering chips
- Supports multiple I2C devices, like TC74 temperature sensor, MCP23017 port expander, PCF8574T LCD 2x16 (or other?), etc
- NTP time from network (can be used with TH06 and other TuyaMCU devices)
- basic support for TuyaMCU Battery Powered devices protocol (TuyaMCU enables WiFi module only to report the state, eg. for door sensors, water sensors)
- RGBCW LED lighting control compatible with Home Assistant (both PWM LEDs, SM2135 LEDs and BP5758 LEDs)
- LittleFS integration for large files (resides in OTA memory, so you have to backup it every time you OTA)
- Command line system for starting and configuring drivers
- Short startup command (up to 512 characters) storage in flash config, so you can easily init your drivers (eg. BL0942) without LittleFS
- Simple scripting and events system (allows you to mirror Tasmota rules, for example catch button click, double click, hold)
- Automatic reconnect when WiFi network goes out
- and much more

# Wiki 

For more Information refer to the [WIKI](https://github.com/openshwprojects/OpenBK7231T_App/wiki/Wiki-Home)

# Building

OpenBeken supports online builds for all platforms (BK7231T, BK7231N, XR809, BL602, W800), but if you want to compile it yourself, see  [BUILDING.md](https://github.com/openshwprojects/OpenBK7231T_App/blob/main/BUILDING.md)

# Flashing for BK7231T

## UART (Windows only)

get BKwriter 1.60 exe (extract zip) from [here](https://github.com/openshwprojects/OpenBK7231T/blob/master/bk_writer1.60.zip)
  
Use USB to TTL converter with 3.3V logic levels, like HW 597

connect the PC to TX1 and RX1 on the bk7231 (TX2 and RX2 are optional, only for log)
  
start flash in BKwriter 1.60 (select COM port, etc)
then re-power the device (or reset with CEN by temporary connecting CEN to ground) until the flashing program continues, repeat if required.
  
## UART (multiplatform method, Python required)

clone the repo https://github.com/OpenBekenIOT/hid_download_py
  
Use USB to TTL converter with 3.3V logic levels, like HW 597 

connect the PC to TX1 and RX1 on the bk7231 (TX2 and RX2 are optional, only for log)

start flash using:
`python uartprogram <sdk folder>\apps\<folder>\output\1.0.0\<appname>_UA_<appversion>.bin -d <port> -w`
then re-power the device (or reset with CEN temporary connecting CEN to ground) until the flashing program continues, repeat if required.

e.g.
`python uartprogram C:\DataNoBackup\tuya\tuya-iotos-embeded-sdk-wifi-ble-bk7231t\apps\my_alpha_demo\output\1.0.0\my_alpha_demo_UA_1.0.0.bin -d com4 -w`

## SPI

see https://github.com/OpenBekenIOT/hid_download_py/blob/master/SPIFlash.md

## SPI (new, tested tool for BK7231 SPI mode)
See: https://github.com/openshwprojects/BK7231_SPI_Flasher

## OTA

Once the firmware has been flashed for the first time, it can be flashed over wifi (note: change hardcoded firmware URL in new_http.c)

Setup a simple webserver to serve `<sdk folder>\apps\<folder>\output\1.0.0\<appname>_<appversion>.rbl`

Visit <ip>/ota - here start the flashing process.

## First run

At first boot, if the new firmware does not find your wifi SSID and password in the Tuya flash, it will start as an access point.

The access point will come up on 192.168.4.1, however some machines may not get an ip from it - you may need to configure your connecting for a staitc IP on that network, e.g. 192.168.4.10

Once you are connected and have an IP, go to http://192.168.4.1/index , select config then wifi, and setup your wifi.

After a reboot, the device should connect to your lan.

# Flashing for BK7231N

BKwriter 1.60 doesn't work for BK7231N for me, in BK7231 mode it errors with "invalid CRC" and in BK7231N mode it fails to unprotect the device.
For BK7231N, one should use:
  
https://github.com/OpenBekenIOT/hid_download_py
  
Flash BK7231N QIO binary, like that:
  
`python uartprogram W:\GIT\OpenBK7231N\apps\OpenBK7231N_App\output\1.0.0\OpenBK7231N_app_QIO_1.0.0.bin --unprotect -d com10 -w --startaddr 0x0`
  
  Remember - QIO binary with --unprotect and --startaddr 0x0, this is for N chip, not for T.
 
You can see an example of detailed teardown and BK7231N flashing here: https://www.elektroda.com/rtvforum/topic3874289.html
  
# Flashing for XR809
  
Get USB to UART converter, start phoenixMC.exe from OpenXR809 repository and follow this guide: https://www.elektroda.com/rtvforum/topic3806769.html
  
# Building and flashing for BL602

Follow the BL602 guide:
https://www.elektroda.com/rtvforum/topic3889041.html

# Flashing for W800/W801

Use wm_tool.exe, command line utility from this SDK https://github.com/openshwprojects/OpenW800

wm_tool.exe -c COM9 -dl W:\GIT\wm_sdk_w800\bin\w800\w800.fls

wm_tool.exe will then wait for device reset. Repower it or connect RESET to ground, then it will start the flashing

# OTA for W800/W801
  
  Create a HTTP server (maybe with Node-Red), then use the update mechanism by HTTP link. Give link to w800_ota.img file from the build. The second OTA mechanism (on javascript panel, by drag and drop) is not ready yet for W800/W801. Wait for device to restart, do not repower it manually.
  
# Testing HTTP server on Windows
  
It is also possible to build a part of our App for Windows platform. It basically creates a Windows .exe for our HTTP server, so developers can create our configurator, etc, pages faster, without having any Tuya modules at hand. For building on Windows, use MSVC projects in the app directory. It is using Winsock and creates a TCP listening socket on port 80, so make sure your machine has it free to use.
  
# Pin roles
 
You can set pin roles in "Configure Module" section or use one of predefined templates from "Quick config" subpage.
For each pin, you also set coresponding channel value. This is needed for modules with multiple relays. If you have 3 relays and 3 buttons, you need to use channel values like 1, 2, and 3. Just enter '1' in the text field, etc.
Currently available pin roles:
- Button 
- Button_n (as Button but pin logical value is inversed)
- Relay 
- Relay_n (as Relay but pin logical value is inversed)
- LED 
- LED_n (as Led but pin logical value is inversed)
- Button Toggle All - this button toggles all channels at once
- Button Toggle All_n (as above but pin logical value is inversed)
- PWM - pulse width modulation output for LED dimmers (with MQTT dimming support from Home Assistant)
- PWM_n - as above, but percentage of duty is inversed. This might be useful for some special LED drivers that are using single PWM to choose between Cool white and Warm white (it also needs setting a special flag in General options)
- WiFi LED - special LED to indicate WLan connection state. LED states are following: LED on = client mode successfully connected to your Router. Half a second blink - connecting to your router, please wait (or connection problem). Fast blink (200ms) - open access point mode. In safe mode (after failed boots), LED might not work.
- DigitalInput - this is a simple digital input pin, it sets the linked channel to current logical value on it, just like digitalRead( ) from Arduino. This input has a internal pull up resistor.
- DigitalInput_n (as above but inversed)
- ToggleChannelOnToggle - this pin will toggle target channel when a value on this pin changes (with debouncing). you can connect simple two position switch here and swapping the switch will toggle target channel relay on or off
- DigitalInput_NoPullUp - same as DigitalInput but with no programmable pull up resistor. This is used for, for example, XR809 water sensor and door sensor.
- DigitalInput_NoPullUp_n (as above but inversed)
- ADC (Analog to Digital converter) - converts voltage to channel value which is later published by MQTT and also can be used to trigger scriptable events
- AlwaysHigh - always outputs 1
- AlwaysLow - always outputs 0
- Btn_NextColor (and _n) - for RGB strip with buttons; sets next predefined color
- Btn_NextDimmer (and _n) - for RGB strip with buttons; when hold, adjusts the brightness
- IRRecv - IR receiver
- IRSend - IR sender 
- PWM_n - as PWM, but inversed value
  
# Safe mode
  
  Device is counting full boots (full boot is a boot after which device worked for 30 seconds). If you power off and on device multiple times, it will enter open access point mode and safe mode (safe mode means even pin systems are not initialized). Those modes are used to recover devices from bad configs and errors.
    
# Console Commands

There are multiple console commands that allow you to automate your devices. Commands can be entered manually in command line, can be send by HTTP (just like in Tasmota), can be send by MQTT and also can be scripted.

| Command        | Arguments          | Description  |
| ------------- |:-------------:| -----:|
| setChannel     | [ChannelIndex][ChannelValue] | Sets a raw channel to given value. Relay channels are using 1 and 0 values. PWM channels are within [0,100] range. Do not use this for LED control, because there is a better and more advanced LED driver with dimming and configuration memory (remembers setting after on/off), LED driver commands has "led_" prefix. |
| addChannel     | [ChannelIndex][ValueToAdd][ClampMin][ClampMax] | Ads a given value to the channel. Can be used to change PWM brightness. Clamp min and max arguments are optional. |
| setPinRole     | [PinRole][RoleIndexOrName] | qqq |
| setPinChannel     | [PinRole][ChannelIndex] | qqq |
| addRepeatingEvent     | [IntervalSeconds][RepeatsOr-1][CommandToRun] | Starts a timer/interval command. Use "backlog" to fit multiple commands in a single string. |
| addEventHandler     | [EventName][EventArgument][CommandToRun] | This can be used to trigger an action on a button click, long press, etc |
| addChangeHandler     | [Variable][Relation][Constant][Command] | This can listen to change in channel value (for example channel 0 becoming 100), or for a voltage/current/power change for BL0942/BL0937. This supports multiple relations, like ==, !=, >=, < etc. The Variable name for channel is Channel0, Channel2, etc, for BL0XXX it can be "Power", or "Current" or "Voltage" |
| sendGet     | [TargetURL] | Sends a HTTP GET request to target URL. May include GET arguments. Can be used to control devices by Tasmota HTTP protocol. |
| publish     | [Topic][Value] | Publishes data by MQTT. The final topic will be obk0696FB33/[Topic]/get |
| linkTuyaMCUOutputToChannel     | TODO | Used to map between TuyaMCU dpIDs and our internal channels. Mapping works both ways. |
| tuyaMcu_setBaudRate     | [BaudValue] | Sets the baud rate used by TuyaMCU UART communication. Default value is 9600. |
| led_enableAll     | [1or0] | Power on/off LED but remember the RGB(CW) values. |
| led_basecolor_rgb     | [HexValue] | Puts the LED driver in RGB mode and sets given color. |
| led_basecolor_rgbcw     | [HexValue] | TODO |
| led_temperature     | [TempValue] | Toggles LED driver into temperature mode and sets given temperature. It using Home Assistant temperature range (in the range from 154-500 defined in homeassistant/util/color.py as HASS_COLOR_MIN and HASS_COLOR_MAX) |
| led_dimmer     | [DimmerValue] | Used to dim all kinds of lights, works for both RGB and CW modes. |
| led_brightnessMult     | [Value] | Internal usage only. |
| led_colorMult     | [Value] | TODO |
| led_saturation     | [Value] | This is an alternate way to set the LED color. |
| led_hue     | [Value] | This is an alternate way to set the LED color. |
| SM2135_Map     | [Ch0][Ch1][Ch2][Ch3][Ch4] | Maps the RGBCW values to given indices of SM2135 channels. This is because SM2135 channels order is not the same for some devices. Some devices are using RGBCW order and some are using GBRCW, etc, etc. |
| SM2135_RGBCW     | [HexColor] | Don't use it. It's for direct access of SM2135 driver. You don't need it because LED driver automatically calls it, so just use led_basecolor_rgb |
| BP5758D_Map     | [Ch0][Ch1][Ch2][Ch3][Ch4] | Maps the RGBCW values to given indices of BP5758D channels. This is because BP5758D channels order is not the same for some devices. Some devices are using RGBCW order and some are using GBRCW, etc, etc. |
| BP5758D_RGBCW     | [HexColor] | Don't use it. It's for direct access of BP5758D driver. You don't need it because LED driver automatically calls it, so just use led_basecolor_rgb |
| restart     |  | Reboots the device. |
| clearConfig     |  | Clears all the device config and returns it to AP mode. |
| VoltageSet     | [Value] | Used for BL0942/BL0937/etc calibration. Refer to BL0937 guide for more info. |
| PowerSet     | [Value] | Used for BL0942/BL0937/etc calibration. Refer to BL0937 guide for more info. |
| CurrentSet     | [Value] | Used for BL0942/BL0937/etc calibration. Refer to BL0937 guide for more info. |
| DGR_SendPower     | [GroupName][ChannelValues][ChannelsCount] | Sends a POWER message to given Tasmota Device Group with no reliability. Requires no prior setup and can control any group, but won't retransmit. |
| DGR_SendBrightness     | [GroupName][Brightness] | Sends a Brightness message to given Tasmota Device Group with no reliability. Requires no prior setup and can control any group, but won't retransmit. |
| EnergyCntReset | | Used for BL0942/BL0937/etc consumption measurement data reset |
| SetupEnergyStats | [enable] [sample_time] [sample_count] [enableJSON] | Used for BL0942/BL0937/etc. Configure consumptio history stats. enable: 0/1 sample_time:10..900 sample_count: 10..180 enableJSON: 0/1 |
| PowerMax | [limit] | Used for BL0937 to setup limiter for maximal output based on device definition 3680W for 16A devices |
| ConsumptionThreshold | [threshold] | Used for BL0942/BL0937/etc for define threshold for automatic store of consumption counter to flash |

There is also a conditional exec command. Example:
if MQTTOn then "backlog led_dimmer 100; led_enableAll" else "backlog led_dimmer 10; led_enableAll"
      
# Console Command Examples

| Command        | Description  |
| ------------- | -----:|
| addRepeatingEvent 15 -1 SendGet http://192.168.0.112/cm?cmnd=Power0%20Toggle | This will send a Tasmota HTTP Toggle command every 15 seconds to given device |
| addEventHandler OnClick 8 SendGet http://192.168.0.112/cm?cmnd=Power0%20Toggle     | This will send a Tasmota HTTP Toggle command to given device when a button on pin 8 is clicked (pin 8, NOT channel 8) |
| addChangeHandler Channel1 != 0  SendGet http://192.168.0.112/cm?cmnd=Power0%20On | This will set a Tasmota HTTP Power0 ON command when a channel 1 value become non-zero |
| addChangeHandler Channel1 == 0  SendGet http://192.168.0.112/cm?cmnd=Power0%20Off | This will set a Tasmota HTTP Power0 OFF command when a channel 1 value become zero |
| addChangeHandler Channel1 == 1 addRepeatingEvent 60 1 setChannel 1 0 | This will create a new repeating events with 1 repeat count and 60 seconds delay everytime Channel1 becomes 1. Basically, it will automatically turn off the light 60 seconds after you turn it on. TODO: clear previous event instance? |

# Console Command argument expansion 

  Every console command that takes an integer argument supports following constant expansion:
- $CH[CHANNEL_NUMBER] - so, $CH0 is a channel 0 value, etc, so SetChannel 1 $CH2 will get current value of Channel2 and set it to Channel 1
      
# Scripting engine
  
  Scripting engine with threads is coming soon

# Channel Types

Channel types are often not required and don't have to be configured, but in some cases they are required for better device control from OpenBeken web panel. Channel types describes the kind of value stored in channel, for example, if you have a Tuya Fan Controller with 3 speeds control,  you can set the channel type to LowMidHigh and it will display the correct setting on OpenBeken panel.


| CodeName        | Description  | Screenshot  |
| ------------- |:-------------:| -----:|
| Toggle | Simple on/off Toggle | TODO |
| LowMidHigh | 3 options - Low (0), Mid (1), High (2). Used for TuyaMCU Fan Controller. | TODO |
| OffLowMidHigh | 4 options - Off(0), Low (1), Mid (2), High (3). Used for TuyaMCU Fan Controller. | TODO |
| Dimmer | Display slider for TuyaMCU dimmer. | TODO |
| TextField | Display textfield so you can enter any number. Used for testing, can be also used for time countdown on TuyaMCU devices. | TODO |
| ReadOnly | Display a read only value on web panel. | TODO |
| Temperature | Display a text value with 'C suffix, I am using it with I2C TC74 temperature sensor | TODO |
| temperature_div10 | First divide given value by 10, then display result value with 'C suffix. This is for TuyaMCU LCD/Clock/Calendar/Temperature Sensor/Humidity meter | TODO |
| humidity | Display value as a % humidity. | TODO |
  
# Simple TCP command server for scripting
  
  You can enable a simple TCP server in device Generic/Flags option, which will listen by default on port 100. Server can accept single connection at time from Putty in RAW mode (raw TCP connection) and accepts text commands for OpenBeken console. In future, we may add support for multiple connections at time. Server will close connection if client does nothing for some time.
  
  There are some extra short commands for TCP console:
- GetChannel [index] 
- GetReadings - returns voltage, current and power
- ShortName
the commands above return a single ASCII string as a reply so it's easy to parse.
  
  
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
  
# How to set relay state on device boot? (PowerOnState)
  
  There are two ways.
  
  1. Go to Config -> Startup state and set it from GUI here.
  
  2. Use use short startup command from Options or create "autoexec.bat" in LittleFS file system in web panel and execute commands from there.
  
  For example, for RGBCW LED, you can do startup command:
   
  > backlog led_dimmer 100; led_basecolor_rgb #FF00FF; led_enableAll 1
  
  For simple relay, in this example on channel number 5, you can do:
  
  > backlog SetChannel 5 1
  
  For direct PWM access (for single-color strips etc) you can do:
  
  > backlog SetChannel 5 90
  
  
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
  
  
# Troubles programming by UART?
  
I had some reports from users saying that they are unable to flash BK7231T with CP2102 but CH340 works. So, maybe try different USB to UART dongle. Some of them might not be reliable enough for flashing.
  
# Futher reading
  
For technical insights and generic SDK information related to Bekken and XRadio modules, please refer:
  
https://www.elektroda.com/rtvforum/topic3850712.html
  
https://www.elektroda.com/rtvforum/topic3866123.html
  
https://www.elektroda.com/rtvforum/topic3806769.html
  
# Support project
  
If you want to support project, please donate at: https://www.paypal.com/paypalme/openshwprojects
  
Special thanks for Tasmota/Esphome/etc contributors for making a great reference for implementing Tuya module drivers 
