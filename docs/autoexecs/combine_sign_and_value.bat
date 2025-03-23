// For TuyaMCU
// One dpID is sign, second is value
// Taken from: https://www.elektroda.com/rtvforum/viewtopic.php?p=21125206#21125206

//power A
setChannelType 3 Power_div10
SetChannelLabel 3 "Power(A)"
linkTuyaMCUOutputToChannel  101 1 3

//Direction A
setChannelType 4 ReadOnly
SetChannelLabel 4 "Direction(A)"
linkTuyaMCUOutputToChannel  102 1 4

//Corrected Power
setChannelType 19 ReadOnly
SetChannelLabel 19 "Corrected Power(A)"

// channel 4 is sign
// channel 3 is unsigned val
alias positive setChannel 19 $CH3
alias negative setChannel 19 $CH3*-1
alias myset if $CH4==0 then positive else negative
// now trigger it on every change
addEventHandler OnChannelChange 3 myset 
addEventHandler OnChannelChange 4 myset 
