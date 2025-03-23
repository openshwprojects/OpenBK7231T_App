
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