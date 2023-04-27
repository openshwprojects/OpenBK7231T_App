// A simple script per user request.
// Device has single button on P26
// Device also has a relay or a light

// Btn_ScriptOnly is set on P26
// click toggles power
addEventHandler OnClick 26 POWER TOGGLE
// remaining events do MQTT publishes
// NOTE: publish [topicName] [payload]. Final topic will be like obk0696FB33/[Topic]/get
addEventHandler OnDblClick 26 publish myClickIs doubleClick
addEventHandler On3Click 26 publish myClickIs tripleClick
addEventHandler On4Click 26 publish myClickIs quadraClick
addEventHandler On5Click 26 publish myClickIs pentaClick

// NOTE: if you want also to have button hold, and this device
// is a LED device, you can use:
addEventHandler OnHold 26 led_addDimmer 10 1
// note: led_addDimmer [Delta] [Mode], mode 2 means ping pong

