startDriver TuyaMCU
setChannelType 1 toggle
setChannelType 2 dimmer
tuyaMcu_setBaudRate 115200
tuyaMcu_setDimmerRange 1 1000
// linkTuyaMCUOutputToChannel dpId verType tgChannel
linkTuyaMCUOutputToChannel 1 bool 1
linkTuyaMCUOutputToChannel 2 val 2