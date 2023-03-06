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