# Introduction

This repository is named "OpenBK7231T_App", but now it's a multiplatform app, supporting build for 3 separate chips:
- BK7231T (WB3S, WB2S, etc)
- BK7231N (CB2S, etc)
- XR809 (XR3, etc)

To build for a given platform, just checkout first our version of SDK and then checkout this app repository into it, details later.

# Building for BK7231T

Get the SDK repo:
https://github.com/openshwprojects/OpenBK7231T
Clone it to a folder, e.g. bk7231sdk/

Clone the app repo into bk7231sdk/apps/<appname> - e.g. bk7231sdk\apps\openbk7231app

On Windows, start a cygwin prompt.

go to the SDK folder.

build using:
`./b.sh`
  
you can also do advanced build by build_app.sh:
`./build_app.sh apps/<folder> <appname> <appversion>`

e.g. `./build_app.sh apps/openbk7231app openbk7231app 1.0.0`

# flashing for BK7231T

## UART

clone the repo https://github.com/OpenBekenIOT/hid_download_py

connect the PC to serial 2 on the bk7231

flash using:
`python uartprogram <sdk folder>\apps\<folder>\output\1.0.0\<appname>_UA_<appversion>.bin -d <port> -w`
re-power the device until the flashing program works, repeat if required.

e.g.
`python uartprogram C:\DataNoBackup\tuya\tuya-iotos-embeded-sdk-wifi-ble-bk7231t\apps\my_alpha_demo\output\1.0.0\my_alpha_demo_UA_1.0.0.bin -d com4 -w`


## SPI

see https://github.com/OpenBekenIOT/hid_download_py/blob/master/SPIFlash.md

## OTA

Once the firmware has been flashed for the first time, it can be flashed over wifi (note: change hardcoded firmware URL in new_http.c)

Setup a simple webserver to serve `<sdk folder>\apps\<folder>\output\1.0.0\<appname>_<appversion>.rbl`

Visit <ip>/ota - this will start the flashing process.

## First run

At first boot, if the new formware does not find your wifi SSID and password in the Tuya flash, it will start as an access point.

The access point will come up on 192.168.4.1, however some machines may not get an ip from it - you may need to configure your connecting for a staitc IP on that network, e.g. 192.168.4.10

Once you are connected and have an IP, go to http://192.168.4.1/index , select config then wifi, and setup your wifi.

After a reboot, the device should connect to your lan.

  
 
# Building for BK7231N

Same as for BK7231T, but use BK7231N SDK:
https://github.com/openshwprojects/OpenBK7231N


# Flashing for BK7231N

BKwriter 1.60 doesn't work for BK7231N for me, in BK7231 mode it errors with "invalid CRC" and in BK7231N mode it fails to unprotect the device.
For BK7231N, one should use:
  
https://github.com/OpenBekenIOT/hid_download_py
  
Flash BK7231N QIO binary, like that:
  
`python uartprogram W:\GIT\OpenBK7231N\apps\OpenBK7231N_App\output\1.0.0\OpenBK7231N_app_QIO_1.0.0.bin --unprotect -d com10 -w --startaddr 0x0`
  
 
# Building for XR809

Get XR809 SDK:
https://github.com/openshwprojects/OpenXR809

(to be continued)
  
  
# Testing HTTP server on Windows
  
It is also possible to build a part of our App for Windows platform. It basically creates a Windows .exe for our HTTP server, so developers can create our configurator, etc, pages faster, without having any Tuya modules at hand. For building on Windows, use MSVC projects in the app directory.
  
# Futher reading
  
For technical insights and generic SDK information related to Bekken and XRadio modules, please refer:
  
https://www.elektroda.com/rtvforum/topic3850712.html
  
https://www.elektroda.com/rtvforum/topic3866123.html
  
https://www.elektroda.com/rtvforum/topic3806769.html
  
# Support project
  
If you want to support project, please donate at: https://www.paypal.com/paypalme/openshwprojects
  
