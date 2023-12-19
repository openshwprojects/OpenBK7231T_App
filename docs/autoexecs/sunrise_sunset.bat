NOTE: Set Time offset, latitude, longitude accordingly

// autoexec for mini smart switch
PowerSave 1
addEventHandler OnHold 8 SafeMode
startDriver ntp
ntp_timeZoneOfs -8
ntp_setLatlong 44.002130 -123.091473
setPinRole 6 WifiLed_n
setPinRole 8 Btn_Tgl_All
setPinRole 14 TglChanOnTgl
setPinChannel 14 0
setPinRole 15 Rel
setPinChannel 15 0
removeClockEvent 12
removeClockEvent 13
waitFor NTPState 1
addClockEvent sunrise 0x7f 12 POWER OFF
addClockEvent sunset 0x7f 13 POWER ON
