
// Water Sensor with deep sleep and buzzer (PWM)
// See related topic: https://www.elektroda.com/rtvforum/viewtopic.php?p=21096228#21096228


Battery_Setup 2000 3000 2 2400 4096
//measure batt every 2s
Battery_cycle 2
//mqtt_broadcastInterval 1
//mqtt_broadcastItemsPerSec 5
addEventHandler OnHold 10 SafeMode 5

setChannelLabel 1 Water

// now wait for MQTT
waitFor MQTTState 1
// extra delay, to be sure
delay_s 1
// publish water state at least once after boot
publish 1 $CH1

// if water detected, keep cycling (drains battery)
// also useful for configuring the device or doing OTA
mainLoop:
delay_s 1
if $CH1!=1 then "goto sirenOn" else "goto sleep"


// turn on the siren and goes into siren loop
sirenOn:
setChannel 6 10

// siren loop repeats while water is detected
sirenLoop:
backlog PWMfrequency 460;delay_ms 50
backlog PWMfrequency 491;delay_ms 50
backlog PWMfrequency 521;delay_ms 50
backlog PWMfrequency 550;delay_ms 50
backlog PWMfrequency 577;delay_ms 50
backlog PWMfrequency 601;delay_ms 50
backlog PWMfrequency 621;delay_ms 50
backlog PWMfrequency 638;delay_ms 50
backlog PWMfrequency 650;delay_ms 50
backlog PWMfrequency 657;delay_ms 50
backlog PWMfrequency 660;delay_ms 50
backlog PWMfrequency 657;delay_ms 50
backlog PWMfrequency 650;delay_ms 50
backlog PWMfrequency 638;delay_ms 50
backlog PWMfrequency 621;delay_ms 50
backlog PWMfrequency 601;delay_ms 50
backlog PWMfrequency 577;delay_ms 50
backlog PWMfrequency 550;delay_ms 50
backlog PWMfrequency 521;delay_ms 50
backlog PWMfrequency 491;delay_ms 50
if $CH1!=1 then "goto sirenLoop" else "goto sirenOff"

// turn off the siren and go into main loop
sirenOff:
setChannel 6 0
goto mainLoop

sleep:
delay_s 5
// All good, sleep for 1 day or next water event
PinDeepSleep 86400