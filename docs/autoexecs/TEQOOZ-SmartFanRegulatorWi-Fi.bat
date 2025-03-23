
// start MCU driver
startDriver TuyaMCU

// fan on/off channel
setChannelType 1 toggle

//fan speed channel
setChannelType 3 OffLowestLowMidHighHighest

// child Lock channel
setChannelType 14 toggle

// countdown channel
setChannelType 22 TextField

// remaining timer channel
setChannelType 23 TextField

// link output 1 to channel 1
// linkTuyaMCUOutputToChannel dpId varType tgChannel
linkTuyaMCUOutputToChannel 1 1 1

// link output 3 to channel 3
linkTuyaMCUOutputToChannel 3 2 3

// link output 14 to channel 14
linkTuyaMCUOutputToChannel 14 1 14

// link output 22 to channel 22
linkTuyaMCUOutputToChannel 22 2 22

// link output 23 to channel 23
linkTuyaMCUOutputToChannel 23 2 23