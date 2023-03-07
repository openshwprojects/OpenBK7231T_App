// You must set IRRecv role for one of the pins
// IR driver will start itself automatically after reboot

// P21 role is Btn, a power button that works without scripting
// set button hold/repeat/etc times
SetButtonTimes 10 3 3
// alias to turn off LED after 4 secs (repeating event with 1 repeat)
alias add_turnoff_event addRepeatingEvent 4 1 led_enableAll 0
// button events - 23, 22, etc are pin numbers
addEventHandler OnHold 23 add_dimmer 10
addEventHandler OnHold 22 add_dimmer -10
addEventHandler OnDblClick 22 led_dimmer 5
addEventHandler OnDblClick 23 led_dimmer 100
addEventHandler OnClick 20 add_turnoff_event
// IR events
addEventHandler2 IR_Samsung 0x707 0x62 add_dimmer 10
addEventHandler2 IR_Samsung 0x707 0x65 add_dimmer -10
addEventHandler2 IR_Samsung 0x707 0x61 led_enableAll 0
addEventHandler2 IR_Samsung 0x707 0x60 led_enableAll 1