// do anything on startup
startDriver NTP
startDriver SSDP

// now wait for MQTT
waitFor MQTTState 1
// extra delay, to be sure
delay_s 1
publish myVariable 2022
// you can publish again just to be sure
// delay_s 1
// publish myVariable 2022

// if you have a battery powered device, you can now uncomment this line:
// Deep sleep (shut down everything) and reboot automatically after 600 seconds
//DeepSleep 600
