# Flashing for BK7231 (BK7231T and BK7231N) on Windows - easy method for beginners

Use our new BK7231 GUI Flash tool:
https://github.com/openshwprojects/BK7231GUIFlashTool

# Flashing for BK7231T (alternate method)

## UART (obsolete method; Windows only)

get BKwriter 1.60 exe (extract zip) from [here](https://github.com/openshwprojects/OpenBK7231T/blob/master/bk_writer1.60.zip)
  
Use USB to TTL converter with 3.3V logic levels, like HW 597

connect the PC to TX1 and RX1 on the bk7231 (TX2 and RX2 are optional, only for log)
  
start flash in BKwriter 1.60 (select COM port, etc)
then re-power the device (or reset with CEN by temporary connecting CEN to ground) until the flashing program continues, repeat if required.
  
## UART (obsolete method; multiplatform method, Python required)

clone the repo https://github.com/OpenBekenIOT/hid_download_py
  
Use USB to TTL converter with 3.3V logic levels, like HW 597 

connect the PC to TX1 and RX1 on the bk7231 (TX2 and RX2 are optional, only for log)

start flash using:
`python uartprogram <sdk folder>\apps\<folder>\output\1.0.0\<appname>_UA_<appversion>.bin -d <port> -w`
then re-power the device (or reset with CEN temporary connecting CEN to ground) until the flashing program continues, repeat if required.

e.g.
`python uartprogram C:\DataNoBackup\tuya\tuya-iotos-embeded-sdk-wifi-ble-bk7231t\apps\my_alpha_demo\output\1.0.0\my_alpha_demo_UA_1.0.0.bin -d com4 -w`

## SPI

see https://github.com/OpenBekenIOT/hid_download_py/blob/master/SPIFlash.md

## SPI (new, tested tool for BK7231 SPI mode)
See: https://github.com/openshwprojects/BK7231_SPI_Flasher
