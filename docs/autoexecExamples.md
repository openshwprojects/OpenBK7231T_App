# Example 'autoexec.bat' files


[Configuration for EDM-01AA-EU dimmer with TuyaMCU](https://www.elektroda.com/rtvforum/topic3929151.html)
<br>
```startDriver TuyaMCU
setChannelType 1 toggle
setChannelType 2 dimmer
tuyaMcu_setBaudRate 115200
tuyaMcu_setDimmerRange 1 1000
// linkTuyaMCUOutputToChannel dpId verType tgChannel
linkTuyaMCUOutputToChannel 1 bool 1
linkTuyaMCUOutputToChannel 2 val 2
```


[Configuration for QIACHIP Universal WIFI Ceiling Fan Light Remote Control Kit - BK7231N - CB2S with TuyaMCU](https://www.elektroda.com/rtvforum/topic3895301.html)
<br>
```// start MCU driver
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


[Configuration for controlling LED strip with IR receiver by TV Remote](https://www.elektroda.com/rtvforum/topic3944210.html)
<br>
```// You must set IRRecv role for one of the pins
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
```// channels 1 to 5 are used
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


Simple example showing how to do MQTT publish on button event (double click, etc). It also includes button hold event to adjust dimmer.
<br>
```// A simple script per user request.
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


NTP and 'waitFor' command example. You can use 'waitFor NTPState 1' in autoexec.bat to wait for NTP sync. After that, you can be sure that correct time is set. 'waitFor' will block execution until given event.
<br>
```// do anything on startup
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
```// do anything on startup
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


This script will use NTP and 'addClockEvent' to change light styles in the morning and in the evening. Here you can see how  'waitFor NTPState 1' and 'addClockEvent' and $hour script functions/variables are used.
<br>
```// setup NTP driver
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
```startDriver ntp
ntp_timeZoneOfs 10:00

alias day_lights backlog led_temperature 200; led_dimmer 100; echo lights_day
alias night_lights backlog led_temperature 500; led_dimmer 50; echo lights_night
alias set_now_colour backlog if $hour>=06&&$hour<21 then day_lights; if $hour>=21||$hour<06 then night_lights

addClockEvent 06:01 0xff 1 day_lights 
addClockEvent 20:01 0xff 1 night_lights 

AddEventHandler NTPState 1 set_now_colour


```


Simple UART log redirection to UART1 instead of UART2 on Beken with UART command line support
<br>
```// Here is how you can get log print on UART1, instead of default UART2 on Beken

// Enable "[UART] Enable UART command line"
// this also can be done in flags, enable command line on UART1 at 115200 baud
SetFlag 31 1
// UART1 is RXD1/TXD1 which is used for programming and for TuyaMCU/BL0942,
// but now we will set that UART1 is used for log
logPort 1 

```


Simple single LED blink on device boot
<br>
```// this script just manually turns on and later off LED on device boot
// It assumes that you have a LED on channel 4

// turn on LED on ch 4
setChannel 4 1
// schedule an event, after 2 seconds, turn off LED (this is non-blocking call)
// NOTE: addRepeatingEvent [RepeatTime] [RepeatCount]
addRepeatingEvent 2 1 setChannel 4 0
```


Advanced config for TuyaMCU power meter and electric car charging limit driver
<br>
```// Config for TAC2121C-like TuyaMCU power meter device
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
```// TC74A5 (check address in datasheet! is on SoftSDA and SoftSCL with 10k pull up resistors)
startDriver I2C
setChannelType 6 temperature
// Options are: I2C1 (BK port), I2C2 (BK port), Soft (pins set in configure module)
// 0x4D is device address 
// 6 is target channel
addI2CDevice_TC74 Soft 0x4D 6
```


Deep sleep usage with SHT30 sensor and data reporting via HTTP GET
<br>
```// source: https://www.elektroda.com/rtvforum/viewtopic.php?p=20693161#20693161
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


Manual flash save example for TuyaMCU - using special Channels 200, 201, etc
<br>
```// Read full explanation here: https://www.elektroda.com/rtvforum/topic4003825.html
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


Shift register setup, usage and control with channels
<br>
```// Start shiftRegister driver and map pins, also map channels to outputs
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


Custom countdown/timer system with HTTP GUI for TuyaMCU relay [see tutorial](https://www.elektroda.com/rtvforum/topic4009196.html)
<br>
```// See tutorial: https://www.elektroda.com/rtvforum/topic4009196.html

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


