// Battery powered SHT30 sensor in deep sleep, waking when a reading passes its +- limit, plus a 4 hour periodic wake timer
// Tested on a BK7231N CBU temperature/humidity sensor with an SHT30 on software I2C (RH-MAGv3B board)
// Reference template: https://openbekeniot.github.io/webapp/devices/Tuya_Generic_Temperature_and_Humidity_Sensor_Battery_Powered_SHT30.html
// Set P16 to the DoorSnsrWSleep role so the SHT30 can wake the chip

startDriver SHT3X
setChannelType 2 temperature_div10
setChannelType 3 Humidity
// hold the button on P20 for 2 seconds to reboot into safe mode (its own AP at 192.168.4.1)
// the button does not wake the chip, remove/add batteries to boot fresh
addEventHandler OnHold 20 SafeMode
// read once, so the limits below have a value to center on
SHT_Measure
// center the alert limits on that reading: +-1.11 C (2 F) and +-3 %RH
// channel 2 holds temperature x10
SHT_SetAlert $CH2/10+1.11 $CH2/10-1.11 $CH3+3 $CH3-3
// the SHT30 only compares readings against its limits while measuring periodically
// 0x21 0x2D is 1 measurement per second, low repeatability
SHT_LaunchPer 0x21 0x2D
// clear the alert from the sensor to prevent an endless wake loop
SHT_ClearStatus
// sleep 3 seconds after connecting, and wake every 4 hours even if nothing changed
DSTime 3 14400
