
// This script provides extra REST page for hosting in LittleFS
// Please see: https://www.elektroda.com/rtvforum/topic4040354.html
// Script taken from: https://www.elektroda.com/rtvforum/viewtopic.php?p=20990058#20990058
// Download HTML there.

// Clear all previous definitions
clearIO

// clear flags
// flags 0

// set flag 46
SetFlag 46 1

// start TuyaMCU driver
startDriver TuyaMCU
tuyaMcu_setBaudRate 9600

// This one is better than tuyaMcu_defWiFiState 4;  MQTTState 1 = WiFiState 4
// issuing of tuyaMcu_defWiFiState 4 continues the script,
// but doesnt report to MQTT since there is still no connection.
// if you didn't setup MQTT connection then issue tuyaMcu_defWiFiState 4
// and comment waitFor MQTTState 1

waitFor MQTTState 1
// tuyaMcu_defWiFiState 4

// Breaker Id - Dpid 19 "breaker_id" -> channel 0
linkTuyaMCUOutputToChannel 19 str 0
setChannelType 0 ReadOnly
setChannelLabel 0 "Breaker ID"

// Main Relay - Dpid 16 "switch" -> Channel 1
linkTuyaMCUOutputToChannel 16 bool 1
setChannelType 1 toggle
setChannelLabel 1 "Relay"

// Prepayment on-off - Dpid 11 "switch_prepayment" -> channel 2
linkTuyaMCUOutputToChannel 11 bool 2
setChannelType 2 toggle
setChannelLabel 2 "Prepayment"

// Clear Energy Counters - Dpid 12 "clear_energy" -> channel 3
linkTuyaMCUOutputToChannel 12 bool 3
setChannelType 3 toggle
setChannelLabel 3 "Clear Energy Counters"

// Total energy - Dpid 1 "total_forward_energy" -> channel 4
linkTuyaMCUOutputToChannel 1 val 4
setChannelType 4 EnergyTotal_kWh_div100
setChannelLabel 4 "Total Energy"

// Measurements - Dpid 6 "phase_a" - channel RAW_TAC2121C_VCP -> 5,6,7
// TAC2121C VoltageCurrentPower Packet
// This will automatically set voltage, power and current
linkTuyaMCUOutputToChannel 6 RAW_TAC2121C_VCP
setChannelType 5 Voltage_div10
setChannelLabel 5 "Voltage"
setChannelType 6 Power
setChannelLabel 6 "Power"
setChannelType 7 Current_div1000
setChannelLabel 7 "Current"

// Leakage current - Dpid 15 "leakage_current" -> channel 8
linkTuyaMCUOutputToChannel 15 val 8
setChannelType 8 Current_div1000
setChannelLabel 8 "Leakage Current"

// Fault - Dpid 9 "fault" -> channel 9
linkTuyaMCUOutputToChannel 9 raw 9
setChannelType 9 ReadOnly
setChannelLabel 9 "Fault"

// Settings 2 - Dpid 18 "alarm_set_2" -> channel 10
linkTuyaMCUOutputToChannel 18 raw 10
setChannelType 10 ReadOnly
setChannelLabel 10 "UV, OV, Max. Current Protections Settings"

// Prepaid energy - Dpid 13 "balance_energy" -> channel 11
linkTuyaMCUOutputToChannel 13 val 11
setChannelType 11 EnergyTotal_kWh_div100
setChannelLabel 11 "Total Prepaid Energy"

// The above are read only channels. Except Settings 2 (mapped on channel 10), but since we cannot set it due to wrong parse of TuyaMCU driver - for now read only

// Charge Prepayment - Dpid 14 "charge_energy" - channel 12
linkTuyaMCUOutputToChannel 14 val 12
setChannelType 12 TextField
setChannelLabel 12 "Purchased Energy [kWh*100], i.e. 1kWh = 100"

// Settings 1 - Dpid 17 "alarm_set_1" - channel 13
linkTuyaMCUOutputToChannel 17 val 13
setChannelType 13 TextField
setChannelLabel 13 "Leakage Current Protection Settings"

// Protection Reaction Time - Dpid 104 "over_vol_protect_time" (applied for all protections, regardless the name of DpID) -> channel 14
linkTuyaMCUOutputToChannel 104 val 14
setChannelType 14 TextField
setChannelLabel 14 "Protection Reaction Time [s]"

// Protection Recovery Time - Dpid 105 "over_vol_recovery_time" (applied for all protections, regardless the name of DpID) -> channel 15
linkTuyaMCUOutputToChannel 105 val 15
setChannelType 15 TextField
setChannelLabel 15 "Protection Recovery Time [s]"

// NOTE: addRepeatingEvent [RepeatTime] [RepeatCount]
// code below will forever Send query state command every 5 seconds
// addRepeatingEvent 5 -1 tuyaMcu_sendQueryState 

// AngelOfDeath - We don't need it forever, since TuyaMCU sends everything when necessary
// we need it just first time to obtain initial status. Some dpIDs not reported without asking
tuyaMcu_sendQueryState
