# Building

Get the SDK repo:
https://github.com/openshwprojects/OpenBK7231T
Clone it to a folder, e.g. bk7231sdk/

Clone the app repo into bk7231sdk/apps/<appname> - e.g. bk7231sdk\apps\openbk7231app

On Windows, start a cygwin prompt.

go to the SDK folder.

build using:
`./build_app.sh apps/<folder> <appname> <appversion>`

e.g. `./build_app.sh apps/openbk7231app openbk7231app 1.0.0`

# flashing

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


