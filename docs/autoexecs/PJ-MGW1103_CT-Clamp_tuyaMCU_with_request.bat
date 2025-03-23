
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