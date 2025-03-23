
// This aliased command will turn off relay on CH1 after 10 seconds
// addRepeatingEvent	[IntervalSeconds][RepeatsOr-1][CommandToRun]
alias turn_off_after_time addRepeatingEvent 10 1 setChannel 1 0
// this will run the turn off command every time that CH1 becomes 1
addChangeHandler Channel1 == 1 turn_off_after_time 
