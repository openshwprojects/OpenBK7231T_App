# Flashing for BK7231 (BK7231T and BK7231N) on Windows - easy method for beginners

Use our new BK7231 GUI Flash tool:
https://github.com/openshwprojects/BK7231GUIFlashTool
For step by step process, you can see those guides:
- https://www.elektroda.com/rtvforum/topic3875654.html
- https://www.elektroda.com/rtvforum/topic3874289.html

You can check our Youtube channel for many per-device flashing guides:
https://www.youtube.com/@elektrodacom

# Flashing for BK7231T (alternate method)

## UART (obsolete method; Windows only)

get BKwriter 1.60 exe (extract zip) from [here](https://github.com/openshwprojects/OpenBK7231T/blob/master/bk_writer1.60.zip)
  
Use USB to TTL converter with 3.3V logic levels, like HW 597

connect the PC to TX1 and RX1 on the bk7231 (TX2 and RX2 are optional, only for log)
  
start flash in BKwriter 1.60 (select COM port, etc)
then re-power the device (or reset with CEN by temporarily connecting CEN to ground) until the flashing program continues, repeat if required.
  
## UART (obsolete method; multiplatform method, Python required)

clone the repo https://github.com/OpenBekenIOT/hid_download_py
  
Use USB to TTL converter with 3.3V logic levels, like HW 597 

connect the PC to TX1 and RX1 on the bk7231 (TX2 and RX2 are optional, only for log)

start flash using:
`python uartprogram <sdk folder>\apps\<folder>\output\1.0.0\<appname>_UA_<appversion>.bin -d <port> -w`
then re-power the device (or reset with CEN temporarily connecting CEN to ground) until the flashing program continues, repeat if required.

e.g.
`python uartprogram C:\DataNoBackup\tuya\tuya-iotos-embeded-sdk-wifi-ble-bk7231t\apps\my_alpha_demo\output\1.0.0\my_alpha_demo_UA_1.0.0.bin -d com4 -w`

## SPI

see https://github.com/OpenBekenIOT/hid_download_py/blob/master/SPIFlash.md

## SPI (new, tested tool for BK7231 SPI mode)
See: https://github.com/openshwprojects/BK7231_SPI_Flasher


## OTA

Once the firmware has been flashed for the first time, it can be flashed over wifi.

Go to "Open Web Application", OTA tab, drag and drop proper RBL file on the field, press a button to start OTA process

## First run

At first boot, if the new firmware does not find your wifi SSID and password in the Tuya flash, it will start as an access point.

The access point will come up on 192.168.4.1, however some machines may not get an ip from it - you may need to configure your connecting for a static IP on that network, e.g. 192.168.4.10

Once you are connected and have an IP, go to http://192.168.4.1/index , select config then wifi, and setup your wifi.

After a reboot, the device should connect to your lan.

# Flashing for BK7231N (obsolete method)

BKwriter 1.60 doesn't work for BK7231N for me, in BK7231 mode it errors with "invalid CRC" and in BK7231N mode it fails to unprotect the device.
For BK7231N, one should use:
  
https://github.com/OpenBekenIOT/hid_download_py
  
Flash BK7231N QIO binary, like that:
  
`python uartprogram W:\GIT\OpenBK7231N\apps\OpenBK7231N_App\output\1.0.0\OpenBK7231N_app_QIO_1.0.0.bin --unprotect -d com10 -w --startaddr 0x0`
  
  Remember - QIO binary with --unprotect and --startaddr 0x0, this is for N chip, not for T.
 
You can see an example of detailed teardown and BK7231N flashing here: https://www.elektroda.com/rtvforum/topic3874289.html
  
# Flashing for XR809
  
Get USB to UART converter, start phoenixMC.exe from OpenXR809 repository and follow those guides:
- https://www.elektroda.com/rtvforum/topic3806769.html
- https://www.elektroda.com/rtvforum/topic3890640.html
  
# Building and flashing for BL602

Follow the BL602 guide:
https://www.elektroda.com/rtvforum/topic3889041.html

# Flashing for W800/W801

Use wm_tool.exe, command line utility from this SDK https://github.com/openshwprojects/OpenW800

wm_tool.exe -c COM9 -dl W:\GIT\wm_sdk_w800\bin\w800\w800.fls

wm_tool.exe will then wait for device reset. Repower it or connect RESET to ground, then it will start the flashing

# OTA for W800/W801
  
  Create a HTTP server (maybe with Node-Red), then use the update mechanism by HTTP link. Give link to w800_ota.img file from the build. The second OTA mechanism (on javascript panel, by drag and drop) is not ready yet for W800/W801. Wait for device to restart, do not repower it manually.
  
 
# Troubles programming by UART?
  
I had some reports from users saying that they are unable to flash BK7231T with CP2102 but CH340 works. So, maybe try different USB to UART dongle. Some of them might not be reliable enough for flashing.
  Also make sure to use a reliable 3.3V power supply. 
