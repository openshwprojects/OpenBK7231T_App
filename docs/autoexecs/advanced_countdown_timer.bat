
// advanced delay script
// See: https://www.elektroda.com/rtvforum/topic4032982.html

// When user toggles relay on, relay will turn off after delay
// The current delay value is displayed on www gui
// The constant delay value is displayed on www gui and fully adjustable and remembered between reboots 

// Used channels:
// Channel 1 - relay
// Channel 2 - current countdown value
// Channel 3 - constant time for countdown

// this will make channel 3 save in memory
setStartValue 3 -1
// if channel 3 is not set (default first case), set it to 15 seconds11
if $CH3==0 then setChannel 3 15
// display text field for channel 3 on www page
setChannelType 3 TextField
// set label
setChannelLabel 3 TurnOffTime
// display timer seconds (read only) for channel 2 on www page
setChannelType 2 TimerSeconds
// shorthand for setting channel 2 to value of channel 3
alias turn_off_after_time backlog setChannel 2 $CH3
// shorthand for clearing current timer value
alias on_turned_off setChannel 2 0
// shorthand to set relay off
alias turn_off_relay setChannel 1 0
// check used to turn off relay when countdown value reaches 0
alias do_check if $CH2==0 then turn_off_relay 
// an update command; it decreases current countdown by 1 and checks for 0
alias do_tick backlog addChannel 2 -1; do_check
// event triggers for channel 1 changing to 0 and 1
addChangeHandler Channel1 == 1 turn_off_after_time 
addChangeHandler Channel1 == 0 on_turned_off

// main loop - infinite
again:
if $CH2!=0 then do_tick
delay_s 1
goto again