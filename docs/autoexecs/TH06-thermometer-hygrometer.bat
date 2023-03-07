
startDriver TuyaMCU
startDriver NTP
// dpID 1 is tempererature div 10
setChannelType 1 temperature_div10
linkTuyaMCUOutputToChannel 1 val 1
// dpID 2 is % humidity
setChannelType 2 Humidity
linkTuyaMCUOutputToChannel 2 val 2