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
