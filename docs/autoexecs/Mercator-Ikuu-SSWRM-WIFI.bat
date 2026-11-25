// Mercator Ikuu SSWRM-WIFI rotary dimmer (AU, no neutral, trailing edge), tested on OpenRTL87X0C 1.18.219
// Tuya WBR2 module (RTL8720CF) plus a separate dimming MCU, talking TuyaMCU
// UART0 (RX PA13, TX PA14) is the RTL87X0C default, so there are no pins to set
// dpIDs: 1 bool on/off, 2 value brightness 10-1000, 3 value minimum brightness (kept by the MCU),
// 6 value countdown in seconds. 3 and 6 are left unlinked here
// comments on their own lines only: text after a command is read as more arguments
startDriver TuyaMCU
tuyaMcu_setBaudRate 9600
// report the network as connected, so the MCU is not sent 0x00 (pairing) until MQTT is up
tuyaMcu_defWiFiState 4
// the MCU won't go below its dpID 3 minimum (30 from the factory), so the bottom of the slider
// does nothing. (dpID3 - 10) * 100 / 99 as the first number puts 1% on that floor: 20 for 30.
// the floor itself changes with tuyaMcu_sendState 3 val 60, and the MCU keeps it across power cycles
tuyaMcu_setDimmerRange 10 1000
// toggle + dimmer is the pair Home Assistant discovery turns into one light
setChannelType 1 toggle
setChannelType 2 dimmer
linkTuyaMCUOutputToChannel 1 bool 1
linkTuyaMCUOutputToChannel 2 val 2
