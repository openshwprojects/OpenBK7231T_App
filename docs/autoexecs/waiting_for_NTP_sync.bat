// do anything on startup
startDriver NTP
startDriver SSDP


// now you can wait for NTP to sync
// This will block execution until NTP get correct time
// NOTE: you can also do 'waitFor MQTTState 1' as well, if you want MQTT state 1
waitFor NTPState 1

// default setting
Dimmer 5
Power ON

// now you can do something basing on time
// idk, it's just a simple example, note that 
// next Dimmer setting will overwite previos setting
if $hour<8 then Dimmer 50
if $hour>=20 then Dimmer 50
if $hour>=23 then Dimmer 90
