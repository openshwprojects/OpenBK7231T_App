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


