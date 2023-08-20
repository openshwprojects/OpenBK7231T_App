// Config for TAC2121C-like TuyaMCU power meter device
// Also a charging limit driver is setup for charging electric cars
// See: https://www.elektroda.com/rtvforum/topic3996220.html
// See: https://obrazki.elektroda.pl/6653013600_1692204415.png
startDriver httpButtons
startDriver TuyaMCU
startDriver NTP

linkTuyaMCUOutputToChannel 16 1 1
setChannelType 1 toggle

setChannelType 2 Voltage_div10

setChannelType 3 Power

setChannelType 4 Current_div1000

linkTuyaMCUOutputToChannel 1 0 5
setChannelType 5 EnergyTotal_kWh_div100

linkTuyaMCUOutputToChannel 6 RAW_TAC2121C_VCP

linkTuyaMCUOutputToChannel 101 0 6
setChannelType 6 Temperature_div10

setChannelType 7 Error


// *******************************
// Charger setup 

// We use toggle 29 to enable/disable the charger mode
// while enabling, we start a new cycle immediately

setChannelType 29 Toggle
SetChannelLabel 29 "Charger mode"
addChangeHandler Channel29 == 1 startchargermode
addChangeHandler Channel29 == 0 stopchargermode

// disable driver by default after restart 
SetChannel 29 0

// (re)start session when toggled
addChangeHandler Channel1 == 1 startsession  
addChangeHandler Channel1 == 0 stopsession

// disable output by default after restart 
setChannel 1 0

// we keep the maximum time in channel 30 ( $CH30)
setChannelType 30 TextField
SetChannelLabel 30 "30 : Max Power (Wh)"
setChannel 30 2

// we keep the maximum time in channel 31 ( $CH31)
setChannelType 31 TextField
SetChannelLabel 31 "31 : Max Time (Seconds)"
setChannel 31 60


// chargermode

alias startchargermode backlog startDriver ChargingLimit; publish charging/driver "started";  startsession; 
alias stopchargermode backlog stopDriver ChargingLimit; publish charging/driver "stopped"; stopsession;

// charging sessions
alias startsession backlog setChannel 1 1; chSetupLimit 3 $CH30 $CH31 "POWER OFF"; publish charging/status "started" ; publish charging/status/starthour $hour ; publish charging/status/startminute $minute ; publish charging/status/stophour 0; publish charging/status/stopminute 0;

alias stopsession backlog setChannel 1 0; publish charging/status "stopped"; publish charging/status/stophour $hour; publish charging/status/stopminute $minute;


// some presets 

alias presetguests backlog setChannel 30 5000; setChannel 31 3600;
alias presetfriends backlog setChannel 30 10000; setChannel 31 7200;
alias presettest backlog setChannel 30 2; setChannel 31 10;

setButtonEnabled 0 1
setButtonLabel 0 "Preset guests 5 KWh / 1 Hour" 
setButtonCommand 0 presetguests
setButtonColor 0 "#56b08f"

setButtonEnabled 1 1
setButtonLabel 1 "Preset Friends 10 KWh / 2 Hours" 
setButtonCommand 1 presetfriends
setButtonColor 1 "#56b08f"

setButtonEnabled 2 1
setButtonLabel 2 "Preset Test 2 Wh / 10 sec" 
SetButtonCommand 2 presettest
setButtonColor 2 "#56b08f"


