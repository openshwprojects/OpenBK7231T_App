// See tutorial: https://www.elektroda.com/rtvforum/topic4009196.html

// start TuyaMCU driver
startDriver TuyaMCU
// set TuyaMCU baud rate
tuyaMcu_setBaudRate 115200
// set TuyaMCU default wifi state 0x04, which means "paired",
// because some TuyaMCU MCUs will not report all data
// unless they think they are connected to cloud
tuyaMcu_defWiFiState 4
// start NTP driver, so we have time from web
startDriver NTP
// set channel 1 to display toggle
setChannelType 1 Toggle
// Map given dpID to channel 1, dpID type bool
// linkTuyaMCUOutputToChannel [dpId][varType][channelID]
linkTuyaMCUOutputToChannel 1 bool 1
// channel 2 will be temperature
setChannelType 2 temperature
// Map given dpID to channel 2, dpID type value
linkTuyaMCUOutputToChannel 24 val 2

// setup cosmetics for channel 10
setChannelLabel 10  "<b><span style='color:orange'\>Time left</span></b>"
// set channel 10 to display formatted time
setChannelType 10 TimerSeconds

// create command aliases for enable + setup delay (in channel 10) sequences
alias enable_30min backlog POWER ON; setChannel 10 30*60
alias enable_30sec backlog POWER ON; setChannel 10 30

// set channel 11 label, channel 11 will allow to enter hours value of new timespan
setChannelLabel 11  "<b><span style='color:orange'\>Custom time setting [hours]</span></b>"
setChannelType 11 TextField

// start driver handling extra buttons on http page
startDriver httpButtons
// button 0 - set enable to 1
setButtonEnabled 0 1
// set button 0 label
setButtonLabel 0 "Enable for 30 minutes"
// set button 0 command (see alias created above)
setButtonCommand 0 "enable_30min"
// set button 0 color (CSS color code)
setButtonColor 0 "green"

// when anything sets relay to off, also clear timer
addChangeHandler Channel1 == 0 setChannel 10 0
// if user manually sets channel 1 with no timer, then set default time
addChangeHandler Channel1 == 1  if $CH10==0 then "setChannel 10 1234"

// just like for button 0, also setup button 1
setButtonEnabled 1 1
setButtonLabel 1"Enable for 60 minutes"
setButtonCommand 1 "enable_60min"
setButtonColor 1 "green"

// just like for button 0, also setup button 2
startDriver httpButtons
setButtonEnabled 2 1
setButtonLabel 2 "Enable for 30 sec"
setButtonCommand 2 "enable_30sec"
setButtonColor 2 "green"


// every 10 seconds, request update from TuyaMCU
addRepeatingEvent 10 -1 tuyaMcu_sendQueryState

// now this part runs in a loop
again:
// wait one secnd
delay_s 1
// if channel sued for timer has reached 0, time off
if $CH10<=0 then POWER OFF

// subtract one from time, clamp at 0 and 99999
addChannel 10 -1 0 999999

// continue the loop forever
goto again


