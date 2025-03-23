startDriver ntp
ntp_timeZoneOfs 10:00

alias day_lights backlog led_temperature 200; led_dimmer 100; echo lights_day
alias night_lights backlog led_temperature 500; led_dimmer 50; echo lights_night
alias set_now_colour backlog if $hour>=06&&$hour<21 then day_lights; if $hour>=21||$hour<06 then night_lights

addClockEvent 06:01 0xff 1 day_lights 
addClockEvent 20:01 0xff 1 night_lights 

AddEventHandler NTPState 1 set_now_colour

