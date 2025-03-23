// This will automatically turn off relay after about 2 seconds
// NOTE: addRepeatingEvent [RepeatTime] [RepeatCount]
addChangeHandler Channel1 != 0 addRepeatingEvent 2 1 setChannel 1 0


// here is alternative sample script, but commented out
// code above will forever toggle relay every 15 seconds
// addRepeatingEvent 15 -1 POWER TOGGLE


