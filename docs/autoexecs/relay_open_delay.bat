
// Following sample shows how to turn on relay without delay, but turn it off with a delay

// Assumptions:
// - channel 1 has a relay (one of pins has role Relay and channel set to 0)
// - channel 2 is purely virtual channel used to introduce the relay

alias enable_without_delay backlog cancelRepeatingEvent 123456; setChannel 1 1
alias disable_with_delay backlog cancelRepeatingEvent 123456; addRepeatingEventID 5 1 123456 setChannel 1 0

setChannelType 2 Toggle
addChangeHandler Channel2 == 1 enable_without_delay
addChangeHandler Channel2 == 0 disable_with_delay