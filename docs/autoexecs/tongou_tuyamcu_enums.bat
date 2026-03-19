// tuyaMCU store RAW data in /cm?cmnd=Dp must be turned off on this device..
setflag 46 0

ntp_setServer 132.163.97.4
ntp_timeZoneOfs 12:00

startDriver TuyaMCU

tuyaMcu_setBaudRate 115200

// always report paired
tuyaMcu_defWiFiState 4

// update states any time the temperature changes
addEventHandler OnChannelChange 27 tuyaMcu_sendQueryState


// 2 switch 1 relay bool - 121 device control must be 2 or 3 (remote mode)
setChannelType 21 Toggle
setChannelLabel 21 "Switch 1"
linkTuyaMCUOutputToChannel 2 bool 21

// 101 switch 2 relay bool - 121 device control must be 2 or 3 (remote mode)
setChannelType 22 Toggle
setChannelLabel 22 "Switch 2"
linkTuyaMCUOutputToChannel 101 bool 22

// 104 Switch 1 Automatic Mode bool
setChannelType 24 Toggle
setChannelLabel 24 "Switch 1 Auto Mode"
linkTuyaMCUOutputToChannel 104 bool 24

// 105 Switch 2 Automatic Mode bool
setChannelType 25 Toggle
setChannelLabel 25 "Switch 2 Auto Mode"
linkTuyaMCUOutputToChannel 105 bool 25

// 27 current temperature /10 - dpId 20 changes C/F
setChannelType 1 Temperature_div10
linkTuyaMCUOutputToChannel 27 val 1

// 46 current humidity
setChannelType 2 Humidity
linkTuyaMCUOutputToChannel 46 val 2

//102 online state enum; 0 online, 1 offline
setChannelType 3 ReadOnlyEnum
setChannelLabel 3 "Online State"
setChannelEnum 3 0:Online 1:Offline
linkTuyaMCUOutputToChannel 102 enum 3

// 118 event RO
setChannelType 4 ReadOnlyEnum
setChannelLabel 4 "Event Status"
SetChannelEnum 4 0:Normal "9:Buttons Locked" "10:Local Mode" "11:Remote Control" "12:Any Control"
linkTuyaMCUOutputToChannel 118 enum 4

// 121 device control mode enum; 0 local_lock, 1 MCU control, 2 OBK control, 3 MCU and Tuya control
setChannelType 5 Enum
setChannelLabel 5 "Device Control"
SetChannelEnum 5 "0:Buttons Locked" "1:Device Control" "2:Remote Control" "3:Any Control"
linkTuyaMCUOutputToChannel 121 enum 5

// 106 device Power-On Relay behaviour
setChannelType 6 Enum
setChannelLabel 6 "Power-on Behaviour"
SetChannelEnum 6 0:off 1:on 2:memory
linkTuyaMCUOutputToChannel 106 enum 6

// 107 Switch 1 Automatic Control Mode
setChannelType 7 Enum
setChannelLabel 7 "Switch 1 Control Mode"
setChannelEnum 7 0:Temp 1:Humidity
linkTuyaMCUOutputToChannel 107 enum 7

// 108 Switch 2 Automatic Control Mode
setChannelType 8 Enum
setChannelLabel 8 "Switch 2 Control Mode"
setChannelEnum 8 0:Temp 1:Humidity
linkTuyaMCUOutputToChannel 108 enum 8

// 22 Switch 1 Min Temp Set C -10-99
setChannelType 10 TextField
setChannelLabel 10 "Switch 1 Min C"
linkTuyaMCUOutputToChannel 22 val 10

// 110 Switch 1 Max Temp Set C -10-99
setChannelType 11 TextField
setChannelLabel 11 "Switch 1 Max C"
linkTuyaMCUOutputToChannel 110 val 11

// 113 Switch 2 Min Temp Set C -10-99
setChannelType 13 TextField
setChannelLabel 13 "Switch 2 Min C"
linkTuyaMCUOutputToChannel 113 val 13

// 115 Switch 2 Max Temp Set C -10-99
setChannelType 15 TextField
setChannelLabel 15 "Switch 2 Max C"
linkTuyaMCUOutputToChannel 115 val 15

// refresh tuyaMCU after definitions
tuyaMcu_sendQueryState


