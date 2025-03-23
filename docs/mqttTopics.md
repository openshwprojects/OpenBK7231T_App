# MQTT topics of published variables
Some MQTT variables are being published only at the startup, some are published periodically (if you enable "broadcast every N seconds" flag, default time is one minute, customizable with command mqtt_broadcastInterval), some are published only when a given value is changed. Below is the table of used publish topics (TODO: add full descriptions)

Hint: in HA, you can use MQTT wildcard to listen to multiple publishes. OBK_DEV_NAME/#

Publishes send by OBK device:
| Topic        | Sample Value          | Description  |
|:------------- |:------------- | -----:|
| OBK_DEV_NAME/INDEX/get | 1 | send when a given channel is changed. INDEX is a number representing channel index. Most of channels are not published by default. See flags to change it. If you want given channel to be published, you can also change its channel type in the Web App, just select one that fits your needs or just a generic 'Custom' type. You can have any variable in channel, even a custom, fully scriptable counter. |
| OBK_DEV_NAME/connected | online | Sent on connect. |
| OBK_DEV_NAME/sockets | 5 | Send on connect and every N seconds (default: 60, if enabled) |
| OBK_DEV_NAME/rssi | -70 | Send on connect and every N seconds (default: 60, if enabled) |
| OBK_DEV_NAME/uptime | 653 | Send on connect and every N seconds (default: 60, if enabled) |
| OBK_DEV_NAME/freeheap | 95168 | Send on connect and every N seconds (default: 60, if enabled) |
| OBK_DEV_NAME/ip | 192.168.0.123 | Send on connect and every N seconds (default: 60, if enabled) |
| OBK_DEV_NAME/datetime |  | Send on connect and every N seconds (default: 60, if enabled) |
| OBK_DEV_NAME/mac | 84:e3:42:65:d1:87  | Send on connect and every N seconds (default: 60, if enabled) |
| OBK_DEV_NAME/build | Build on Nov 12 2022 12:39:44 version 1.0.0 | Send on connect and every N seconds (default: 60, if enabled) |
| OBK_DEV_NAME/host | obk_t_fourRelays | Send on connect and every N seconds (default: 60, if enabled) |
| OBK_DEV_NAME/voltage/get | 221 | Voltage from BL0942/BL0937 etc, just like current and power |
| OBK_DEV_NAME/led_enableAll/get | 1 | Send when LED On/Off changes or when periodic broadcast is enabled |
| OBK_DEV_NAME/led_basecolor_rgb/get | FFAABB | Send when LED color changes or when periodic broadcast is enabled. |
| OBK_DEV_NAME/led_dimmer/get | 100 | Send when LED dimmer changes or when periodic broadcast is enabled |
| OBK_DEV_NAME/YOUR_TOPIC/get | YOUR_VALUE | You can publish anything with 'publish [YOUR_TOPIC] [YOUR_VALUE]' command |
| tele/OBK_DEV_NAME/STATE | Tasmota JSON | OBK can publish Tasmota STATE style message if you enable TELE/etc publishes in options. This is used for compatibility with ioBroker, etc |
| stat/OBK_DEV_NAME/RESULT | Tasmota JSON | See above. You can also see related self test code for details |
| tele/OBK_DEV_NAME/SENSOR | { "Time": "1970-01-01T00:00:00", "ENERGY": { "Power": 0, "ApparentPower": 0, "ReactivePower": 0, "Factor": 0, "Voltage": 249.932449, "Current": 0,"ConsumptionTotal": 255.346664,"ConsumptionLastHour": 0 }} | See above. Published by power metering devices, BL0937, BL0942, etc). Make sure NTP is running to get time. |
| [similar tasmota messages] | Tasmota JSON | See above. See related self test code for details |
| OBK_DEV_NAME/RESULT | { "IrReceived": { "Protocol": "Samsung", "Bits": 32, "Data": "0xEE110707" } } | Sent if Tasmota syntax IR publish is enabled in flags. NOTE: we may fix it and add tele prefix soon? |


Publishes received by OBK device:
| Topic        | Sample Value          | Description  |
|:------------- |:------------- | -----:|
| OBK_DEV_NAME/INDEX/set | 1 | Sets the channel of INDEX to given value. This can set relays and also provide DIRECT PWM access. If channel is mapped to TuyaMCU, TuyaMCU will also be updated |
| cmnd/OBK_DEV_NAME/COMMAND_TEXT | COMMAND_ARGUMENTS | You can execute any command supported by OpenBeken, just like in Tasmota |
| OBK_DEV_NAME/INDEX/get | no payload | You can send an empty 'get' publish to OBK device to request update on the state of given channel. OBK will reply back with the same topic but with payload representing given channel value. |
| OBK_DEV_NAME/VARIABLE/get | no payload | You can send an empty publish with VARIABLE="led_dimmer"/"led_enableAll", etc etc, to query publish of given variable manually. OBK device will reply with publishing given variable. |
