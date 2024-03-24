// HTTP calls example
// This example shows how can one OBK device control another OBK or Tasmota device via HTTP
// No MQTT is needed
// Make sure to add a button with channel 1
// Also make sure that target device IP is correct

// when channel 1 becomes 0, send OFF
addChangeHandler Channel1 == 0 SendGet http://192.168.0.112/cm?cmnd=Power0%20OFF
// when channel 1 becomes 1, send ON
addChangeHandler Channel1 == 1 SendGet http://192.168.0.112/cm?cmnd=Power0%20ON