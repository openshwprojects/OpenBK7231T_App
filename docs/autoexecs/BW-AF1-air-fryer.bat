
startDriver TuyaMCU

// cook on/off 
setChannelType 1 Toggle
setChannelLabel 1 "Cook"
linkTuyaMCUOutputToChannel 111 bool 1 
// power on/off
setChannelLabel 2 "Power"
setChannelType 2 Toggle
linkTuyaMCUOutputToChannel 101 bool 2 

// set temperature
setChannelLabel 3 "New Temperature"
setChannelType 3 TextField
linkTuyaMCUOutputToChannel 103 val 3

// currenttemperature
setChannelLabel 4 "Current Temperature"
setChannelType 4 ReadOnly
linkTuyaMCUOutputToChannel 104 val 4

// set time
setChannelLabel 5 "New Time"
setChannelType 5 TextField
linkTuyaMCUOutputToChannel 105 val 5

// read time
setChannelLabel 6 "Current Time"
setChannelType 6 ReadOnly
linkTuyaMCUOutputToChannel 106 val 6


alias cook185c15min backlog setChannel 2 1; setChannel 3 185; setChannel 5 15; setChannel 1 1
alias cook170c30min backlog setChannel 2 1; setChannel 3 170; setChannel 5 30; setChannel 1 1



startDriver httpButtons
setButtonEnabled 0 1
setButtonLabel 0 "Set 185C 15minutes"
setButtonCommand 0 "cook185c15min "
setButtonColor 0 "orange"


setButtonEnabled 1 1
setButtonLabel 1 "Set 170C 30minutes"
setButtonCommand 1 "cook170c30min "
setButtonColor 1 "orange"


