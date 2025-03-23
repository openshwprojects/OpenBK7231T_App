# Example script files


<b>Repeating event demos</b>
<br>Repeating events can be used to create a simple timer.
<br>Requirements:
- channel 1 - output relay<br>

```// This will automatically turn off relay after about 2 seconds
// NOTE: addRepeatingEvent [RepeatTime] [RepeatCount]
addChangeHandler Channel1 != 0 addRepeatingEvent 2 1 setChannel 1 0


// here is alternative sample script, but commented out
// code above will forever toggle relay every 15 seconds
// addRepeatingEvent 15 -1 POWER TOGGLE



```


<b>Loop demo</b>
<br>Features a 'goto' script command (for use within script) and, obviously, a label.
<br>Requirements:
- channel 1 - output relay<br>

```again:
	
	again:
	echo "Step 1"
	setChannel 1 0
	echo "Step 2"
	delay_s 2
	echo "Step 3"
	setChannel 1 1
	echo "Step 4"
	delay_s 2
	goto again
```


<b>Loop & if demo</b>
<br>This example shows how you can use a dummy channel as a variable to create a loop
<br>Requirements:
- channel 1 - output relay<br>
- channel 11 - loop variable counter<br>

```restart:
	// Channel 11 is a counter variable and starts at 0
	setChannel 11 0
again:

	// If channel 11 value reached 10, go to done
	if $CH11>=10 then goto done
	// otherwise toggle channel 1, wait and loop
	toggleChannel 1
	addChannel 11 1
	delay_ms 250
	goto again
done:
	toggleChannel 1
	delay_s 1
	toggleChannel 1
	delay_s 1
	toggleChannel 1
	delay_s 1
	toggleChannel 1
	delay_s 1
	goto restart
```


<b>Thread cancelation demo and exclude self demo</b>
<br>This example shows how you can create a script thread with an unique ID and use this ID to cancel the thread later
<br>Requirements:
- channel 1 - output relay<br>
- pin 8 - button<br>
- pin 9 - button<br>

```// 'this' is a special keyword - it mean search for script/label in this file
// 123 and 456 are unique script thread names
addEventHandler OnClick 8 startScript this label1 123
addEventHandler OnClick 9 startScript this label2 456

label1:
	// stopScript ID bExcludeSelf
	// this will stop all other instances
	stopScript 456 1
	stopScript 123 1
	setChannel 1 0
	delay_s 1
	setChannel 1 1
	delay_s 1
	setChannel 1 0
	delay_s 1
	setChannel 1 1
	delay_s 1
	setChannel 1 0
	delay_s 1
	setChannel 1 1
	delay_s 1
	exit;
label2:
	// stopScript ID bExcludeSelf
	// this will stop all other instances
	stopScript 456 1
	stopScript 123 1
	setChannel 1 0
	delay_s 0.25
	setChannel 1 1
	delay_s 0.25
	setChannel 1 0
	delay_s 0.25
	setChannel 1 1
	delay_s 0.25
	setChannel 1 0
	delay_s 0.25
	setChannel 1 1
	delay_s 0.25
	setChannel 1 0
	delay_s 0.25
	exit;
```


<b>Using channel value as a variable demo</b>
<br>
<br>Requirements:
- channel 1 - output relay<br>
- channel 11 - you may use it as ADC, or just use setChannel 11 100 or setChannel 11 500 in console to change delay<br>

```
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
```


<b>Sending UART from script - WXDM2 dimmer support</b>
<br>
<br>Requirements:
- a WXDM2 dimmer with custom UART protocol<br>

```
// WXDM2 dimmer demo
// WXDM2 is using custom UART protocol with no checksum


again:
addChannel 10 1 0 255 1
delay_ms 100
uartSendHex 0A FF 55 02 00 $CH10$ 00 00 0A
goto again



```


