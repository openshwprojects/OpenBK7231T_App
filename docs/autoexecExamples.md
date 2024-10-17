# Example 'autoexec.bat' files


[Configuration for EDM-01AA-EU dimmer with TuyaMCU](https://www.elektroda.com/rtvforum/topic3929151.html)
<br>
```
// EDM-01AA-EU dimmer config
// Start TuyaMCu driver
startDriver TuyaMCU
// set channel types for OBK GUI (and Hass Discovery)
setChannelType 1 toggle
setChannelType 2 dimmer
// set TuyaMCU baud rate
tuyaMcu_setBaudRate 115200
// set TuyaMCU default wifi state 0x04, which means "paired",
// because some TuyaMCU MCUs will not report all data
// unless they think they are connected to cloud
tuyaMcu_defWiFiState 4
// set dimmer range
tuyaMcu_setDimmerRange 1 1000
// map TuyaMCU values
// linkTuyaMCUOutputToChannel dpId verType tgChannel
linkTuyaMCUOutputToChannel 1 bool 1
linkTuyaMCUOutputToChannel 2 val 2
```


[Configuration for QIACHIP Universal WIFI Ceiling Fan Light Remote Control Kit - BK7231N - CB2S with TuyaMCU](https://www.elektroda.com/rtvforum/topic3895301.html)
<br>
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


[Configuration for BK7231T LCD calendar/thermometer/hygrometer TH06 WiFi for TuyaMCU](https://www.elektroda.com/rtvforum/viewtopic.php?p=20342890#20342890)
<br>
```

startDriver TuyaMCU
startDriver NTP
// dpID 1 is tempererature div 10
setChannelType 1 temperature_div10
linkTuyaMCUOutputToChannel 1 val 1
// dpID 2 is % humidity
setChannelType 2 Humidity
linkTuyaMCUOutputToChannel 2 val 2
```


[Automatic relay turn off after given relay (aka inching)](https://www.elektroda.com/rtvforum/viewtopic.php?p=20797236#20797236)
<br>
```

// This aliased command will turn off relay on CH1 after 10 seconds
// addRepeatingEvent	[IntervalSeconds][RepeatsOr-1][CommandToRun]
alias turn_off_after_time addRepeatingEvent 10 1 setChannel 1 0
// this will run the turn off command every time that CH1 becomes 1
addChangeHandler Channel1 == 1 turn_off_after_time 

```


[Better turn off after time with timer on UI](https://www.elektroda.com/rtvforum/viewtopic.php?p=20797440#20797440)
<br>
```

// display seconds timer
setChannelType 2 TimerSeconds
// set start values as "remember in flash"
SetStartValue 1 -1
SetStartValue 2 -1

// channel 2 is timer
// channel 1 is relay

// here is countdown value in seconds - 30
alias turn_off_after_time backlog setChannel 2 30
alias on_turned_off setChannel 2 0
alias turn_off_relay setChannel 1 0
alias do_check if $CH2==0 then turn_off_relay 
alias do_tick backlog addChannel 2 -1; do_check

// this will run the turn off command every time that CH1 becomes 1
addChangeHandler Channel1 == 1 turn_off_after_time 
// this will run the turn off command every time that CH1 becomes 0
addChangeHandler Channel1 == 0 on_turned_off


again:

if $CH2!=0 then do_tick
delay_s 1
goto again
```


[Advanced turn off after time with timer on UI and timer setting on UI and kept in flash](https://www.elektroda.com/rtvforum/viewtopic.php?t=4032982&highlight=)
<br>
```

// advanced delay script
// See: https://www.elektroda.com/rtvforum/topic4032982.html

// When user toggles relay on, relay will turn off after delay
// The current delay value is displayed on www gui
// The constant delay value is displayed on www gui and fully adjustable and remembered between reboots 

// Used channels:
// Channel 1 - relay
// Channel 2 - current countdown value
// Channel 3 - constant time for countdown

// this will make channel 3 save in memory
setStartValue 3 -1
// if channel 3 is not set (default first case), set it to 15 seconds11
if $CH3==0 then setChannel 3 15
// display text field for channel 3 on www page
setChannelType 3 TextField
// set label
setChannelLabel 3 TurnOffTime
// display timer seconds (read only) for channel 2 on www page
setChannelType 2 TimerSeconds
// shorthand for setting channel 2 to value of channel 3
alias turn_off_after_time backlog setChannel 2 $CH3
// shorthand for clearing current timer value
alias on_turned_off setChannel 2 0
// shorthand to set relay off
alias turn_off_relay setChannel 1 0
// check used to turn off relay when countdown value reaches 0
alias do_check if $CH2==0 then turn_off_relay 
// an update command; it decreases current countdown by 1 and checks for 0
alias do_tick backlog addChannel 2 -1; do_check
// event triggers for channel 1 changing to 0 and 1
addChangeHandler Channel1 == 1 turn_off_after_time 
addChangeHandler Channel1 == 0 on_turned_off

// main loop - infinite
again:
if $CH2!=0 then do_tick
delay_s 1
goto again
```


[Configuration for controlling LED strip with IR receiver by TV Remote](https://www.elektroda.com/rtvforum/topic3944210.html)
<br>
```
// You must set IRRecv role for one of the pins
// IR driver will start itself automatically after reboot

// P21 role is Btn, a power button that works without scripting
// set button hold/repeat/etc times
SetButtonTimes 10 3 3
// alias to turn off LED after 4 secs (repeating event with 1 repeat)
alias add_turnoff_event addRepeatingEvent 4 1 led_enableAll 0
// button events - 23, 22, etc are pin numbers
addEventHandler OnHold 23 add_dimmer 10
addEventHandler OnHold 22 add_dimmer -10
addEventHandler OnDblClick 22 led_dimmer 5
addEventHandler OnDblClick 23 led_dimmer 100
addEventHandler OnClick 20 add_turnoff_event
// IR events
addEventHandler2 IR_Samsung 0x707 0x62 add_dimmer 10
addEventHandler2 IR_Samsung 0x707 0x65 add_dimmer -10
addEventHandler2 IR_Samsung 0x707 0x61 led_enableAll 0
addEventHandler2 IR_Samsung 0x707 0x60 led_enableAll 1
```


[Configuration for controlling Tuya 5 Speed Fan Controller by TEQOOZ - Home Assistant](https://www.elektroda.com/rtvforum/topic3908093.html)
<br>
```

// start MCU driver
startDriver TuyaMCU

// fan on/off channel
setChannelType 1 toggle

//fan speed channel
setChannelType 3 OffLowestLowMidHighHighest

// child Lock channel
setChannelType 14 toggle

// countdown channel
setChannelType 22 TextField

// remaining timer channel
setChannelType 23 TextField

// link output 1 to channel 1
// linkTuyaMCUOutputToChannel dpId varType tgChannel
linkTuyaMCUOutputToChannel 1 1 1

// link output 3 to channel 3
linkTuyaMCUOutputToChannel 3 2 3

// link output 14 to channel 14
linkTuyaMCUOutputToChannel 14 1 14

// link output 22 to channel 22
linkTuyaMCUOutputToChannel 22 2 22

// link output 23 to channel 23
linkTuyaMCUOutputToChannel 23 2 23
```


[Configuration for controlling BlitzWolf BW-AF1 air fryer](https://www.elektroda.com/rtvforum/viewtopic.php?p=20448156#20448156)
<br>
```

startDriver TuyaMCU

// cook on/off 
setChannelType 1 Toggle
setChannelLabel 1 "Cook"
linkTuyaMCUOutputToChannel 111 bool 1 
// power on/off
setChannelLabel 2 "Power"
setChannelType 2 Toggle
linkTuyaMCUOutputToChannel 101 bool 2 

// set temperature
setChannelLabel 3 "New Temperature"
setChannelType 3 TextField
linkTuyaMCUOutputToChannel 103 val 3

// currenttemperature
setChannelLabel 4 "Current Temperature"
setChannelType 4 ReadOnly
linkTuyaMCUOutputToChannel 104 val 4

// set time
setChannelLabel 5 "New Time"
setChannelType 5 TextField
linkTuyaMCUOutputToChannel 105 val 5

// read time
setChannelLabel 6 "Current Time"
setChannelType 6 ReadOnly
linkTuyaMCUOutputToChannel 106 val 6


alias cook185c15min backlog setChannel 2 1; setChannel 3 185; setChannel 5 15; setChannel 1 1
alias cook170c30min backlog setChannel 2 1; setChannel 3 170; setChannel 5 30; setChannel 1 1



startDriver httpButtons
setButtonEnabled 0 1
setButtonLabel 0 "Set 185C 15minutes"
setButtonCommand 0 "cook185c15min "
setButtonColor 0 "orange"


setButtonEnabled 1 1
setButtonLabel 1 "Set 170C 30minutes"
setButtonCommand 1 "cook170c30min "
setButtonColor 1 "orange"



```


[Configuration for Tuya ATORCH AT4P(WP/BW) Smartlife Energy monitor (BK7231N/C3BS/CH573F/BL0924)](https://www.elektroda.com/rtvforum/topic3941692.html)
<br>
```

startDriver TuyaMCU
startDriver NTP
tuyaMcu_setBaudRate 115200
setChannelType 1 toggle
setChannelType 2 Voltage_div10
setChannelType 3 Power
setChannelType 4 Current_div1000
setChannelType 5 Frequency_div100
setChannelType 6 ReadOnly
setChannelType 7 Temperature
setChannelType 8 ReadOnly
setChannelType 9 ReadOnly

//ch 1 (dpid 1) power relay control
linkTuyaMCUOutputToChannel 1 bool 1
//ch 2(dpid 20) voltage
linkTuyaMCUOutputToChannel  20 1 2
//ch 3(dpid 19) power watts
linkTuyaMCUOutputToChannel 19 1 3
//ch 4 (dpid 18)current Amps
linkTuyaMCUOutputToChannel 18 1 4
//ch 5 (dpid (133) frequency 
linkTuyaMCUOutputToChannel 133 1 5
//ch 6 (dpid  102) energy cost used
linkTuyaMCUOutputToChannel 102 1 6
// ch 7 (dpid 135) temp
linkTuyaMCUOutputToChannel 135 1 7
//ch 8 (dpid  134) power factor 
linkTuyaMCUOutputToChannel 134 raw 8
//ch 9 (dpid  123) energy consumed
linkTuyaMCUOutputToChannel 123 1 9
```


[Configuration for 4x socket + 1x USB power strip with a single button (double click, triple, etc)](https://www.elektroda.com/rtvforum/topic3941692.html)
<br>
```
// channels 1 to 5 are used
setChannelType 1 toggle
setChannelType 2 toggle
setChannelType 3 toggle
setChannelType 4 toggle
setChannelType 5 toggle
// Btn_ScriptOnly is set on P26
addEventHandler OnClick 26 ToggleChannel 1
addEventHandler OnDblClick 26 ToggleChannel 2
addEventHandler On3Click 26 ToggleChannel 3
addEventHandler On4Click 26 ToggleChannel 4
addEventHandler On5Click 26 ToggleChannel 5
```


[Script for LED acting like WiFi state LED during connecting to network but like Relay state LED when online](https://www.elektroda.com/rtvforum/viewtopic.php?p=20804036#20804036)
<br>
```

alias mode_wifi setPinRole 10 WifiLED_n
alias mode_relay setPinRole 10 LED_n

// at reboot, set WiFiLEd
mode_wifi
// then, setup handlers
addChangeHandler WiFiState == 4 mode_relay 
addChangeHandler WiFiState != 4 mode_wifi 
```


Simple example showing how to do MQTT publish on button event (double click, etc). It also includes button hold event to adjust dimmer.
<br>
```
// A simple script per user request.
// Device has single button on P26
// Device also has a relay or a light

// Btn_ScriptOnly is set on P26
// click toggles power
addEventHandler OnClick 26 POWER TOGGLE
// remaining events do MQTT publishes
// NOTE: publish [topicName] [payload]. Final topic will be like obk0696FB33/[Topic]/get
addEventHandler OnDblClick 26 publish myClickIs doubleClick
addEventHandler On3Click 26 publish myClickIs tripleClick
addEventHandler On4Click 26 publish myClickIs quadraClick
addEventHandler On5Click 26 publish myClickIs pentaClick

// NOTE: if you want also to have button hold, and this device
// is a LED device, you can use:
addEventHandler OnHold 26 led_addDimmer 10 1
// note: led_addDimmer [Delta] [Mode], mode 2 means ping pong


```


Basic Ping Watchdog usage. Ping given IP with given interval and run script if there was no ping reply within given time.
<br>
```

// this is autoexec.bat, it runs at startup, you should restart after making changes
// target to ping
PingHost 192.168.0.1
// interval in seconds
PingInterval 10
// events to run when no ping happens
addChangeHandler noPingTime > 600 reboot


```


Alternate Ping Watchdog usage.
<br>
```

// Example autoexec.bat usage
// wait for ping watchdog alert when target host does not reply for 100 seconds:

// do anything on startup
startDriver NTP
startDriver SSDP

// setup ping watchdog
PingHost 192.168.0.123
PingInterval 10

// WARNING! Ping Watchdog starts after a delay after a reboot.
// So it will not start counting to 100 immediatelly

// wait for NoPingTime reaching 100 seconds
waitFor NoPingTime 100
echo Sorry, it seems that ping target is not replying for over 100 seconds, will reboot now!
// delay just to get log out
delay_s 1
echo Reboooooting!
// delay just to get log out
delay_s 1
echo Nooow!
reboot
```


[Artificial delay for relay open (and no delay for close)](https://www.elektroda.com/rtvforum/viewtopic.php?p=20940593#20940593)
<br>
```

// Following sample shows how to turn on relay without delay, but turn it off with a delay

// Assumptions:
// - channel 1 has a relay (one of pins has role Relay and channel set to 0)
// - channel 2 is purely virtual channel used to introduce the relay

alias enable_without_delay backlog cancelRepeatingEvent 123456; setChannel 1 1
alias disable_with_delay backlog cancelRepeatingEvent 123456; addRepeatingEventID 5 1 123456 setChannel 1 0

setChannelType 2 Toggle
addChangeHandler Channel2 == 1 enable_without_delay
addChangeHandler Channel2 == 0 disable_with_delay
```


HTTP-only control of Tasmota/OBK device from OBK.
<br>
```
// HTTP calls example
// This example shows how can one OBK device control another OBK or Tasmota device via HTTP
// No MQTT is needed
// Make sure to add a button with channel 1
// Also make sure that target device IP is correct

// when channel 1 becomes 0, send OFF
addChangeHandler Channel1 == 0 SendGet http://192.168.0.112/cm?cmnd=Power0%20OFF
// when channel 1 becomes 1, send ON
addChangeHandler Channel1 == 1 SendGet http://192.168.0.112/cm?cmnd=Power0%20ON
```


NTP and 'waitFor' command example. You can use 'waitFor NTPState 1' in autoexec.bat to wait for NTP sync. After that, you can be sure that correct time is set. 'waitFor' will block execution until given event.
<br>
```
// do anything on startup
startDriver NTP
startDriver SSDP


// now you can wait for NTP to sync
// This will block execution until NTP get correct time
// NOTE: you can also do 'waitFor MQTTState 1' as well, if you want MQTT state 1
waitFor NTPState 1

// default setting
Dimmer 5
Power ON

// now you can do something basing on time
// idk, it's just a simple example, note that 
// next Dimmer setting will overwite previos setting
if $hour<8 then Dimmer 50
if $hour>=20 then Dimmer 50
if $hour>=23 then Dimmer 90

```


MQTT and 'waitFor' command example. You can use 'waitFor MQTTState 1' in autoexec.bat to wait for MQTT connection. After that, you can be sure that MATT is online. 'waitFor' will block execution until given event.
<br>
```
// do anything on startup
startDriver NTP
startDriver SSDP

// now wait for MQTT
waitFor MQTTState 1
// extra delay, to be sure
delay_s 1
publish myVariable 2022
// you can publish again just to be sure
// delay_s 1
// publish myVariable 2022

// if you have a battery powered device, you can now uncomment this line:
// Deep sleep (shut down everything) and reboot automatically after 600 seconds
//DeepSleep 600

```


WaitFor advanced syntax.
<br>
```

// Waitfor syntax samples
// WaitFor can support following operators:
// - no operator (default, it means equals)
// - operator < (less)
// - operator > (more)
// - operator ! (not requal)

// default usage - wait for NoPingTime to reach exact value 100
waitFor NoPingTime 100

// extra operator - wait for NoPingTime to become less than 100
// (this will be triggered anytime variable changes to value less than 100)
waitFor NoPingTime < 100

// extra operator - wait for NoPingTime to become more than 100
// (this will be triggered anytime variable changes to value more than 100)
waitFor NoPingTime > 100

// extra operator - wait for NoPingTime to become different than 100
// (this will be triggered anytime variable changes to value other than 100)
waitFor NoPingTime ! 100


```


This script will use NTP and 'addClockEvent' to change light styles in the morning and in the evening. Here you can see how  'waitFor NTPState 1' and 'addClockEvent' and $hour script functions/variables are used.
<br>
```
// setup NTP driver
startDriver ntp
// set your time zone
ntp_timeZoneOfs 10:00

// create command aliases for later usage
alias day_lights backlog led_temperature 200; led_dimmer 100; echo lights_day
alias night_lights backlog led_temperature 500; led_dimmer 50; echo lights_night

// at given hour, change lights state
addClockEvent 06:01 0xff 1 day_lights 
addClockEvent 20:01 0xff 1 night_lights 

// wait for NTP sync
waitFor NTPState 1
// after NTP is synced, just after reboot, adjust light states correctly
if $hour>=06&&$hour<21 then day_lights
if $hour>=21||$hour<06  then night_lights


```


Alternate 'addClockEvent' demo - just like previous one, but written by using AddEventHandler instead of waitFor
<br>
```
startDriver ntp
ntp_timeZoneOfs 10:00

alias day_lights backlog led_temperature 200; led_dimmer 100; echo lights_day
alias night_lights backlog led_temperature 500; led_dimmer 50; echo lights_night
alias set_now_colour backlog if $hour>=06&&$hour<21 then day_lights; if $hour>=21||$hour<06 then night_lights

addClockEvent 06:01 0xff 1 day_lights 
addClockEvent 20:01 0xff 1 night_lights 

AddEventHandler NTPState 1 set_now_colour


```


A more complete demo with the addition of: An NTP server configured on the LAN, off/low/high modes for the lights, make use of sunset event and constant to determine when the light should start to be on.
<br>
```
PowerSave 1
startDriver ntp
ntp_setServer 192.168.1.12
ntp_timeZoneOfs 11
ntp_setLatlong -33.868820 151.209290

// Use channel 10 to maintain sunset time (The time when the light should turn on)
setChannelLabel 10  "<b><span style='color:orange'\>Light ON (Civil Sunset)</span></b>"
setChannelType 10 TimerSeconds

// Use channel 11 as a hidden register to store current time as TimerSeconds
setChannelType 11 TimerSeconds
setChannelPrivate 11 1
setChannelVisible 11 0

alias high_lights backlog led_temperature 300; led_dimmer 30; led_enableAll 1; echo lights_set_high
alias low_lights backlog led_temperature 500; led_dimmer 5; led_enableAll 1; echo lights_set_low
alias off_lights backlog led_enableAll 0; echo lights_set_off

alias calc_sunset backlog setChannel 10 $sunset+1800; removeClockEvent 4; addClockEvent $CH10 0xff 4 high_lights; echo Lights will turn on at $CH10

addClockEvent 3:30 0xff 1 calc_sunset
addClockEvent 21:00 0xff 2 low_lights
addClockEvent 23:00 0xff 3 off_lights

waitFor NTPState 1

calc_sunset

// set the current time as TimerSeconds in register for checks below
setChannel 11 $hour*3600+$minute*60

// Set initial light state to match the above clock events
// sunset - 21:00   lights set to high (evening activities)
// 21:00 - 23:00    lights set to low (sleepy mode: warmest temperature and very dim)
// 23:00 - sunset   lights turned off (bedtime and day time the lights should be off)
if $CH11>=$CH10&&$hour<21 then high_lights
if $hour>=21&&$hour<23 then low_lights
if $hour>=23||$CH11<$CH10 then off_lights
```


Simple UART log redirection to UART1 instead of UART2 on Beken with UART command line support
<br>
```
// Here is how you can get log print on UART1, instead of default UART2 on Beken

// Enable "[UART] Enable UART command line"
// this also can be done in flags, enable command line on UART1 at 115200 baud
SetFlag 31 1
// UART1 is RXD1/TXD1 which is used for programming and for TuyaMCU/BL0942,
// but now we will set that UART1 is used for log
logPort 1 

```


Simple single LED blink on device boot
<br>
```
// this script just manually turns on and later off LED on device boot
// It assumes that you have a LED on channel 4

// turn on LED on ch 4
setChannel 4 1
// schedule an event, after 2 seconds, turn off LED (this is non-blocking call)
// NOTE: addRepeatingEvent [RepeatTime] [RepeatCount]
addRepeatingEvent 2 1 setChannel 4 0
```


[Advanced config for TuyaMCU power meter and electric car charging limit driver](https://www.elektroda.com/rtvforum/topic3936455.html)
<br>
```
// Config for TAC2121C-like TuyaMCU power meter device
// Also a charging limit driver is setup for charging electric cars
// See: https://www.elektroda.com/rtvforum/topic3996220.html
// See: https://obrazki.elektroda.pl/6653013600_1692204415.png
startDriver httpButtons
startDriver TuyaMCU
startDriver NTP

linkTuyaMCUOutputToChannel 16 1 1
setChannelType 1 toggle

setChannelType 2 Voltage_div10

setChannelType 3 Power

setChannelType 4 Current_div1000

linkTuyaMCUOutputToChannel 1 0 5
setChannelType 5 EnergyTotal_kWh_div100

linkTuyaMCUOutputToChannel 6 RAW_TAC2121C_VCP

linkTuyaMCUOutputToChannel 101 0 6
setChannelType 6 Temperature_div10

setChannelType 7 Error


// *******************************
// Charger setup 

// We use toggle 29 to enable/disable the charger mode
// while enabling, we start a new cycle immediately

setChannelType 29 Toggle
SetChannelLabel 29 "Charger mode"
addChangeHandler Channel29 == 1 startchargermode
addChangeHandler Channel29 == 0 stopchargermode

// disable driver by default after restart 
SetChannel 29 0

// (re)start session when toggled
addChangeHandler Channel1 == 1 startsession  
addChangeHandler Channel1 == 0 stopsession

// disable output by default after restart 
setChannel 1 0

// we keep the maximum time in channel 30 ( $CH30)
setChannelType 30 TextField
SetChannelLabel 30 "30 : Max Power (Wh)"
setChannel 30 2

// we keep the maximum time in channel 31 ( $CH31)
setChannelType 31 TextField
SetChannelLabel 31 "31 : Max Time (Seconds)"
setChannel 31 60


// chargermode

alias startchargermode backlog startDriver ChargingLimit; publish charging/driver "started";  startsession; 
alias stopchargermode backlog stopDriver ChargingLimit; publish charging/driver "stopped"; stopsession;

// charging sessions
alias startsession backlog setChannel 1 1; chSetupLimit 3 $CH30 $CH31 "POWER OFF"; publish charging/status "started" ; publish charging/status/starthour $hour ; publish charging/status/startminute $minute ; publish charging/status/stophour 0; publish charging/status/stopminute 0;

alias stopsession backlog setChannel 1 0; publish charging/status "stopped"; publish charging/status/stophour $hour; publish charging/status/stopminute $minute;


// some presets 

alias presetguests backlog setChannel 30 5000; setChannel 31 3600;
alias presetfriends backlog setChannel 30 10000; setChannel 31 7200;
alias presettest backlog setChannel 30 2; setChannel 31 10;

setButtonEnabled 0 1
setButtonLabel 0 "Preset guests 5 KWh / 1 Hour" 
setButtonCommand 0 presetguests
setButtonColor 0 "#56b08f"

setButtonEnabled 1 1
setButtonLabel 1 "Preset Friends 10 KWh / 2 Hours" 
setButtonCommand 1 presetfriends
setButtonColor 1 "#56b08f"

setButtonEnabled 2 1
setButtonLabel 2 "Preset Test 2 Wh / 10 sec" 
SetButtonCommand 2 presettest
setButtonColor 2 "#56b08f"



```


Advanced I2C driver for multiple devices on single bus - TC74 example
<br>
```
// TC74A5 (check address in datasheet! is on SoftSDA and SoftSCL with 10k pull up resistors)
startDriver I2C
setChannelType 6 temperature
// Options are: I2C1 (BK port), I2C2 (BK port), Soft (pins set in configure module)
// 0x4D is device address 
// 6 is target channel
addI2CDevice_TC74 Soft 0x4D 6
```


Deep sleep usage with SHT30 sensor and data reporting via HTTP GET
<br>
```
// source: https://www.elektroda.com/rtvforum/viewtopic.php?p=20693161#20693161
// NOTE: SHT3X is configured to store temp in channel2 and humidity in channel 3

// start driver
startDriver SHT3X
//hold button to get into safe mode
addEventHandler OnHold 20 SafeMode
// wait for wifi to become WIFI_STA_CONNECTED
waitFor WiFiState 4
Battery_Setup 2500 4200 2.29 2400 4096
battery_measure
publishFloat "voltage" $CH4/1000
//publishFloat "battery" $CH4/25
// extra wait
delay_s 5
// measure
SHT_Measure
// send data
SendGET http://address:port/sensor1.php?temp=$CH2&hum=$CH3&bat=$voltage
// wait for GET to be sent
delay_s 5
// sleep for 120 seconds and then wake up (from blank state)
DeepSleep 120



```


[Deep sleep usage for water sensor with PWM buzzer](https://www.elektroda.com/rtvforum/viewtopic.php?p=21096228#21096228)
<br>
```

// Water Sensor with deep sleep and buzzer (PWM)
// See related topic: https://www.elektroda.com/rtvforum/viewtopic.php?p=21096228#21096228


Battery_Setup 2000 3000 2 2400 4096
//measure batt every 2s
Battery_cycle 2
//mqtt_broadcastInterval 1
//mqtt_broadcastItemsPerSec 5
addEventHandler OnHold 10 SafeMode 5

setChannelLabel 1 Water

// now wait for MQTT
waitFor MQTTState 1
// extra delay, to be sure
delay_s 1
// publish water state at least once after boot
publish 1 $CH1

// if water detected, keep cycling (drains battery)
// also useful for configuring the device or doing OTA
mainLoop:
delay_s 1
if $CH1!=1 then "goto sirenOn" else "goto sleep"


// turn on the siren and goes into siren loop
sirenOn:
setChannel 6 10

// siren loop repeats while water is detected
sirenLoop:
backlog PWMfrequency 460;delay_ms 50
backlog PWMfrequency 491;delay_ms 50
backlog PWMfrequency 521;delay_ms 50
backlog PWMfrequency 550;delay_ms 50
backlog PWMfrequency 577;delay_ms 50
backlog PWMfrequency 601;delay_ms 50
backlog PWMfrequency 621;delay_ms 50
backlog PWMfrequency 638;delay_ms 50
backlog PWMfrequency 650;delay_ms 50
backlog PWMfrequency 657;delay_ms 50
backlog PWMfrequency 660;delay_ms 50
backlog PWMfrequency 657;delay_ms 50
backlog PWMfrequency 650;delay_ms 50
backlog PWMfrequency 638;delay_ms 50
backlog PWMfrequency 621;delay_ms 50
backlog PWMfrequency 601;delay_ms 50
backlog PWMfrequency 577;delay_ms 50
backlog PWMfrequency 550;delay_ms 50
backlog PWMfrequency 521;delay_ms 50
backlog PWMfrequency 491;delay_ms 50
if $CH1!=1 then "goto sirenLoop" else "goto sirenOff"

// turn off the siren and go into main loop
sirenOff:
setChannel 6 0
goto mainLoop

sleep:
delay_s 5
// All good, sleep for 1 day or next water event
PinDeepSleep 86400
```


Manual flash save example for TuyaMCU - using special Channels 200, 201, etc
<br>
```
// Read full explanation here: https://www.elektroda.com/rtvforum/topic4003825.html
// NOTE: you can also use the feature of TuyaMCU itself (not OBK) to save relay state, but it's not always present
// Anyway, refer to article above.
// Here's the script:

startDriver TuyaMCU

setChannelType 1 toggle
setChannelType 2 toggle
setChannelType 3 toggle
setChannelType 4 toggle


setChannelType 7 TextField
setChannelType 8 TextField
setChannelType 9 TextField
setChannelType 10 TextField
setChannelType 14 TextField

// linkTuyaMCUOutputToChannel dpId varType tgChannel
linkTuyaMCUOutputToChannel 1 1 1
linkTuyaMCUOutputToChannel 2 1 2
linkTuyaMCUOutputToChannel 3 1 3
linkTuyaMCUOutputToChannel 4 1 4
linkTuyaMCUOutputToChannel 7 2 7
linkTuyaMCUOutputToChannel 8 2 8
linkTuyaMCUOutputToChannel 9 2 9
linkTuyaMCUOutputToChannel 10 2 10
linkTuyaMCUOutputToChannel 14 4 14


delay_s 1
echo Restoring states: $CH201 $CH202 $CH203 $CH204
setChannel 1 $CH201
delay_s 0.1
setChannel 2 $CH202
delay_s 0.1
setChannel 3 $CH203
delay_s 0.1
setChannel 4 $CH204

// when channel 1 changes, save it to flash channel 201
addEventHandler OnChannelChange 1 setChannel 201 $CH1
// when channel 2 changes, save it to flash channel 202
addEventHandler OnChannelChange 2 setChannel 202 $CH2
// when channel 3 changes, save it to flash channel 202
addEventHandler OnChannelChange 3 setChannel 203 $CH3
// when channel 4 changes, save it to flash channel 204
addEventHandler OnChannelChange 4 setChannel 204 $CH4

```


[Shift register setup, usage and control with channels](https://www.elektroda.com/rtvforum/viewtopic.php?p=20533505#20533505)
<br>
```
// Start shiftRegister driver and map pins, also map channels to outputs
// startDriver ShiftRegister [DataPin] [LatchPin] [ClkPin] [FirstChannel] [Order] [TotalRegisters] [Invert]
startDriver ShiftRegister 24 6 7 10 1 1 0
// If given argument is not present, default value is used
// First channel is a first channel that is mapped to first output of shift register.
// The total number of channels mapped is equal to TotalRegisters * 8, because every register has 8 pins.
// Order can be 0 or 1, MSBFirst or LSBFirst

// To make channel appear with Toggle on HTTP panel, please also set the type:
setChannelType 10 Toggle
setChannelType 11 Toggle
setChannelType 12 Toggle
setChannelType 13 Toggle
setChannelType 14 Toggle
setChannelType 15 Toggle
setChannelType 16 Toggle
setChannelType 17 Toggle

```


[Custom countdown/timer system with HTTP GUI for TuyaMCU relay](https://www.elektroda.com/rtvforum/topic4009196.html)
<br>
```
// See tutorial: https://www.elektroda.com/rtvforum/topic4009196.html

// start TuyaMCU driver
startDriver TuyaMCU
// set TuyaMCU baud rate
tuyaMcu_setBaudRate 115200
// set TuyaMCU default wifi state 0x04, which means "paired",
// because some TuyaMCU MCUs will not report all data
// unless they think they are connected to cloud
tuyaMcu_defWiFiState 4
// start NTP driver, so we have time from web
startDriver NTP
// set channel 1 to display toggle
setChannelType 1 Toggle
// Map given dpID to channel 1, dpID type bool
// linkTuyaMCUOutputToChannel [dpId][varType][channelID]
linkTuyaMCUOutputToChannel 1 bool 1
// channel 2 will be temperature
setChannelType 2 temperature
// Map given dpID to channel 2, dpID type value
linkTuyaMCUOutputToChannel 24 val 2

// setup cosmetics for channel 10
setChannelLabel 10  "<b><span style='color:orange'\>Time left</span></b>"
// set channel 10 to display formatted time
setChannelType 10 TimerSeconds

// create command aliases for enable + setup delay (in channel 10) sequences
alias enable_30min backlog POWER ON; setChannel 10 30*60
alias enable_30sec backlog POWER ON; setChannel 10 30

// set channel 11 label, channel 11 will allow to enter hours value of new timespan
setChannelLabel 11  "<b><span style='color:orange'\>Custom time setting [hours]</span></b>"
setChannelType 11 TextField

// start driver handling extra buttons on http page
startDriver httpButtons
// button 0 - set enable to 1
setButtonEnabled 0 1
// set button 0 label
setButtonLabel 0 "Enable for 30 minutes"
// set button 0 command (see alias created above)
setButtonCommand 0 "enable_30min"
// set button 0 color (CSS color code)
setButtonColor 0 "green"

// when anything sets relay to off, also clear timer
addChangeHandler Channel1 == 0 setChannel 10 0
// if user manually sets channel 1 with no timer, then set default time
addChangeHandler Channel1 == 1  if $CH10==0 then "setChannel 10 1234"

// just like for button 0, also setup button 1
setButtonEnabled 1 1
setButtonLabel 1"Enable for 60 minutes"
setButtonCommand 1 "enable_60min"
setButtonColor 1 "green"

// just like for button 0, also setup button 2
startDriver httpButtons
setButtonEnabled 2 1
setButtonLabel 2 "Enable for 30 sec"
setButtonCommand 2 "enable_30sec"
setButtonColor 2 "green"


// every 10 seconds, request update from TuyaMCU
addRepeatingEvent 10 -1 tuyaMcu_sendQueryState

// now this part runs in a loop
again:
// wait one secnd
delay_s 1
// if channel sued for timer has reached 0, time off
if $CH10<=0 then POWER OFF

// subtract one from time, clamp at 0 and 99999
addChannel 10 -1 0 999999

// continue the loop forever
goto again



```


[Setup for EZB-WBZS1H16N-A V1.0 Tuya mini smart switch showing sunrise/sunset events](https://www.elektroda.com/rtvforum/topic3967141.html)
<br>
```
NOTE: Set Time offset, latitude, longitude accordingly

// autoexec for mini smart switch
PowerSave 1
addEventHandler OnHold 8 SafeMode
startDriver ntp
ntp_timeZoneOfs -8
ntp_setLatlong 44.002130 -123.091473
setPinRole 6 WifiLed_n
setPinRole 8 Btn_Tgl_All
setPinRole 14 TglChanOnTgl
setPinChannel 14 0
setPinRole 15 Rel
setPinChannel 15 0
removeClockEvent 12
removeClockEvent 13
waitFor NTPState 1
addClockEvent sunrise 0x7f 12 POWER OFF
addClockEvent sunset 0x7f 13 POWER ON

```


[Setup for PJ-MGW1103 T-Clamp TuyaMCU with a device state request demonstation](https://www.elektroda.com/rtvforum/viewtopic.php?p=20882983#20882983)
<br>
```

// config for PJ-MGW1103 CT-Clamp Energy Meter by Tuya, BL0942, CB2S Components
// Source: https://www.elektroda.com/rtvforum/viewtopic.php?p=20882983#20882983
// Teadown: https://www.elektroda.com/rtvforum/topic3946128.html
startDriver TuyaMCU
tuyaMcu_setBaudRate 115200
tuyaMcu_defWiFiState 4

setChannelType 2 Voltage_div10
setChannelType 3 Power_div10
setChannelType 4 Current_div1000

// linkTuyaMCUOutputToChannel dpId variableType tgChannel
//ch 2(dpid 20) voltage
linkTuyaMCUOutputToChannel  20 1 2
//ch 3(dpid 19) power watts
linkTuyaMCUOutputToChannel 19 1 3
//ch 4(dpid 18) current in mA
linkTuyaMCUOutputToChannel 18 1 4

// every 15 seconds query measurements from the MCU
addRepeatingEvent 5 -1 tuyaMcu_sendQueryState
```


[TOMPD-63-WIFI TuyaMCU power meter config with alternate HTML panel hosted in LittleFS](https://www.elektroda.com/rtvforum/topic4040354.html)
<br>
```

// This script provides extra REST page for hosting in LittleFS
// Please see: https://www.elektroda.com/rtvforum/topic4040354.html
// Script taken from: https://www.elektroda.com/rtvforum/viewtopic.php?p=20990058#20990058
// Download HTML there.

// Clear all previous definitions
clearIO

// clear flags
// flags 0

// set flag 46
SetFlag 46 1

// start TuyaMCU driver
startDriver TuyaMCU
tuyaMcu_setBaudRate 9600

// This one is better than tuyaMcu_defWiFiState 4;  MQTTState 1 = WiFiState 4
// issuing of tuyaMcu_defWiFiState 4 continues the script,
// but doesnt report to MQTT since there is still no connection.
// if you didn't setup MQTT connection then issue tuyaMcu_defWiFiState 4
// and comment waitFor MQTTState 1

waitFor MQTTState 1
// tuyaMcu_defWiFiState 4

// Breaker Id - Dpid 19 "breaker_id" -> channel 0
linkTuyaMCUOutputToChannel 19 str 0
setChannelType 0 ReadOnly
setChannelLabel 0 "Breaker ID"

// Main Relay - Dpid 16 "switch" -> Channel 1
linkTuyaMCUOutputToChannel 16 bool 1
setChannelType 1 toggle
setChannelLabel 1 "Relay"

// Prepayment on-off - Dpid 11 "switch_prepayment" -> channel 2
linkTuyaMCUOutputToChannel 11 bool 2
setChannelType 2 toggle
setChannelLabel 2 "Prepayment"

// Clear Energy Counters - Dpid 12 "clear_energy" -> channel 3
linkTuyaMCUOutputToChannel 12 bool 3
setChannelType 3 toggle
setChannelLabel 3 "Clear Energy Counters"

// Total energy - Dpid 1 "total_forward_energy" -> channel 4
linkTuyaMCUOutputToChannel 1 val 4
setChannelType 4 EnergyTotal_kWh_div100
setChannelLabel 4 "Total Energy"

// Measurements - Dpid 6 "phase_a" - channel RAW_TAC2121C_VCP -> 5,6,7
// TAC2121C VoltageCurrentPower Packet
// This will automatically set voltage, power and current
linkTuyaMCUOutputToChannel 6 RAW_TAC2121C_VCP
setChannelType 5 Voltage_div10
setChannelLabel 5 "Voltage"
setChannelType 6 Power
setChannelLabel 6 "Power"
setChannelType 7 Current_div1000
setChannelLabel 7 "Current"

// Leakage current - Dpid 15 "leakage_current" -> channel 8
linkTuyaMCUOutputToChannel 15 val 8
setChannelType 8 Current_div1000
setChannelLabel 8 "Leakage Current"

// Fault - Dpid 9 "fault" -> channel 9
linkTuyaMCUOutputToChannel 9 raw 9
setChannelType 9 ReadOnly
setChannelLabel 9 "Fault"

// Settings 2 - Dpid 18 "alarm_set_2" -> channel 10
linkTuyaMCUOutputToChannel 18 raw 10
setChannelType 10 ReadOnly
setChannelLabel 10 "UV, OV, Max. Current Protections Settings"

// Prepaid energy - Dpid 13 "balance_energy" -> channel 11
linkTuyaMCUOutputToChannel 13 val 11
setChannelType 11 EnergyTotal_kWh_div100
setChannelLabel 11 "Total Prepaid Energy"

// The above are read only channels. Except Settings 2 (mapped on channel 10), but since we cannot set it due to wrong parse of TuyaMCU driver - for now read only

// Charge Prepayment - Dpid 14 "charge_energy" - channel 12
linkTuyaMCUOutputToChannel 14 val 12
setChannelType 12 TextField
setChannelLabel 12 "Purchased Energy [kWh*100], i.e. 1kWh = 100"

// Settings 1 - Dpid 17 "alarm_set_1" - channel 13
linkTuyaMCUOutputToChannel 17 val 13
setChannelType 13 TextField
setChannelLabel 13 "Leakage Current Protection Settings"

// Protection Reaction Time - Dpid 104 "over_vol_protect_time" (applied for all protections, regardless the name of DpID) -> channel 14
linkTuyaMCUOutputToChannel 104 val 14
setChannelType 14 TextField
setChannelLabel 14 "Protection Reaction Time [s]"

// Protection Recovery Time - Dpid 105 "over_vol_recovery_time" (applied for all protections, regardless the name of DpID) -> channel 15
linkTuyaMCUOutputToChannel 105 val 15
setChannelType 15 TextField
setChannelLabel 15 "Protection Recovery Time [s]"

// NOTE: addRepeatingEvent [RepeatTime] [RepeatCount]
// code below will forever Send query state command every 5 seconds
// addRepeatingEvent 5 -1 tuyaMcu_sendQueryState 

// AngelOfDeath - We don't need it forever, since TuyaMCU sends everything when necessary
// we need it just first time to obtain initial status. Some dpIDs not reported without asking
tuyaMcu_sendQueryState

```


[PJ-MGW1103 CT-Clamp Energy Meter sample for combining two dpIDs (sign and value) into one channel](https://www.elektroda.com/rtvforum/viewtopic.php?p=21125206#21125206)
<br>
```
// For TuyaMCU
// One dpID is sign, second is value
// Taken from: https://www.elektroda.com/rtvforum/viewtopic.php?p=21125206#21125206

//power A
setChannelType 3 Power_div10
SetChannelLabel 3 "Power(A)"
linkTuyaMCUOutputToChannel  101 1 3

//Direction A
setChannelType 4 ReadOnly
SetChannelLabel 4 "Direction(A)"
linkTuyaMCUOutputToChannel  102 1 4

//Corrected Power
setChannelType 19 ReadOnly
SetChannelLabel 19 "Corrected Power(A)"

// channel 4 is sign
// channel 3 is unsigned val
alias positive setChannel 19 $CH3
alias negative setChannel 19 $CH3*-1
alias myset if $CH4==0 then positive else negative
// now trigger it on every change
addEventHandler OnChannelChange 3 myset 
addEventHandler OnChannelChange 4 myset 

```


[Scripting custom light animation/styles for TuyaMCU](https://www.elektroda.com/rtvforum/topic4014389.html)
<br>
```
// See: https://www.elektroda.com/rtvforum/topic4014389.html

startDriver httpButtons
setButtonEnabled 0 1
setButtonLabel 0 "Music mode"
setButtonCommand 0 "tuyaMcu_sendState 21 4 3"

setButtonEnabled 1 1
setButtonLabel 1 "Light mode"
setButtonCommand 1 "tuyaMcu_sendState 21 4 1" 
 
setButtonEnabled 2 1
setButtonLabel 2 "Curtain"
setButtonCommand 2 "startScript autoexec.bat do_cur"
 
setButtonEnabled 3 1
setButtonLabel 3 "Collision"
setButtonCommand 3 "startScript autoexec.bat do_col"
 
setButtonEnabled 4 1
setButtonLabel 4 "Rainbow"
setButtonCommand 4 "startScript autoexec.bat do_rai"
 
setButtonEnabled 5 1
setButtonLabel 5 "Pile"
setButtonCommand 5 "startScript autoexec.bat do_pil"
 
setButtonEnabled 6 1
setButtonLabel 6 "Firework"
setButtonCommand 6 "startScript autoexec.bat do_fir"
 
setButtonEnabled 7 1
setButtonLabel 7 "Chase"
setButtonCommand 7 "startScript autoexec.bat do_chase"
 
// startDriver MCP9808 [ClkPin] [DatPin] [OptionalTargetChannel]
startDriver MCP9808 7 8  1
MCP9808_Adr 0x30
MCP9808_Cycle 1


// use choice to choose effect by index stored in $CH10
alias do_chosen_effect Choice $CH10 cmd_cur cmd_col cmd_rai cmd_pil cmd_fir cmd_chase
// when click on Btn_ScriptOnly on P24 happens, add 1 to $CH10 (wrap to 0-5) and do effect
addEventHandler OnClick 24 backlog addChannel 10 1 0 4 1; do_chosen_effect
 
// stop execution
return
 
 


do_chase:
tuyaMcu_sendState 21 4 2
delay_s 0.1
tuyaMcu_sendState 25 3 020e0d00001403e803e800000000
return
 
 
do_cur:
tuyaMcu_sendState 21 4 2
delay_s 0.1
tuyaMcu_sendState 25 3 000e0d00002e03e802cc00000000
return
 
do_col:
tuyaMcu_sendState 21 4 2
delay_s 0.1
tuyaMcu_sendState 25 3 07464602000003e803e800000000464602007803e803e80000000046460200f003e803e800000000464602003d03e803e80000000046460200ae03e803e800000000464602011303e803e800000000
return
 
do_rai:
tuyaMcu_sendState 21 4 2
delay_s 0.1
tuyaMcu_sendState 25 3 06464601000003e803e800000000464601007803e803e80000000046460100f003e803e800000000
return
 
do_pil:
tuyaMcu_sendState 21 4 2
delay_s 0.1
tuyaMcu_sendState 25 3 010e0d000084000003e800000000
return
 
do_fir:
tuyaMcu_sendState 21 4 2
delay_s 0.1
tuyaMcu_sendState 25 3 05464601000003e803e800000000464601007803e803e80000000046460100f003e803e800000000464601003d03e803e80000000046460100ae03e803e800000000464601011303e803e800000000
return
```


