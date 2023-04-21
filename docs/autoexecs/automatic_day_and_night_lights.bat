// setup NTP driver
startDriver ntp
// set your time zone
ntp_timeZoneOfs 10:00

// create command aliases for later usage
alias day_lights backlog led_temperature 200; led_dimmer 100; echo lights_day
alias night_lights backlog led_temperature 500; led_dimmer 50; echo lights_night

// at given hour, change lights state
addClockEvent 06:01 0xff 1 day_lights 
addClockEvent 20:01 0xff 1 night_lights 

// wait for NTP sync
waitFor NTPState 1
// after NTP is synced, just after reboot, adjust light states correctly
if $hour>=06&&$hour<21 then day_lights
if $hour>=21||$hour<06  then night_lights

