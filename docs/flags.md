# Flags
Here is the latest, up to date Flags list.
This file was autogenerated by running 'node scripts/getcommands.js' in the repository.
All descriptions were taken from code.
Do not add anything here, as it will overwritten with next rebuild.
| ID |   Description  |
|:--| -------:|
| 0 | [MQTT] Broadcast led params together (send dimmer and color when dimmer or color changes, topic name: YourDevName/led_basecolor_rgb/get, YourDevName/led_dimmer/get) |
| 1 | [MQTT] Broadcast led final color (topic name: YourDevName/led_finalcolor_rgb/get) |
| 2 | [MQTT] Broadcast self state every N (def: 60) seconds (delay configurable by 'mqtt_broadcastInterval' and 'mqtt_broadcastItemsPerSec' commands) |
| 3 | [LED][Debug] Show raw PWM controller on WWW index instead of new LED RGB/CW/etc picker |
| 4 | [LED] Force show RGBCW controller (for example, for SM2135 LEDs, or for DGR sender) |
| 5 | [CMD] Enable TCP console command server (for Putty, etc) |
| 6 | [BTN] Instant touch reaction instead of waiting for release (aka SetOption 13) |
| 7 | [MQTT] [Debug] Always set Retain flag to all published values |
| 8 | [LED] Alternate CW light mode (first PWM for warm/cold slider, second for brightness) |
| 9 | [SM2135] Use separate RGB/CW modes instead of writing all 5 values as RGB |
| 10 | [MQTT] Broadcast self state on MQTT connect |
| 11 | [PWM] BK7231 use 600hz instead of 1khz default |
| 12 | [LED] Remember LED driver state (RGBCW, enable, brightness, temperature) after reboot |
| 13 | [HTTP] Show actual PIN logic level for unconfigured pins |
| 14 | [IR] Do MQTT publish (RAW STRING) for incoming IR data |
| 15 | [IR] Allow 'unknown' protocol |
| 16 | [MQTT] Broadcast led final color RGBCW (topic name: YourDevName/led_finalcolor_rgbcw/get) |
| 17 | [LED] Automatically enable Light when changing brightness, color or temperature on WWW panel |
| 18 | [LED] Smooth transitions for LED (EXPERIMENTAL) |
| 19 | [MQTT] Always publish channels used by TuyaMCU |
| 20 | [LED] Force RGB mode (3 PWMs for LEDs) and ignore further PWMs if they are set |
| 21 | [MQTT] Retain power channels (Relay channels, etc) |
| 22 | [IR] Do MQTT publish (Tasmota JSON format) for incoming IR data |
| 23 | [LED] Automatically enable Light on any change of brightness, color or temperature |
| 24 | [LED] Emulate Cool White with RGB in device with four PWMS - Red is 0, Green 1, Blue 2, and Warm is 4 |
| 25 | [POWER] Allow negative current/power for power measurement (all chips, BL0937, BL0942, etc) |
| 26 | [UART] Use alternate UART for BL0942, CSE, TuyaMCU, etc |
| 27 | [HASS] Invoke HomeAssistant discovery on change to ip address, configuration |
| 28 | [LED] Setting RGB white (FFFFFF) enables temperature mode |
| 29 | [NETIF] Use short device name as a hostname instead of a long name |
| 30 | [MQTT] Enable Tasmota TELE etc publishes (for ioBroker etc) |
| 31 | [UART] Enable UART command line |
| 32 | [LED] Use old linear brightness mode, ignore gamma ramp |
| 33 | [MQTT] Apply channel type multiplier on (if any) on channel value before publishing it |
| 34 | [MQTT] In HA discovery, add relays as lights |
| 35 | [HASS] Deactivate avty_t flag when publishing to HASS (permit to keep value). You must restart HASS discovery for change to take effect. |
| 36 | [DRV] Deactivate Autostart of all drivers |
| 37 | [WiFi] Quick connect to WiFi on reboot (TODO: check if it works for you and report on github) |
| 38 | [Power] Set power and current to zero if all relays are open |
| 39 | [MQTT] [Debug] Publish all channels (don't enable it, it will be publish all 64 possible channels on connect) |
| 40 | [MQTT] Use kWh unit for energy consumption (total, last hour, today) instead of Wh |
| 41 | [BTN] Ignore all button events (aka child lock) |
| 42 | [DoorSensor] Invert state |
| 43 | [TuyaMCU] Use queue |
| 44 | [HTTP] Disable authentication in safe mode (not recommended) |
| 45 | [MQTT Discovery] Don't merge toggles and dimmers into lights |
| 46 | [TuyaMCU] Store raw data |
| 47 | [TuyaMCU] Store ALL data |
| 48 | [PWR] Invert AC dir |
| 49 | [HTTP] Hide ON/OFF for relays (only red/green buttons) |
| 50 | [MQTT] Never add get sufix |
| 51 | [WiFi] (RTL/BK) Enhanced fast connect by saving AP data to flash (preferable with Flag 37 & static ip). Quick reset 3 times to connect normally |
