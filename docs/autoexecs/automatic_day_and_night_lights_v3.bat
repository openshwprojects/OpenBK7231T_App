PowerSave 1
startDriver ntp
ntp_setServer 192.168.1.12
ntp_timeZoneOfs 11
ntp_setLatlong -33.868820 151.209290

// Use channel 10 to maintain sunset time (The time when the light should turn on)
setChannelLabel 10  "<b><span style='color:orange'\>Light ON (Civil Sunset)</span></b>"
setChannelType 10 TimerSeconds

// Use channel 11 as a hidden register to store current time as TimerSeconds
setChannelType 11 TimerSeconds
setChannelPrivate 11 1
setChannelVisible 11 0

alias high_lights backlog led_temperature 300; led_dimmer 30; led_enableAll 1; echo lights_set_high
alias low_lights backlog led_temperature 500; led_dimmer 5; led_enableAll 1; echo lights_set_low
alias off_lights backlog led_enableAll 0; echo lights_set_off

alias calc_sunset backlog setChannel 10 $sunset+1800; removeClockEvent 4; addClockEvent $CH10 0xff 4 high_lights; echo Lights will turn on at $CH10

addClockEvent 3:30 0xff 1 calc_sunset
addClockEvent 21:00 0xff 2 low_lights
addClockEvent 23:00 0xff 3 off_lights

waitFor NTPState 1

calc_sunset

// set the current time as TimerSeconds in register for checks below
setChannel 11 $hour*3600+$minute*60

// Set initial light state to match the above clock events
// sunset - 21:00   lights set to high (evening activities)
// 21:00 - 23:00    lights set to low (sleepy mode: warmest temperature and very dim)
// 23:00 - sunset   lights turned off (bedtime and day time the lights should be off)
if $CH11>=$CH10&&$hour<21 then high_lights
if $hour>=21&&$hour<23 then low_lights
if $hour>=23||$CH11<$CH10 then off_lights