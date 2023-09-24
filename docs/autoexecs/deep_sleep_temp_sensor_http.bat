// source: https://www.elektroda.com/rtvforum/viewtopic.php?p=20693161#20693161
// NOTE: SHT3X is configured to store temp in channel2 and humidity in channel 3

// start driver
startDriver SHT3X
//hold button for 3 seconds to get into safe mode
addEventHandler OnHold 20 SafeMode 3
// wait for wifi to become WIFI_STA_CONNECTED
waitFor WiFiState 4
Battery_Setup 2500 4200 2.29 2400 4096
battery_measure
publishFloat "voltage" $CH4/1000
//publishFloat "battery" $CH4/25
// extra wait
delay_s 5
// measure
SHT_Measure
// send data
SendGET http://address:port/sensor1.php?temp=$CH2&hum=$CH3&bat=$voltage
// wait for GET to be sent
delay_s 5
// sleep for 120 seconds and then wake up (from blank state)
DeepSleep 120


