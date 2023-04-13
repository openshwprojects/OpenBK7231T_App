
// this is autoexec.bat, it runs at startup, you should restart after making changes
// target to ping
PingHost 192.168.0.1
// interval in seconds
PingInterval 10
// events to run when no ping happens
addChangeHandler noPingTime > 600 reboot

