
// set default value
setChannel 11 500
// if you don't have ADC, use this to force-display 11 as a slider on GUI
setChannelType 11 dimmer1000

looper:
	setChannel 1 0
	delay_ms $CH11
	setChannel 1 1
	delay_ms $CH11
	goto looper