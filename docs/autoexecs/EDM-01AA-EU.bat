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