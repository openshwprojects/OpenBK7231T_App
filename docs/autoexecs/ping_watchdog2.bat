
// Example autoexec.bat usage
// wait for ping watchdog alert when target host does not reply for 100 seconds:

// do anything on startup
startDriver NTP
startDriver SSDP

// setup ping watchdog
PingHost 192.168.0.123
PingInterval 10

// WARNING! Ping Watchdog starts after a delay after a reboot.
// So it will not start counting to 100 immediatelly

// wait for NoPingTime reaching 100 seconds
waitFor NoPingTime 100
echo Sorry, it seems that ping target is not replying for over 100 seconds, will reboot now!
// delay just to get log out
delay_s 1
echo Reboooooting!
// delay just to get log out
delay_s 1
echo Nooow!
reboot