// this script just manually turns on and later off LED on device boot
// It assumes that you have a LED on channel 4

// turn on LED on ch 4
setChannel 4 1
// schedule an event, after 2 seconds, turn off LED (this is non-blocking call)
// NOTE: addRepeatingEvent [RepeatTime] [RepeatCount]
addRepeatingEvent 2 1 setChannel 4 0