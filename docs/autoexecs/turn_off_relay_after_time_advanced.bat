
// display seconds timer
setChannelType 2 TimerSeconds
// set start values as "remember in flash"
SetStartValue 1 -1
SetStartValue 2 -1

// channel 2 is timer
// channel 1 is relay

// here is countdown value in seconds - 30
alias turn_off_after_time backlog setChannel 2 30
alias on_turned_off setChannel 2 0
alias turn_off_relay setChannel 1 0
alias do_check if $CH2==0 then turn_off_relay 
alias do_tick backlog addChannel 2 -1; do_check

// this will run the turn off command every time that CH1 becomes 1
addChangeHandler Channel1 == 1 turn_off_after_time 
// this will run the turn off command every time that CH1 becomes 0
addChangeHandler Channel1 == 0 on_turned_off


again:

if $CH2!=0 then do_tick
delay_s 1
goto again