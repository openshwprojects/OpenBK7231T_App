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

[Russian guide for T34/BK7231N](https://www.v-elite.ru/t34)
[Russian guide for BL602](https://www.v-elite.ru/bl602rgb)

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
- Driver system for custom peripherals, including [TuyaMCU](https://www.elektroda.com/rtvforum/topic3898502.html), I2C bus and [BL0942](https://www.elektroda.com/rtvforum/topic3887748.html), BL0937 power metering chips
- Supports multiple I2C devices, like TC74 temperature sensor, MCP23017 port expander, PCF8574T LCD 2x16 (or other?), etc
- NTP time from network (can be used with [TH06](https://www.elektroda.com/rtvforum/topic3942730.html) and other TuyaMCU devices)
- basic support for [TuyaMCU Battery Powered devices protocol](https://www.elektroda.com/rtvforum/topic3914412.html) (TuyaMCU enables WiFi module only to report the state, eg. for door sensors, water sensors)
- RGBCW LED lighting control compatible with Home Assistant (both PWM LEDs, SM2135 LEDs and BP5758 LEDs)
- LittleFS integration for large files (resides in OTA memory, so you have to backup it every time you OTA)
- Command line system for starting and configuring drivers
- Short startup command (up to 512 characters) storage in flash config, so you can easily init your drivers (eg. BL0942) without LittleFS
- Advanced scripting and events system (allows you to mirror Tasmota rules, for example catch button click, double click, hold)
- Easily configurable via commands (see [tutorial](https://www.elektroda.com/rtvforum/topic3947241.html))
- Thanks to keeping Tasmota standard, OBK has basic compatibility with [ioBroker](https://www.youtube.com/watch?v=x4p3JHXbK1E&ab_channel=Elektrodacom) and similiar systems through TELE/STAT/CMND MQTT packets
- Automatic reconnect when WiFi network goes out
- and much more

# Wiki 

For more Information refer to the [WIKI](https://github.com/openshwprojects/OpenBK7231T_App/wiki/Wiki-Home)

# FAQ (Frequently Asked Questions)

<em>Q: How do I enable more logging? How to make more logs visible?</em><br>
A: First type "loglevel x" in console, where x is 0 to 7, default value is 3 (log all up to info), value 4 will also log debug logs, and value 5 will include "extradebug". Then, on online panel, also switch filter to "All" (both steps must be done for logs to show up)

<em>Q: I entered wrong SSID and my device is not accessible. How to recover?</em><br>
A: Do five short power on/power off cycles (but not too short, because device might have capacitors and they need to discharge every time). Device will go back to AP mode (aka Safe Mode)

<em>Q: I somehow lost my MAC address and I am unable to change it in Options! My MAC ends with 000000, how to fix?</em><br>
A: You have most likely overwrote the TLV header of RF partition of BK7231. For BK7231T, we have a way to restore it - open Web App, go to Flash tab, and press "Restore RF Config"

<em>Q: How do I setup single button to control two relays (first on click, second on double click)?</em><br>
A: If you set a pin role to "Button", you will get a second textbox after saving pins. First checkbox is a channel to toggle on single click, and second textbox is a channel to toggle on double click.

<em>Q: My wall touch switch reacts slowly! It's laggy, how to make it react instantly?</em><br>
A: It's like with Tasmota - go to our Options/General-Flags and set flag "6 - [BTN] Instant touch reaction instead of waiting for release (aka SetOption 13)"

<em>Q: How to enter multiple startup commands? For example, to start both NTP and BL0942 drivers?</em><br>
A: Use backlog - like in Tasmota. Open Config->Short startup command, and enter, for example: backlog startDriver BL0942; startDriver NTP; ntp_setServer 217.147.223.78

<em>Q: How to configure ping watchdog to do a relay cycle when given IP does not respond for a given amount of time?</em><br>
A: See the following example there: https://www.elektroda.com/rtvforum/viewtopic.php?p=20368812#20368812

# Building

OpenBeken supports online builds for all platforms (BK7231T, BK7231N, XR809, BL602, W800), but if you want to compile it yourself, see  [BUILDING.md](https://github.com/openshwprojects/OpenBK7231T_App/blob/main/BUILDING.md)

# Flashing for BK7231 (BK7231T and BK7231N) on Windows - easy method for beginners

Use our new BK7231 GUI Flash tool:
https://github.com/openshwprojects/BK7231GUIFlashTool

# Flashing for BK7231T (alternate method)

## UART (obsolete method; Windows only)

get BKwriter 1.60 exe (extract zip) from [here](https://github.com/openshwprojects/OpenBK7231T/blob/master/bk_writer1.60.zip)
  
Use USB to TTL converter with 3.3V logic levels, like HW 597

connect the PC to TX1 and RX1 on the bk7231 (TX2 and RX2 are optional, only for log)
  
start flash in BKwriter 1.60 (select COM port, etc)
then re-power the device (or reset with CEN by temporary connecting CEN to ground) until the flashing program continues, repeat if required.
  
## UART (obsolete method; multiplatform method, Python required)

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

Once the firmware has been flashed for the first time, it can be flashed over wifi.

Go to "Open Web Application", OTA tab, drag and drop proper RBL file on the field, press a button to start OTA proccess

## First run

At first boot, if the new firmware does not find your wifi SSID and password in the Tuya flash, it will start as an access point.

The access point will come up on 192.168.4.1, however some machines may not get an ip from it - you may need to configure your connecting for a staitc IP on that network, e.g. 192.168.4.10

Once you are connected and have an IP, go to http://192.168.4.1/index , select config then wifi, and setup your wifi.

After a reboot, the device should connect to your lan.

# Flashing for BK7231N (obsolete method)

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
  
# Building for Windows
  
It is also possible to build OpenBeken for Windows. Entire OBK builds correctly, along with script support, but MQTT from LWIP library on Windows is currently a stub and there a minor issue in Winsock code which breaks Tasmota Control compatibility. To build for Windows, open openBeken_win32_mvsc2017 in Microsoft Visual Studio Community 2017 and select configuration Debug Windows or Debug Windows Scriptonly and press build.
This should make development and testing easier.
LittleFS works in Windows build, it operates on 2MB memory saved in file, so you can even test scripting, etc
  
# Pin roles
 
You can set pin roles in "Configure Module" section or use one of predefined templates from "Quick config" subpage.
For each pin, you also set coresponding channel value. This is needed for modules with multiple relays. If you have 3 relays and 3 buttons, you need to use channel values like 1, 2, and 3. Just enter '1' in the text field, etc.
Currently available pin roles:
- Button - a typical button of Tuya device with active-low state (a button that connects IO pin to ground when pressed and also has a 10k or so pull up resistor)
- Button_n (as Button but pin logical value is inversed)
- Relay - an active-high relay. This relay is closed when a logical 1 value is on linked channel
- Relay_n (as Relay but pin logical value is inversed)
- LED - an active-high LED. The internals of "LED" are the same as of "Relay". Names are just separate to make it easier for users.
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

## [Autogenerated Complete Commands List](https://github.com/openshwprojects/OpenBK7231T_App/blob/main/docs/commands.md)
## [Autogenerated Complete Commands List - Extended Info](https://github.com/openshwprojects/OpenBK7231T_App/blob/main/docs/commands-extended.md)

There is also a conditional exec command. Example:
if MQTTOn then "backlog led_dimmer 100; led_enableAll" else "backlog led_dimmer 10; led_enableAll"
      
# Console Command Examples

| Command        | Description  |
| ------------- | -----:|
| addRepeatingEvent 15 -1 SendGet http://192.168.0.112/cm?cmnd=Power0%20Toggle | This will send a Tasmota HTTP Toggle command every 15 seconds to given device. Repeats value here is "-1" because we want this event to stay forever. |
| addEventHandler OnClick 8 SendGet http://192.168.0.112/cm?cmnd=Power0%20Toggle     | This will send a Tasmota HTTP Toggle command to given device when a button on pin 8 is clicked (pin 8, NOT channel 8) |
| addChangeHandler Channel1 != 0  SendGet http://192.168.0.112/cm?cmnd=Power0%20On | This will set a Tasmota HTTP Power0 ON command when a channel 1 value become non-zero |
| addChangeHandler Channel1 == 0  SendGet http://192.168.0.112/cm?cmnd=Power0%20Off | This will set a Tasmota HTTP Power0 OFF command when a channel 1 value become zero |
| addChangeHandler Channel1 == 1 addRepeatingEvent 60 1 setChannel 1 0 | This will create a new repeating events with 1 repeat count and 60 seconds delay everytime Channel1 becomes 1. Basically, it will automatically turn off the light 60 seconds after you turn it on. TODO: clear previous event instance? |
| addRepeatingEvent 5 -1 led_enableAll !$led_enableAll | This simple timer will toggle LED state every 5 seconds. -1 hear means infinite repeats. The ! stands for negation and $led_enableAll is a constant that you can read to get 0 or 1. It works like $CH11, $CH4 etc (any number) for accessing channel value |




# Console Command argument expansion 

  Every console command that takes an integer argument supports following constant expansion:
- $CH[CHANNEL_NUMBER] - so, $CH0 is a channel 0 value, etc, so SetChannel 1 $CH2 will get current value of Channel2 and set it to Channel 1
- $led_enableAll - this is the state of led driver, returns 1 or 0
- $led_hue
- $led_saturation
- $led_dimmer
      
  
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

| CodeName        | Description  | Screenshot  |
| ------------- |:-------------:| -----:|
| Toggle | Simple on/off Toggle | TODO |
| LowMidHigh | 3 options - Low (0), Mid (1), High (2). Used for TuyaMCU Fan Controller. | TODO |
| OffLowMidHigh | 4 options - Off(0), Low (1), Mid (2), High (3). Used for TuyaMCU Fan Controller. | TODO |
| OffLowestLowMidHighHighest | 6 options. Used for TuyaMCU Fan Controller. | TODO |
| LowestLowMidHighHighest | 5 options. Used for TuyaMCU Fan Controller. | TODO |
| Dimmer | Display slider for TuyaMCU dimmer. | TODO |
| TextField | Display textfield so you can enter any number. Used for testing, can be also used for time countdown on TuyaMCU devices. | TODO |
| ReadOnly | Display a read only value on web panel. | TODO |
| Temperature | Display a text value with 'C suffix, I am using it with I2C TC74 temperature sensor | TODO |
| temperature_div10 | First divide given value by 10, then display result value with 'C suffix. This is for TuyaMCU LCD/Clock/Calendar/Temperature Sensor/Humidity meter | TODO |
| OpenClosed | Read only value, displays "Open" if 0 and "Closed" if 1. | TODO |
| OpenClosed_Inv | Read only value, displays "Open" if 1 and "Closed" if 0. | TODO |
| humidity | Display value as a % humidity. | TODO |
| humidity_div10 | Divide by 10 and display value as a % humidity. | TODO |
| Frequency_div100 | Divide by 100 and display value as a Hz frequency. | TODO |
| Voltage_div100 | Divide by 100 and display value as a V voltage. | TODO |
| Power | Power in W. | TODO |
| Voltage_div10 | Divide by 10 and display value as a V voltage. | TODO |
| Current_div100 | Divide by 100 and display value as a A current. | TODO |
| Current_div1000 | Divide by 1000 and display value as a A current. | TODO |
| OffDimBright | 3 options - Off (0), Dim (1), Bright (2). Used for TuyaMCU LED indicator. | TODO |
  
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
