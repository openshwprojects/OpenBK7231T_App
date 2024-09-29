#ifndef __NEW_PINS_H__
#define __NEW_PINS_H__
#include "new_common.h"
#include "hal/hal_wifi.h"

#define test12321321  321321321

typedef enum ioRole_e {
	//iodetail:{"name":"None",
	//iodetail:"title":"TODO",
	//iodetail:"descr":"Default pin role; this pin does nothing.",
	//iodetail:"enum":"IOR_None",
	//iodetail:"file":"new_pins.h",
	//iodetail:"driver":""}
	IOR_None,
	//iodetail:{"name":"Relay",
	//iodetail:"title":"TODO",
	//iodetail:"descr":"an active-high relay. This relay is closed when a logical 1 value is on linked channel",
	//iodetail:"enum":"IOR_Relay",
	//iodetail:"file":"new_pins.h",
	//iodetail:"driver":""}
	IOR_Relay,
	//iodetail:{"name":"Relay_n",
	//iodetail:"title":"TODO",
	//iodetail:"descr":"(as Relay but pin logical value is inversed)",
	//iodetail:"enum":"IOR_Relay_n",
	//iodetail:"file":"new_pins.h",
	//iodetail:"driver":""}
	IOR_Relay_n,
	//iodetail:{"name":"Button",
	//iodetail:"title":"TODO",
	//iodetail:"descr":"a typical button of Tuya device with active-low state (a button that connects IO pin to ground when pressed and also has a 10k or so pull up resistor)",
	//iodetail:"enum":"IOR_Button",
	//iodetail:"file":"new_pins.h",
	//iodetail:"driver":""}
	IOR_Button,
	//iodetail:{"name":"Button_n",
	//iodetail:"title":"TODO",
	//iodetail:"descr":"as Button but pin logical value is inversed",
	//iodetail:"enum":"IOR_Button_n",
	//iodetail:"file":"new_pins.h",
	//iodetail:"driver":""}
	IOR_Button_n,
	//iodetail:{"name":"LED",
	//iodetail:"title":"TODO",
	//iodetail:"descr":"an active-high LED. The internals of 'LED' are the same as of 'Relay'. Names are just separate to make it easier for users.",
	//iodetail:"enum":"IOR_LED",
	//iodetail:"file":"new_pins.h",
	//iodetail:"driver":""}
	IOR_LED,
	//iodetail:{"name":"LED_n",
	//iodetail:"title":"TODO",
	//iodetail:"descr":"(as Led but pin logical value is inversed)",
	//iodetail:"enum":"IOR_LED_n",
	//iodetail:"file":"new_pins.h",
	//iodetail:"driver":""}
	IOR_LED_n,
	//iodetail:{"name":"PWM",
	//iodetail:"title":"TODO",
	//iodetail:"descr":"Pulse width modulation output for LED dimmers (with MQTT dimming support from Home Assistant). Remember to set related channel to correct color index, in the RGBCW order. For CW only lights, set only CW indices.",
	//iodetail:"enum":"IOR_PWM",
	//iodetail:"file":"new_pins.h",
	//iodetail:"driver":""}
	IOR_PWM,
	//iodetail:{"name":"LED_WIFI",
	//iodetail:"title":"TODO",
	//iodetail:"descr":"special LED to indicate WLan connection state. LED states are following: LED on = client mode successfully connected to your Router. Half a second blink - connecting to your router, please wait (or connection problem). Fast blink (200ms) - open access point mode. In safe mode (after failed boots), LED might not work.",
	//iodetail:"enum":"IOR_LED_WIFI",
	//iodetail:"file":"new_pins.h",
	//iodetail:"driver":""}
	IOR_LED_WIFI,
	//iodetail:{"name":"LED_WIFI_n",
	//iodetail:"title":"TODO",
	//iodetail:"descr":"As LED_WIFI, but with inversed logic.",
	//iodetail:"enum":"IOR_LED_WIFI_n",
	//iodetail:"file":"new_pins.h",
	//iodetail:"driver":""}
	IOR_LED_WIFI_n,
	//iodetail:{"name":"Button_ToggleAll",
	//iodetail:"title":"TODO",
	//iodetail:"descr":"this button toggles all channels at once",
	//iodetail:"enum":"IOR_Button_ToggleAll",
	//iodetail:"file":"new_pins.h",
	//iodetail:"driver":""}
	IOR_Button_ToggleAll,
	//iodetail:{"name":"Button_ToggleAll_n",
	//iodetail:"title":"TODO",
	//iodetail:"descr":"Button_ToggleAll as, but inversed logic of button",
	//iodetail:"enum":"IOR_Button_ToggleAll_n",
	//iodetail:"file":"new_pins.h",
	//iodetail:"driver":""}
	IOR_Button_ToggleAll_n,
	//iodetail:{"name":"DigitalInput",
	//iodetail:"title":"TODO",
	//iodetail:"descr":"this is a simple digital input pin, it sets the linked channel to current logical value on it, just like digitalRead( ) from Arduino. This input has a internal pull up resistor.",
	//iodetail:"enum":"IOR_DigitalInput",
	//iodetail:"file":"new_pins.h",
	//iodetail:"driver":""}
	IOR_DigitalInput,
	//iodetail:{"name":"DigitalInput_n",
	//iodetail:"title":"TODO",
	//iodetail:"descr":"As DigitalInput as above, but inverted",
	//iodetail:"enum":"IOR_DigitalInput_n",
	//iodetail:"file":"new_pins.h",
	//iodetail:"driver":""}
	IOR_DigitalInput_n,
	//iodetail:{"name":"ToggleChannelOnToggle",
	//iodetail:"title":"TODO",
	//iodetail:"descr":"this pin will toggle target channel when a value on this pin changes (with debouncing). you can connect simple two position switch here and swapping the switch will toggle target channel relay on or off",
	//iodetail:"enum":"IOR_ToggleChannelOnToggle",
	//iodetail:"file":"new_pins.h",
	//iodetail:"driver":""}
	IOR_ToggleChannelOnToggle,
	//iodetail:{"name":"DigitalInput_NoPup",
	//iodetail:"title":"TODO",
	//iodetail:"descr":"As DigitalInput, but without internal programmable pullup resistor. This is used for, for example, XR809 water sensor and door sensor.",
	//iodetail:"enum":"IOR_DigitalInput_NoPup",
	//iodetail:"file":"new_pins.h",
	//iodetail:"driver":""}
	IOR_DigitalInput_NoPup,
	//iodetail:{"name":"DigitalInput_NoPup_n",
	//iodetail:"title":"TODO",
	//iodetail:"descr":"As DigitalInput_n, but without internal programmable pullup resistor",
	//iodetail:"enum":"IOR_DigitalInput_NoPup_n",
	//iodetail:"file":"new_pins.h",
	//iodetail:"driver":""}
	IOR_DigitalInput_NoPup_n,
	// energy sensor
		//iodetail:{"name":"BL0937_SEL",
		//iodetail:"title":"TODO",
		//iodetail:"descr":"SEL pin for BL0937 energy measuring devices. Set all BL0937 pins to autostart BL0937 driver. Don't forget to calibrate it later.",
		//iodetail:"enum":"IOR_BL0937_SEL",
		//iodetail:"file":"new_pins.h",
		//iodetail:"driver":""}
	IOR_BL0937_SEL,
	//iodetail:{"name":"BL0937_CF",
	//iodetail:"title":"TODO",
	//iodetail:"descr":"CF pin for BL0937 energy measuring devices. Set all BL0937 pins to autostart BL0937 driver. Don't forget to calibrate it later.",
	//iodetail:"enum":"IOR_BL0937_CF",
	//iodetail:"file":"new_pins.h",
	//iodetail:"driver":""}
	IOR_BL0937_CF,
	//iodetail:{"name":"BL0937_CF1",
	//iodetail:"title":"TODO",
	//iodetail:"descr":"CF1 pin for BL0937 energy measuring devices. Set all BL0937 pins to autostart BL0937 driver. Don't forget to calibrate it later.",
	//iodetail:"enum":"IOR_BL0937_CF1",
	//iodetail:"file":"new_pins.h",
	//iodetail:"driver":""}
	IOR_BL0937_CF1,
	//iodetail:{"name":"ADC",
	//iodetail:"title":"TODO",
	//iodetail:"descr":"Analog to Digital converter converts voltage to channel value which is later published by MQTT and also can be used to trigger scriptable events",
	//iodetail:"enum":"IOR_ADC",
	//iodetail:"file":"new_pins.h",
	//iodetail:"driver":""}
	IOR_ADC,
	//iodetail:{"name":"SM2135_DAT",
	//iodetail:"title":"TODO",
	//iodetail:"descr":"SM2135 DAT pin for SM2135 modified-I2C twowire LED driver, used in RGBCW lights. Set both required SM2135 pins to autostart the related driver. Don't forget to Map the colors order later, so colors are not mixed.",
	//iodetail:"enum":"IOR_SM2135_DAT",
	//iodetail:"file":"new_pins.h",
	//iodetail:"driver":""}
	IOR_SM2135_DAT,
	//iodetail:{"name":"SM2135_CLK",
	//iodetail:"title":"TODO",
	//iodetail:"descr":"SM2135 CLK pin for SM2135 modified-I2C twowire LED driver, used in RGBCW lights. Set both required SM2135 pins to autostart the related driver. Don't forget to Map the colors order later, so colors are not mixed.",
	//iodetail:"enum":"IOR_SM2135_CLK",
	//iodetail:"file":"new_pins.h",
	//iodetail:"driver":""}
	IOR_SM2135_CLK,
	//iodetail:{"name":"BP5758D_DAT",
	//iodetail:"title":"TODO",
	//iodetail:"descr":"BP5758D DAT pin for BP5758D modified-I2C twowire LED driver, used in RGBCW lights. Set both required BP5758D pins to autostart the related driver. Don't forget to Map the colors order later, so colors are not mixed.",
	//iodetail:"enum":"IOR_BP5758D_DAT",
	//iodetail:"file":"new_pins.h",
	//iodetail:"driver":""}
	IOR_BP5758D_DAT,
	//iodetail:{"name":"BP5758D_CLK",
	//iodetail:"title":"TODO",
	//iodetail:"descr":"BP5758D CLK pin for BP5758D modified-I2C twowire LED driver, used in RGBCW lights. Set both required BP5758D pins to autostart the related driver. Don't forget to Map the colors order later, so colors are not mixed.",
	//iodetail:"enum":"IOR_BP5758D_CLK",
	//iodetail:"file":"new_pins.h",
	//iodetail:"driver":""}
	IOR_BP5758D_CLK,
	//iodetail:{"name":"BP1658CJ_DAT",
	//iodetail:"title":"TODO",
	//iodetail:"descr":"BP1658CJ DAT pin for BP5758D modified-I2C twowire LED driver, used in RGBCW lights. Set both required BP1658CJ pins to autostart the related driver. Don't forget to Map the colors order later, so colors are not mixed.",
	//iodetail:"enum":"IOR_BP1658CJ_DAT",
	//iodetail:"file":"new_pins.h",
	//iodetail:"driver":""}
	IOR_BP1658CJ_DAT,
	//iodetail:{"name":"BP1658CJ_CLK",
	//iodetail:"title":"TODO",
	//iodetail:"descr":"BP1658CJ CLK pin for BP5758D modified-I2C twowire LED driver, used in RGBCW lights. Set both required BP1658CJ pins to autostart the related driver. Don't forget to Map the colors order later, so colors are not mixed.",
	//iodetail:"enum":"IOR_BP1658CJ_CLK",
	//iodetail:"file":"new_pins.h",
	//iodetail:"driver":""}
	IOR_BP1658CJ_CLK,
	//iodetail:{"name":"PWM_n",
	//iodetail:"title":"TODO",
	//iodetail:"descr":"as above, but percentage of duty is inversed. This might be useful for some special LED drivers that are using single PWM to choose between Cool white and Warm white (it also needs setting a special flag in General options)",
	//iodetail:"enum":"IOR_PWM_n",
	//iodetail:"file":"new_pins.h",
	//iodetail:"driver":""}
	IOR_PWM_n,
	//iodetail:{"name":"IRRecv",
	//iodetail:"title":"TODO",
	//iodetail:"descr":"IR receiver for our IRLibrary port",
	//iodetail:"enum":"IOR_IRRecv",
	//iodetail:"file":"new_pins.h",
	//iodetail:"driver":""}
	IOR_IRRecv,
	//iodetail:{"name":"IRSend",
	//iodetail:"title":"TODO",
	//iodetail:"descr":"IR sender for our IRLibrary port",
	//iodetail:"enum":"IOR_IRSend",
	//iodetail:"file":"new_pins.h",
	//iodetail:"driver":""}
	IOR_IRSend,
	//iodetail:{"name":"Button_NextColor",
	//iodetail:"title":"TODO",
	//iodetail:"descr":"for RGB strip with buttons; sets next predefined color. For a LED strip that has separate POWER and COLOR buttons.",
	//iodetail:"enum":"IOR_Button_NextColor",
	//iodetail:"file":"new_pins.h",
	//iodetail:"driver":""}
	IOR_Button_NextColor,
	//iodetail:{"name":"Button_NextColor_n",
	//iodetail:"title":"TODO",
	//iodetail:"descr":"As NextColor, but inversed button logic",
	//iodetail:"enum":"IOR_Button_NextColor_n",
	//iodetail:"file":"new_pins.h",
	//iodetail:"driver":""}
	IOR_Button_NextColor_n,
	//iodetail:{"name":"Button_NextDimmer",
	//iodetail:"title":"TODO",
	//iodetail:"descr":"for RGB strip with buttons; when hold, adjusts the brightness",
	//iodetail:"enum":"IOR_Button_NextDimmer",
	//iodetail:"file":"new_pins.h",
	//iodetail:"driver":""}
	IOR_Button_NextDimmer,
	//iodetail:{"name":"Button_NextDimmer_n",
	//iodetail:"title":"TODO",
	//iodetail:"descr":"As NextDimmer, but inversed button logic",
	//iodetail:"enum":"IOR_Button_NextDimmer_n",
	//iodetail:"file":"new_pins.h",
	//iodetail:"driver":""}
	IOR_Button_NextDimmer_n,
	//iodetail:{"name":"AlwaysHigh",
	//iodetail:"title":"TODO",
	//iodetail:"descr":"always outputs 1",
	//iodetail:"enum":"IOR_AlwaysHigh",
	//iodetail:"file":"new_pins.h",
	//iodetail:"driver":""}
	IOR_AlwaysHigh,
	//iodetail:{"name":"AlwaysLow",
	//iodetail:"title":"TODO",
	//iodetail:"descr":"always outputs 0",
	//iodetail:"enum":"IOR_AlwaysLow",
	//iodetail:"file":"new_pins.h",
	//iodetail:"driver":""}
	IOR_AlwaysLow,
	//iodetail:{"name":"UCS1912_DIN",
	//iodetail:"title":"TODO",
	//iodetail:"descr":"WIP driver, write a post on Elektroda if you need it working",
	//iodetail:"enum":"IOR_UCS1912_DIN",
	//iodetail:"file":"new_pins.h",
	//iodetail:"driver":""}
	IOR_UCS1912_DIN,
	//iodetail:{"name":"SM16703P_DIN",
	//iodetail:"title":"TODO",
	//iodetail:"descr":"WIP driver, write a post on Elektroda if you need it working",
	//iodetail:"enum":"IOR_SM16703P_DIN",
	//iodetail:"file":"new_pins.h",
	//iodetail:"driver":""}
	IOR_SM16703P_DIN,
	//iodetail:{"name":"Button_NextTemperature",
	//iodetail:"title":"TODO",
	//iodetail:"descr":"Button that automatically allows you to control temperature of your LED device",
	//iodetail:"enum":"IOR_Button_NextTemperature",
	//iodetail:"file":"new_pins.h",
	//iodetail:"driver":""}
	IOR_Button_NextTemperature,
	//iodetail:{"name":"Button_NextTemperature_n",
	//iodetail:"title":"TODO",
	//iodetail:"descr":"Like Button_NextTemperature, but inversed button logic",
	//iodetail:"enum":"IOR_Button_NextTemperature_n",
	//iodetail:"file":"new_pins.h",
	//iodetail:"driver":""}
	IOR_Button_NextTemperature_n,
	//iodetail:{"name":"Button_ScriptOnly",
	//iodetail:"title":"TODO",
	//iodetail:"descr":"This button does nothing automatically, even the linked channel is not changed. Useful for scripts, but you can still also use any buttons for scripting.",
	//iodetail:"enum":"IOR_Button_ScriptOnly",
	//iodetail:"file":"new_pins.h",
	//iodetail:"driver":""}
	IOR_Button_ScriptOnly,
	//iodetail:{"name":"Button_ScriptOnly_n",
	//iodetail:"title":"TODO",
	//iodetail:"descr":"Like Button_ScriptOnly, but inversed logic",
	//iodetail:"enum":"IOR_Button_ScriptOnly_n",
	//iodetail:"file":"new_pins.h",
	//iodetail:"driver":""}
	IOR_Button_ScriptOnly_n,
	//iodetail:{"name":"DHT11",
	//iodetail:"title":"TODO",
	//iodetail:"descr":"DHT11 data line. You can have multiple DHT sensors on your device. Related driver is automatically started. Results are saved in related channels to pin with that role (when editing pins, you get two textboxes to set channel indexes)",
	//iodetail:"enum":"IOR_DHT11",
	//iodetail:"file":"new_pins.h",
	//iodetail:"driver":""}
	IOR_DHT11,
	//iodetail:{"name":"DHT12",
	//iodetail:"title":"TODO",
	//iodetail:"descr":"DHT12 data line. You can have multiple DHT sensors on your device. Related driver is automatically started. Results are saved in related channels to pin with that role (when editing pins, you get two textboxes to set channel indexes)",
	//iodetail:"enum":"IOR_DHT12",
	//iodetail:"file":"new_pins.h",
	//iodetail:"driver":""}
	IOR_DHT12,
	//iodetail:{"name":"DHT21",
	//iodetail:"title":"TODO",
	//iodetail:"descr":"DHT21 data line. You can have multiple DHT sensors on your device. Related driver is automatically started. Results are saved in related channels to pin with that role (when editing pins, you get two textboxes to set channel indexes)",
	//iodetail:"enum":"IOR_DHT21",
	//iodetail:"file":"new_pins.h",
	//iodetail:"driver":""}
	IOR_DHT21,
	//iodetail:{"name":"DHT22",
	//iodetail:"title":"TODO",
	//iodetail:"descr":"DHT22 data line. You can have multiple DHT sensors on your device. Related driver is automatically started. Results are saved in related channels to pin with that role (when editing pins, you get two textboxes to set channel indexes)",
	//iodetail:"enum":"IOR_DHT22",
	//iodetail:"file":"new_pins.h",
	//iodetail:"driver":""}
	IOR_DHT22,
	//iodetail:{"name":"CHT83XX_DAT",
	//iodetail:"title":"TODO",
	//iodetail:"descr":"DAT pin of CHT83XX. Setting this pin role and saving will reveal two fields next to it. Set first field to 1 and second to 2. Those are related channel numbers to store temperature and humidity.",
	//iodetail:"enum":"IOR_CHT83XX_DAT",
	//iodetail:"file":"new_pins.h",
	//iodetail:"driver":""}
	IOR_CHT83XX_DAT,
	//iodetail:{"name":"CHT83XX_CLK",
	//iodetail:"title":"TODO",
	//iodetail:"descr":"CLK pin of CHT83XX sensor",
	//iodetail:"enum":"IOR_CHT83XX_CLK",
	//iodetail:"file":"new_pins.h",
	//iodetail:"driver":""}
	IOR_CHT83XX_CLK,
	//iodetail:{"name":"SHT3X_DAT",
	//iodetail:"title":"TODO",	
	//iodetail:"descr":"Humidity/temperature sensor DATA pin. Driver will autostart if both required pins are set. See [SHT Sensor tutorial topic here](https://www.elektroda.com/rtvforum/topic3958369.html), also see [this sensor teardown](https://www.elektroda.com/rtvforum/topic3945688.html)",
	//iodetail:"enum":"IOR_SHT3X_DAT",
	//iodetail:"file":"new_pins.h",
	//iodetail:"driver":""}
	IOR_SHT3X_DAT,
	//iodetail:{"name":"SHT3X_CLK",
	//iodetail:"title":"TODO",
	//iodetail:"descr":"Humidity/temperature sensor CLOCK pin. Driver will autostart if both required pins are set. See [SHT Sensor tutorial topic here](https://www.elektroda.com/rtvforum/topic3958369.html), also see [this sensor teardown](https://www.elektroda.com/rtvforum/topic3945688.html)",
	//iodetail:"enum":"IOR_SHT3X_CLK",
	//iodetail:"file":"new_pins.h",
	//iodetail:"driver":""}
	IOR_SHT3X_CLK,
	//iodetail:{"name":"SOFT_SDA",
	//iodetail:"title":"TODO",
	//iodetail:"descr":"Generic software SDA pin for our more advanced, scriptable I2C driver. This allows you to even connect a I2C display to OBK.",
	//iodetail:"enum":"IOR_SOFT_SDA",
	//iodetail:"file":"new_pins.h",
	//iodetail:"driver":""}
	IOR_SOFT_SDA,
	//iodetail:{"name":"SOFT_SCL",
	//iodetail:"title":"TODO",
	//iodetail:"descr":"Generic software SCL pin for our more advanced, scriptable I2C driver.",
	//iodetail:"enum":"IOR_SOFT_SCL",
	//iodetail:"file":"new_pins.h",
	//iodetail:"driver":""}
	IOR_SOFT_SCL,
	//iodetail:{"name":"SM2235_DAT",
	//iodetail:"title":"TODO",	
	//iodetail:"descr":"It works for both SM2235 and SM2335. SM2235 DAT pin for SM2235 modified-I2C twowire LED driver, used in RGBCW lights. Set both required SM2235 pins to autostart the related driver. Don't forget to Map the colors order later, so colors are not mixed.",
	//iodetail:"enum":"IOR_SM2235_DAT",
	//iodetail:"file":"new_pins.h",
	//iodetail:"driver":""}
	IOR_SM2235_DAT,
	//iodetail:{"name":"SM2235_CLK",
	//iodetail:"title":"TODO",	
	//iodetail:"descr":"It works for both SM2235 and SM2335. SM2235 CLK pin for SM2235 modified-I2C twowire LED driver, used in RGBCW lights. Set both required SM2235 pins to autostart the related driver. Don't forget to Map the colors order later, so colors are not mixed.",
	//iodetail:"enum":"IOR_SM2235_CLK",
	//iodetail:"file":"new_pins.h",
	//iodetail:"driver":""}
	IOR_SM2235_CLK,
	//iodetail:{"name":"BridgeForward",
	//iodetail:"title":"TODO",
	//iodetail:"descr":"Motor/Relay bridge driver control signal. FORWARD direction.",
	//iodetail:"enum":"IOR_BridgeForward",
	//iodetail:"file":"new_pins.h",
	//iodetail:"driver":""}
	IOR_BridgeForward,
	//iodetail:{"name":"BridgeReverse",
	//iodetail:"title":"TODO",
	//iodetail:"descr":"Motor/Relay bridge driver control signal. REVERSE direction.",
	//iodetail:"enum":"IOR_BridgeReverse",
	//iodetail:"file":"new_pins.h",
	//iodetail:"driver":""}
	IOR_BridgeReverse,
	//iodetail:{"name":"SmartButtonForLEDs",
	//iodetail:"title":"TODO",
	//iodetail:"descr":"A single button that does all control for LED. Click it toggle power, hold to adjust brightness, double click for next color, triple click for next temperature",
	//iodetail:"enum":"IOR_SmartButtonForLEDs",
	//iodetail:"file":"new_pins.h",
	//iodetail:"driver":""}
	IOR_SmartButtonForLEDs,
	//iodetail:{"name":"SmartButtonForLEDs_n",
	//iodetail:"title":"TODO",
	//iodetail:"descr":"As SmartButtonForLEDs, but inverted",
	//iodetail:"enum":"IOR_SmartButtonForLEDs_n",
	//iodetail:"file":"new_pins.h",
	//iodetail:"driver":""}
	IOR_SmartButtonForLEDs_n,
	//iodetail:{"name":"DoorSensorWithDeepSleep",
	//iodetail:"title":"TODO",
	//iodetail:"descr":"Setting this role will make DoorSensor driver autostart. DoorSensor will work like digital input, sending only its value on change. When there are no changes for some times, device will go into deep sleep to save battery. When a change occurs, device will wake up and report change. See [a door sensor example here](https://www.elektroda.com/rtvforum/topic3960149.html)",
	//iodetail:"enum":"IOR_DoorSensorWithDeepSleep",
	//iodetail:"file":"new_pins.h",
	//iodetail:"driver":""}
	IOR_DoorSensorWithDeepSleep,
	//iodetail:{"name":"DoorSensorWithDeepSleep_NoPup",
	//iodetail:"title":"TODO",
	//iodetail:"descr":"As DoorSensorWithDeepSleep, but no pullup resistor",
	//iodetail:"enum":"IOR_DoorSensorWithDeepSleep_NoPup",
	//iodetail:"file":"new_pins.h",
	//iodetail:"driver":""}
	IOR_DoorSensorWithDeepSleep_NoPup,
	//iodetail:{"name":"BAT_ADC",
	//iodetail:"title":"TODO",
	//iodetail:"descr":"Like ADC, but for a Battery driver that does Battery measurement. See [battery driver topic here](https://www.elektroda.com/rtvforum/topic3959103.html)",
	//iodetail:"enum":"IOR_BAT_ADC",
	//iodetail:"file":"new_pins.h",
	//iodetail:"driver":""}
	IOR_BAT_ADC,
	//iodetail:{"name":"BAT_Relay",
	//iodetail:"title":"TODO",
	//iodetail:"descr":"Like Relay, but for a Battery driver that does Battery measurement. See [battery driver topic here](https://www.elektroda.com/rtvforum/topic3959103.html)",
	//iodetail:"enum":"IOR_BAT_Relay",
	//iodetail:"file":"new_pins.h",
	//iodetail:"driver":""}
	IOR_BAT_Relay,
	//iodetail:{"name":"TM1637_DIO",
	//iodetail:"title":"TODO",
	//iodetail:"descr":"TM1637 LED display driver DIO pin. Setting all required TM1637 pins will autostart related driver",
	//iodetail:"enum":"IOR_TM1637_DIO",
	//iodetail:"file":"new_pins.h",
	//iodetail:"driver":""}
	IOR_TM1637_DIO,
	//iodetail:{"name":"TM1637_CLK",
	//iodetail:"title":"TODO",
	//iodetail:"descr":"TM1637 LED display driver CLK pin. Setting all required TM1637 pins will autostart related driver",
	//iodetail:"enum":"IOR_TM1637_CLK",
	//iodetail:"file":"new_pins.h",
	//iodetail:"driver":""}
	IOR_TM1637_CLK,
	//iodetail:{"name":"BL0937_SEL_n",
	//iodetail:"title":"TODO",
	//iodetail:"descr":"Inverted SEL alternative for BL0937. Choose only one, either SEL or SEL_n. SEL_n may be needed in rare cases.",
	//iodetail:"enum":"IOR_BL0937_SEL_n",
	//iodetail:"file":"new_pins.h",
	//iodetail:"driver":""}
	IOR_BL0937_SEL_n,
	//iodetail:{"name":"DoorSensorWithDeepSleep_pd",
	//iodetail:"title":"TODO",
	//iodetail:"descr":"As DoorSensorWithDeepSleep, but with pulldown resistor",
	//iodetail:"enum":"IOR_DoorSensorWithDeepSleep_pd",
	//iodetail:"file":"new_pins.h",
	//iodetail:"driver":""}
	IOR_DoorSensorWithDeepSleep_pd,
	//iodetail:{"name":"SGP_CLK",
	//iodetail:"title":"TODO",
	//iodetail:"descr":"SGP Quality Sensor Clock line. will autostart related driver",
	//iodetail:"enum":"IOR_SGP_CLK",
	//iodetail:"file":"new_pins.h",
	//iodetail:"driver":""}
	IOR_SGP_CLK,
	//iodetail:{"name":"SGP_DAT",
	//iodetail:"title":"TODO",
	//iodetail:"descr":"SGP Quality Sensor Data line. will autostart related driver",
	//iodetail:"enum":"IOR_SGP_DAT",
	//iodetail:"file":"new_pins.h",
	//iodetail:"driver":""}
	IOR_SGP_DAT,
	//iodetail:{"name":"ADC_Button",
	//iodetail:"title":"TODO",
	//iodetail:"descr":"Single ADC with multiple buttons connected.d",
	//iodetail:"enum":"ADC_Button",
	//iodetail:"file":"new_pins.h",
	//iodetail:"driver":""}
	IOR_ADC_Button,
	//iodetail:{"name":"GN6932_CLK",
	//iodetail:"title":"TODO",
	//iodetail:"descr":"QQQ",
	//iodetail:"enum":"GN6932_CLK",
	//iodetail:"file":"new_pins.h",
	//iodetail:"driver":""}
	IOR_GN6932_CLK,
	//iodetail:{"name":"GN6932_DAT",
	//iodetail:"title":"TODO",
	//iodetail:"descr":"QQQ",
	//iodetail:"enum":"GN6932_DAT",
	//iodetail:"file":"new_pins.h",
	//iodetail:"driver":""}
	IOR_GN6932_DAT,
	//iodetail:{"name":"GN6932_STB",
	//iodetail:"title":"TODO",
	//iodetail:"descr":"QQQ",
	//iodetail:"enum":"GN6932_STB",
	//iodetail:"file":"new_pins.h",
	//iodetail:"driver":""}
	IOR_GN6932_STB,
	//iodetail:{"name":"TM1638_CLK",
	//iodetail:"title":"TODO",
	//iodetail:"descr":"QQQ",
	//iodetail:"enum":"TM1638_CLK",
	//iodetail:"file":"new_pins.h",
	//iodetail:"driver":""}
	IOR_TM1638_CLK,
	//iodetail:{"name":"TM1638_DAT",
	//iodetail:"title":"TODO",
	//iodetail:"descr":"QQQ",
	//iodetail:"enum":"TM1638_DAT",
	//iodetail:"file":"new_pins.h",
	//iodetail:"driver":""}
	IOR_TM1638_DAT,
	//iodetail:{"name":"TM1638_STB",
	//iodetail:"title":"TODO",
	//iodetail:"descr":"QQQ",
	//iodetail:"enum":"TM1638_STB",
	//iodetail:"file":"new_pins.h",
	//iodetail:"driver":""}
	IOR_TM1638_STB,
	//iodetail:{"name":"BAT_Relay_n",
	//iodetail:"title":"TODO",
	//iodetail:"descr":"Like BAT_Relay, but inversed. See [battery driver topic here](https://www.elektroda.com/rtvforum/topic3959103.html)",
	//iodetail:"enum":"IOR_BAT_Relay_n",
	//iodetail:"file":"new_pins.h",
	//iodetail:"driver":""}
	IOR_BAT_Relay_n,
	//iodetail:{"name":"KP18058_CLK",
	//iodetail:"title":"TODO",
	//iodetail:"descr":"QQQ",
	//iodetail:"enum":"KP18058_CLK",
	//iodetail:"file":"new_pins.h",
	//iodetail:"driver":""}
	IOR_KP18058_CLK,
	//iodetail:{"name":"KP18058_DAT",
	//iodetail:"title":"TODO",
	//iodetail:"descr":"QQQ",
	//iodetail:"enum":"KP18058_DAT",
	//iodetail:"file":"new_pins.h",
	//iodetail:"driver":""}
	IOR_KP18058_DAT,
	//iodetail:{"name":"DS1820",
	//iodetail:"title":"TODO",
	//iodetail:"descr":"vers simple OneWire Temp sensor DS1820",
	//iodetail:"enum":"DS1820_IO",
	//iodetail:"file":"new_pins.h",
	//iodetail:"driver":""}
	IOR_DS1820_IO,
	//iodetail:{"name":"Total_Options",
	//iodetail:"title":"TODO",
	//iodetail:"descr":"Current total number of available IOR roles",
	//iodetail:"enum":"IOR_Total_Options",
	//iodetail:"file":"new_pins.h",
	//iodetail:"driver":""}
	IOR_Total_Options,
} ioRole_t;

#define IS_PIN_DHT_ROLE(role) (((role)>=IOR_DHT11) && ((role)<=IOR_DHT22))
#define IS_PIN_TEMP_HUM_SENSOR_ROLE(role) (((role)==IOR_SHT3X_DAT) || ((role)==IOR_CHT83XX_DAT))
#define IS_PIN_AIR_SENSOR_ROLE(role) (((role)==IOR_SGP_DAT))

typedef enum channelType_e {
	//chandetail:{"name":"Default",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"Default channel type. This channel type is often used in simple devices and is suitable for relays. You don't need to use anything else for most of the non-TuyaMCU devices.",
	//chandetail:"enum":"ChType_Default",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_Default,
	//chandetail:{"name":"Error",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"This is used to indicate an error.",
	//chandetail:"enum":"ChType_Error",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_Error,
	//chandetail:{"name":"Temperature",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"This channel type represents a temperature in degrees. The temperature is shown as a read only, integer value on WWW panel.",
	//chandetail:"enum":"ChType_Temperature",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_Temperature,
	//chandetail:{"name":"Humidity",
	//chandetail:"title":"TODO",	
	//chandetail:"descr":"This channel type represents a humidity in percent. The humidity is shown as a read only, integer value on WWW panel.",
	//chandetail:"enum":"ChType_Humidity",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_Humidity,
	//chandetail:{"name":"Humidity_div10",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"This is also humidity, but in TuyaMCU format, multiplied by 10, so 554 is 55.4%. Main WWW panel displays it correctly. If you want multiplied value to be published to MQTT, check flags.",
	//chandetail:"enum":"ChType_Humidity_div10",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_Humidity_div10,
	//chandetail:{"name":"Temperature_div10",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"Just like humidity_div10, but for temperature.",
	//chandetail:"enum":"ChType_Temperature_div10",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_Temperature_div10,
	//chandetail:{"name":"Toggle",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"This channel will show ON/OFF toggle, so it is very much like default channel type, but the toggle will be always shown, even if channel is not used",
	//chandetail:"enum":"ChType_Toggle",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_Toggle,
	//chandetail:{"name":"Dimmer",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"A custom dimmer channel. This will have a 0-100 range and will show a slider. The slider value will be saved to channel, but nothing else is done automatically. You can map it to TuyaMCU dpID to get dimming working or script it however you like.",
	//chandetail:"enum":"ChType_Dimmer",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_Dimmer,
	//chandetail:{"name":"LowMidHigh",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"This channel has 3 values: 0, 1 and 2. This will show radio selection with those 3 options on the main WWW panel.",
	//chandetail:"enum":"ChType_LowMidHigh",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_LowMidHigh,
	//chandetail:{"name":"TextField",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"This is a custom textfield channel, where you can type any value. Used for testing, can be also used for time countdown on TuyaMCU devices. Text field will be shown on main WWW panel",
	//chandetail:"enum":"ChType_TextField",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_TextField,
	//chandetail:{"name":"ReadOnly",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"This channel is read only. It will just print its value on main WWW page. Of course, you can still write to it with console commands and scripts.",
	//chandetail:"enum":"ChType_ReadOnly",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_ReadOnly,
	//chandetail:{"name":"OffLowMidHigh",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"Like LowMidHigh, but with 4 options. Some of TuyaMCU fans might require that.",
	//chandetail:"enum":"ChType_OffLowMidHigh",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_OffLowMidHigh,
	//chandetail:{"name":"OffLowestLowMidHighHighest",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"Like LowMidHigh, but more options. Some of TuyaMCU fans might require that.",
	//chandetail:"enum":"ChType_OffLowestLowMidHighHighest",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_OffLowestLowMidHighHighest,
	//chandetail:{"name":"LowestLowMidHighHighest",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"Like LowMidHigh, but more options. Some of TuyaMCU fans might require that.",
	//chandetail:"enum":"ChType_LowestLowMidHighHighest",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_LowestLowMidHighHighest,
	//chandetail:{"name":"Dimmer256",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"Just like dimmer, but it's using 0-255 range. Everything else is the same.",
	//chandetail:"enum":"ChType_Dimmer256",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_Dimmer256,
	//chandetail:{"name":"Dimmer1000",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"Just like dimmer, but it's using 0-1000 range. Everything else is the same.",
	//chandetail:"enum":"ChType_Dimmer1000",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_Dimmer1000,
	//chandetail:{"name":"Frequency_div100",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"For TuyaMCU power metering. Not used for BL09** and CSE** sensors. Divider is used by TuyaMCU, because TuyaMCU sends always values as integers so we have to divide them before displaying on UI",
	//chandetail:"enum":"ChType_Frequency_div100",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_Frequency_div100,
	//chandetail:{"name":"Voltage_div10",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"For TuyaMCU power metering. Not used for BL09** and CSE** sensors. Divider is used by TuyaMCU, because TuyaMCU sends always values as integers so we have to divide them before displaying on UI",
	//chandetail:"enum":"ChType_Voltage_div10",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_Voltage_div10,
	//chandetail:{"name":"Power",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"For TuyaMCU power metering. Not used for BL09** and CSE** sensors. Divider is used by TuyaMCU, because TuyaMCU sends always values as integers so we have to divide them before displaying on UI",
	//chandetail:"enum":"ChType_Power",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_Power,
	//chandetail:{"name":"Current_div100",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"For TuyaMCU power metering. Not used for BL09** and CSE** sensors. Divider is used by TuyaMCU, because TuyaMCU sends always values as integers so we have to divide them before displaying on UI",
	//chandetail:"enum":"ChType_Current_div100",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_Current_div100,
	//chandetail:{"name":"ActivePower",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"For TuyaMCU power metering. Not used for BL09** and CSE** sensors. Divider is used by TuyaMCU, because TuyaMCU sends always values as integers so we have to divide them before displaying on UI",
	//chandetail:"enum":"ChType_ActivePower",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_ActivePower,
	//chandetail:{"name":"PowerFactor_div1000",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"For TuyaMCU power metering. Not used for BL09** and CSE** sensors. Divider is used by TuyaMCU, because TuyaMCU sends always values as integers so we have to divide them before displaying on UI",
	//chandetail:"enum":"ChType_PowerFactor_div1000",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_PowerFactor_div1000,
	//chandetail:{"name":"ReactivePower",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"For TuyaMCU power metering. Not used for BL09** and CSE** sensors. Divider is used by TuyaMCU, because TuyaMCU sends always values as integers so we have to divide them before displaying on UI",
	//chandetail:"enum":"ChType_ReactivePower",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_ReactivePower,
	//chandetail:{"name":"EnergyTotal_kWh_div1000",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"For TuyaMCU power metering. Not used for BL09** and CSE** sensors. Divider is used by TuyaMCU, because TuyaMCU sends always values as integers so we have to divide them before displaying on UI",
	//chandetail:"enum":"ChType_EnergyTotal_kWh_div1000",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_EnergyTotal_kWh_div1000,
	//chandetail:{"name":"EnergyExport_kWh_div1000",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"For TuyaMCU power metering. Not used for BL09** and CSE** sensors. Divider is used by TuyaMCU, because TuyaMCU sends always values as integers so we have to divide them before displaying on UI",
	//chandetail:"enum":"ChType_EnergyExport_kWh_div1000",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_EnergyExport_kWh_div1000,
	//chandetail:{"name":"EnergyToday_kWh_div1000",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"For TuyaMCU power metering. Not used for BL09** and CSE** sensors. Divider is used by TuyaMCU, because TuyaMCU sends always values as integers so we have to divide them before displaying on UI",
	//chandetail:"enum":"ChType_EnergyToday_kWh_div1000",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_EnergyToday_kWh_div1000,
	//chandetail:{"name":"Current_div1000",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"For TuyaMCU power metering. Not used for BL09** and CSE** sensors. Divider is used by TuyaMCU, because TuyaMCU sends always values as integers so we have to divide them before displaying on UI",
	//chandetail:"enum":"ChType_Current_div1000",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_Current_div1000,
	//chandetail:{"name":"EnergyTotal_kWh_div100",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"For TuyaMCU power metering. Not used for BL09** and CSE** sensors. Divider is used by TuyaMCU, because TuyaMCU sends always values as integers so we have to divide them before displaying on UI",
	//chandetail:"enum":"ChType_EnergyTotal_kWh_div100",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_EnergyTotal_kWh_div100,
	//chandetail:{"name":"OpenClosed",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"This will show an 'Open' or 'Closed' string on main WWW panel. Useful for door sensors, etc.",
	//chandetail:"enum":"ChType_OpenClosed",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_OpenClosed,
	//chandetail:{"name":"OpenClosed_Inv",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"Like OpenClosed, but values are inversed.",
	//chandetail:"enum":"ChType_OpenClosed_Inv",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_OpenClosed_Inv,
	//chandetail:{"name":"BatteryLevelPercent",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"This will show current value as a battery level percent on the main WWW panel.",
	//chandetail:"enum":"ChType_BatteryLevelPercent",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_BatteryLevelPercent,
	//chandetail:{"name":"OffDimBright",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"A 3 options radio button for lighting control.",
	//chandetail:"enum":"ChType_OffDimBright",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_OffDimBright,
	//chandetail:{"name":"LowMidHighHighest",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"Like LowMidHigh, but with 4 options. Some of TuyaMCU fans might require that.",
	//chandetail:"enum":"ChType_LowMidHighHighest",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_LowMidHighHighest,
	//chandetail:{"name":"OffLowMidHighHighest",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"Like LowMidHigh, but with 5 options. Some of TuyaMCU fans might require that.",
	//chandetail:"enum":"ChType_OffLowMidHighHighest",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_OffLowMidHighHighest,
	//chandetail:{"name":"Custom",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"A custom channel type that is still send to HA.",
	//chandetail:"enum":"ChType_Custom",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_Custom,
	//chandetail:{"name":"Power_div10",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"Just like power, but with one decimal place (but stored as integer, for TuyaMCU support)",
	//chandetail:"enum":"ChType_Power_div10",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_Power_div10,
	//chandetail:{"name":"ReadOnlyLowMidHigh",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"Like LowMidHigh, but just read only",
	//chandetail:"enum":"ChType_ReadOnlyLowMidHigh",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_ReadOnlyLowMidHigh,
	//chandetail:{"name":"SmokePercent",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"Smoke percentage",
	//chandetail:"enum":"ChType_SmokePercent",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_SmokePercent,
	//chandetail:{"name":"Illuminance",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"Illuminance in Lux",
	//chandetail:"enum":"ChType_Illuminance",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_Illuminance,
	//chandetail:{"name":"Toggle_Inv",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"Like a Toggle, but inverted states.",
	//chandetail:"enum":"ChType_Toggle_Inv",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_Toggle_Inv,
	//chandetail:{"name":"OffOnRemember",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"Radio buttons with 3 options: off, on and 'remember'. This is used for TuyaMCU memory state",
	//chandetail:"enum":"ChType_OffOnRemember",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_OffOnRemember,
	//chandetail:{"name":"Voltage_div100",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"For TuyaMCU power metering. Not used for BL09** and CSE** sensors. Divider is used by TuyaMCU, because TuyaMCU sends always values as integers so we have to divide them before displaying on UI",
	//chandetail:"enum":"ChType_Voltage_div100",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_Voltage_div100,
	//chandetail:{"name":"Temperature_div2",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"Just like ChType_Temperature_div10, but for multiplied by 0.5.",
	//chandetail:"enum":"ChType_Temperature_div2",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_Temperature_div2,
	//chandetail:{"name":"TimerSeconds",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"This will display time formatted to minutes, hours, etc.",
	//chandetail:"enum":"ChType_TimerSeconds",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_TimerSeconds,
	//chandetail:{"name":"Frequency_div10",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"For TuyaMCU power metering. Not used for BL09** and CSE** sensors. Divider is used by TuyaMCU, because TuyaMCU sends always values as integers so we have to divide them before displaying on UI",
	//chandetail:"enum":"ChType_Frequency_div10",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_Frequency_div10,
	//chandetail:{"name":"PowerFactor_div100",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"For TuyaMCU power metering. Not used for BL09** and CSE** sensors. Divider is used by TuyaMCU, because TuyaMCU sends always values as integers so we have to divide them before displaying on UI",
	//chandetail:"enum":"ChType_PowerFactor_div100",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_PowerFactor_div100,
	//chandetail:{"name":"Pressure_div100",
	//chandetail:"title":"TODO",
	//chandetail:"descr":".",
	//chandetail:"enum":"Pressure_div100",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_Pressure_div100,
	//chandetail:{"name":"Temperature_div100",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"Just like humidity_div100, but for temperature.",
	//chandetail:"enum":"ChType_Temperature_div100",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_Temperature_div100,
	//chandetail:{"name":"LeakageCurrent_div1000",
	//chandetail:"title":"TODO",
	//chandetail:"descr":".",
	//chandetail:"enum":"ChType_LeakageCurrent_div1000",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_LeakageCurrent_div1000,
	//chandetail:{"name":"Power_div100",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"Just like power, but with one decimal place (but stored as integer, for TuyaMCU support)",
	//chandetail:"enum":"ChType_Power_div100",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_Power_div100,
	//chandetail:{"name":"Motion",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"Motion",
	//chandetail:"enum":"Motion",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_Motion,
	//chandetail:{"name":"ReadOnly_div10",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"This channel is read only.",
	//chandetail:"enum":"ChType_ReadOnly_div10",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_ReadOnly_div10,
	//chandetail:{"name":"ReadOnly_div100",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"This channel is read only.",
	//chandetail:"enum":"ChType_ReadOnly_div100",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_ReadOnly_div100,
	//chandetail:{"name":"ReadOnly_div1000",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"This channel is read only.",
	//chandetail:"enum":"ChType_ReadOnly_div1000",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_ReadOnly_div1000,
	//chandetail:{"name":"Ph",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"Ph Water Quality",
	//chandetail:"enum":"ChType_Ph",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_Ph,
	//chandetail:{"name":"Orp",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"Orp Water Quality",
	//chandetail:"enum":"ChType_Orp",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_Orp,
	//chandetail:{"name":"Tds",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"TDS Water Quality",
	//chandetail:"enum":"ChType_Tds",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_Tds,
	//chandetail:{"name":"Motion_n",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"Motion_n",
	//chandetail:"enum":"Motion_n",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_Motion_n,
	//chandetail:{"name":"Max",
	//chandetail:"title":"TODO",
	//chandetail:"descr":"This is the current total number of available channel types.",
	//chandetail:"enum":"ChType_Max",
	//chandetail:"file":"new_pins.h",
	//chandetail:"driver":""}
	ChType_Max,
} channelType_t;


#if PLATFORM_BL602
#define PLATFORM_GPIO_MAX 24
#elif PLATFORM_XR809
#define PLATFORM_GPIO_MAX 13
#elif PLATFORM_W600
#define PLATFORM_GPIO_MAX 17
#elif PLATFORM_W800
#define PLATFORM_GPIO_MAX 44
#elif PLATFORM_LN882H
#define PLATFORM_GPIO_MAX 26
#elif PLATFORM_ESPIDF
#ifdef CONFIG_IDF_TARGET_ESP32C3
#define PLATFORM_GPIO_MAX 22
#elif CONFIG_IDF_TARGET_ESP32C2
#define PLATFORM_GPIO_MAX 21
#elif CONFIG_IDF_TARGET_ESP32S2
#define PLATFORM_GPIO_MAX 47
#elif CONFIG_IDF_TARGET_ESP32S3
#define PLATFORM_GPIO_MAX 49
#elif CONFIG_IDF_TARGET_ESP32C6
#define PLATFORM_GPIO_MAX 31
#elif CONFIG_IDF_TARGET_ESP32
#define PLATFORM_GPIO_MAX 40
#else
#define PLATFORM_GPIO_MAX 0
#endif
#else
#define PLATFORM_GPIO_MAX 29
#endif

#define CHANNEL_MAX 64

// Special channel indexes
// They were created so we can have easy and seamless
// access to special variables internally.
// Futhermore, they can be very useful for scripting,
// because they can be plugged into "setChannel" command
#define SPECIAL_CHANNEL_BRIGHTNESS		129
#define SPECIAL_CHANNEL_LEDPOWER		130
#define SPECIAL_CHANNEL_BASECOLOR		131
#define SPECIAL_CHANNEL_TEMPERATURE		132
// RGBCW access (well, in reality, we just use RGB access and CW is derived from temp)
#define SPECIAL_CHANNEL_BASECOLOR_FIRST 133
#define SPECIAL_CHANNEL_BASECOLOR_RED	133
#define SPECIAL_CHANNEL_BASECOLOR_GREEN	134
#define SPECIAL_CHANNEL_BASECOLOR_BLUE	135
#define SPECIAL_CHANNEL_BASECOLOR_COOL	136
#define SPECIAL_CHANNEL_BASECOLOR_WARM	137
#define SPECIAL_CHANNEL_BASECOLOR_LAST	137

// note: real limit here is MAX_RETAIN_CHANNELS
#define SPECIAL_CHANNEL_FLASHVARS_FIRST	200
#define SPECIAL_CHANNEL_FLASHVARS_LAST	264


#if PLATFORM_W800

typedef struct pinsState_s {
	// All above values are indexed by physical pin index
	// (so we assume we have maximum of 32 pins)
	byte roles[48];
	byte channels[48];
	// extra channels array - this is needed for
	// buttons, so button can toggle one relay on single click
	// and other relay on double click
	byte channels2[48];
	// This single field above, is indexed by CHANNEL INDEX
	// (not by pin index)
	byte channelTypes[CHANNEL_MAX];
} pinsState_t;

#elif PLATFORM_ESPIDF

typedef struct pinsState_s
{
	// All above values are indexed by physical pin index
	// (so we assume we have maximum of 32 pins)
	byte roles[50];
	byte channels[50];
	// extra channels array - this is needed for
	// buttons, so button can toggle one relay on single click
	// and other relay on double click
	byte channels2[50];
	// This single field above, is indexed by CHANNEL INDEX
	// (not by pin index)
	byte channelTypes[CHANNEL_MAX];
} pinsState_t;

#else

typedef struct pinsState_s {
	// All above values are indexed by physical pin index
	// (so we assume we have maximum of 32 pins)
	byte roles[32];
	byte channels[32];
	// extra channels array - this is needed for
	// buttons, so button can toggle one relay on single click
	// and other relay on double click
	byte channels2[32];
	// This single field above, is indexed by CHANNEL INDEX
	// (not by pin index)
	byte channelTypes[CHANNEL_MAX];
} pinsState_t;

#endif

// bit indexes (not values), so 0 1 2 3 4
#define OBK_FLAG_MQTT_BROADCASTLEDPARAMSTOGETHER	0
#define OBK_FLAG_MQTT_BROADCASTLEDFINALCOLOR		1
#define OBK_FLAG_MQTT_BROADCASTSELFSTATEPERMINUTE	2
#define OBK_FLAG_LED_RAWCHANNELSMODE				3
#define OBK_FLAG_LED_FORCESHOWRGBCWCONTROLLER		4
#define OBK_FLAG_CMD_ENABLETCPRAWPUTTYSERVER		5
#define OBK_FLAG_BTN_INSTANTTOUCH					6
#define OBK_FLAG_MQTT_ALWAYSSETRETAIN				7
#define OBK_FLAG_LED_ALTERNATE_CW_MODE				8
#define OBK_FLAG_SM2135_SEPARATE_MODES				9
#define OBK_FLAG_MQTT_BROADCASTSELFSTATEONCONNECT	10
#define OBK_FLAG_SLOW_PWM							11
#define OBK_FLAG_LED_REMEMBERLASTSTATE				12
#define OBK_FLAG_HTTP_PINMONITOR					13
#define OBK_FLAG_IR_PUBLISH_RECEIVED				14
#define OBK_FLAG_IR_ALLOW_UNKNOWN					15
#define OBK_FLAG_LED_BROADCAST_FULL_RGBCW			16
#define OBK_FLAG_LED_AUTOENABLE_ON_WWW_ACTION		17
#define OBK_FLAG_LED_SMOOTH_TRANSITIONS				18
#define OBK_FLAG_TUYAMCU_ALWAYSPUBLISHCHANNELS		19
#define OBK_FLAG_LED_FORCE_MODE_RGB					20
#define OBK_FLAG_MQTT_RETAIN_POWER_CHANNELS			21
#define OBK_FLAG_IR_PUBLISH_RECEIVED_IN_JSON		22
#define OBK_FLAG_LED_AUTOENABLE_ON_ANY_ACTION		23
#define OBK_FLAG_LED_EMULATE_COOL_WITH_RGB			24
#define OBK_FLAG_POWER_ALLOW_NEGATIVE				25
#define OBK_FLAG_USE_SECONDARY_UART					26
#define OBK_FLAG_AUTOMAIC_HASS_DISCOVERY			27
#define OBK_FLAG_LED_SETTING_WHITE_RGB_ENABLES_CW	28
#define OBK_FLAG_USE_SHORT_DEVICE_NAME_AS_HOSTNAME	29
#define OBK_FLAG_DO_TASMOTA_TELE_PUBLISHES			30
#define OBK_FLAG_CMD_ACCEPT_UART_COMMANDS			31
#define OBK_FLAG_LED_USE_OLD_LINEAR_MODE			32
#define OBK_FLAG_PUBLISH_MULTIPLIED_VALUES			33
#define OBK_FLAG_MQTT_HASS_ADD_RELAYS_AS_LIGHTS		34
#define OBK_FLAG_NOT_PUBLISH_AVAILABILITY			 35
#define OBK_FLAG_DRV_DISABLE_AUTOSTART              36
#define OBK_FLAG_WIFI_FAST_CONNECT		            37
#define OBK_FLAG_POWER_FORCE_ZERO_IF_RELAYS_OPEN    38
#define OBK_FLAG_MQTT_PUBLISH_ALL_CHANNELS			39
#define OBK_FLAG_MQTT_ENERGY_IN_KWH					40
#define OBK_FLAG_BUTTON_DISABLE_ALL					41
#define OBK_FLAG_DOORSENSOR_INVERT_STATE			42
#define OBK_FLAG_TUYAMCU_USE_QUEUE					43
#define OBK_FLAG_HTTP_DISABLE_AUTH_IN_SAFE_MODE		44
#define OBK_FLAG_DISCOVERY_DONT_MERGE_LIGHTS		45
#define OBK_FLAG_TUYAMCU_STORE_RAW_DATA				46
#define OBK_FLAG_TUYAMCU_STORE_ALL_DATA				47
#define OBK_FLAG_POWER_INVERT_AC					48
#define OBK_FLAG_HTTP_NO_ONOFF_WORDS				49
#define OBK_FLAG_MQTT_NEVERAPPENDGET				50

#define OBK_TOTAL_FLAGS 51

#define LOGGER_FLAG_MQTT_DEDUPER					1
#define LOGGER_FLAG_POWER_SAVE						2
#define LOGGER_FLAG_EVERY_SECOND_PRINT				3
#define LOGGER_FLAG_PERIODIC_WIFI_INFO				4

#define LOGGER_TOTAL_FLAGS 8



#define CGF_MQTT_CLIENT_ID_SIZE			64
#define CGF_SHORT_DEVICE_NAME_SIZE		32
#define CGF_DEVICE_NAME_SIZE			64

typedef union cfgPowerMeasurementCal_u {
	float f;
	int i;
} cfgPowerMeasurementCal_t;

// 8 * 4 = 32 bytes
typedef struct cfgPowerMeasurementCalibration_s {
	cfgPowerMeasurementCal_t values[8];
} cfgPowerMeasurementCalibration_t;

// unit is 0.1s
#define CFG_DEFAULT_BTN_SHORT	3
#define CFG_DEFAULT_BTN_LONG	10
#define CFG_DEFAULT_BTN_REPEAT	5

enum {
	CFG_OBK_VOLTAGE = 0,
	CFG_OBK_CURRENT,
	CFG_OBK_POWER,
	CFG_OBK_POWER_MAX
};

typedef struct led_corr_s { // LED gamma correction and calibration data block
	float rgb_cal[3];     // RGB correction factors, range 0.0-1.0, 1.0 = no correction
	float led_gamma;      // LED gamma value, range 1.0-3.0
	float rgb_bright_min; // RGB minimum brightness, range 0.0-10.0%
	float cw_bright_min;  // CW minimum brightness, range 0.0-10.0%
} led_corr_t;

#define MAGIC_LED_CORR_SIZE		24

typedef struct ledRemap_s {
	//unsigned short r : 3;
	//unsigned short g : 3;
	//unsigned short b : 3;
	//unsigned short c : 3;
	//unsigned short w : 3;
	// I want to be able to easily access indices with []
	union {
		struct {
			byte r;
			byte g;
			byte b;
			byte c;
			byte w;
		};
		byte ar[5];
	};
} ledRemap_t;

#define MAGIC_LED_REMAP_SIZE 5


//
// Main config structure (less than 2KB)
//
// This config structure is supposed  to be saved only when user
// changes the device configuration, so really not often.
// We should not worry about flash memory wear in this case.
// The saved-every-reboot values are stored elsewhere
// (i.e. saved channel states, reboot counter?)
typedef struct mainConfig_s {
	byte ident0;
	byte ident1;
	byte ident2;
	byte crc;
	// 0x4
	int version;
	// 0x08
	int genericFlags;
	// 0x0C
	int genericFlags2;
	// 0x10
	unsigned short changeCounter;
	unsigned short otaCounter;
	// 0x14
	// target wifi credentials
	char wifi_ssid[64];
	// 0x54
	char wifi_pass[64];
	// 0x94
	// MQTT information for Home Assistant
	char mqtt_host[256];
	// ofs 0x194
	// note: #define CGF_MQTT_CLIENT_ID_SIZE			64
	char mqtt_clientId[CGF_MQTT_CLIENT_ID_SIZE];
	// ofs 0x1D4
	char mqtt_userName[64];
	// ofs 0x214
	char mqtt_pass[128];
	//mqtt_port at offs 0x00000294 
	int mqtt_port;
	// addon JavaScript panel is hosted on external server
	// at offs 0x298 
	char webappRoot[64];
	// TODO?
	byte mac[6];
	// at ofs: 0x2DE
	// NOTE: NO LONGER 4 byte aligned!!!
	// TODO?
	// #define CGF_SHORT_DEVICE_NAME_SIZE		32
	char shortDeviceName[CGF_SHORT_DEVICE_NAME_SIZE];
	// #define CGF_DEVICE_NAME_SIZE			64
	// at ofs: 0x2FE
	char longDeviceName[CGF_DEVICE_NAME_SIZE];

	//pins at offs 0x0000033E
	// 160 bytes
	pinsState_t pins;
	// startChannelValues at offs 0x000003DE
	// 64 * 2
	short startChannelValues[CHANNEL_MAX];
	// unused_fill at offs 0x0000045E 
	short unused_fill; // correct alignment
	// dgr_sendFlags at offs 0x00000460 
	int dgr_sendFlags;
	// dgr_recvFlags at offs 0x00000464 
	int dgr_recvFlags;
	// dgr_name at offs 0x00000468
	char dgr_name[16];
	// at offs 0x478
	char ntpServer[32];
	// 8 * 4 = 32 bytes
	cfgPowerMeasurementCalibration_t cal;
	// offs 0x000004B8
	// short press 1 means 100 ms short press time
	// So basically unit is 0.1 second
	byte buttonShortPress;
	// offs 0x000004B9
	byte buttonLongPress;
	// offs 0x000004BA
	byte buttonHoldRepeat;
	// offs 0x000004BB
	byte unused_fill1;

	// offset 0x000004BC
	unsigned long LFS_Size; // szie of LFS volume.  it's aligned against the end of OTA
	int loggerFlags;
#if PLATFORM_W800
	byte unusedSectorAB[51];
#elif PLATFORM_ESPIDF
	byte unusedSectorAB[43];
#else    
	byte unusedSectorAB[99];
#endif    
	obkStaticIP_t staticIP;
	ledRemap_t ledRemap;
	led_corr_t led_corr;
	// alternate topic name for receiving MQTT commands
	// offset 0x00000554
	char mqtt_group[64];
	// offs 0x00000594
	byte unused_bytefill[3];
	// offs 0x00000597
	byte timeRequiredToMarkBootSuccessfull;
	//offs 0x00000598
	int ping_interval;
	int ping_seconds;
	// 0x000005A0
	char ping_host[64];
	// ofs 0x000005E0 (dec 1504)
	//char initCommandLine[512];
#if PLATFORM_W600 || PLATFORM_W800
#define ALLOW_SSID2 0
#define ALLOW_WEB_PASSWORD 0
	char initCommandLine[512];
#else
#define ALLOW_SSID2 1
#define ALLOW_WEB_PASSWORD 1
	char initCommandLine[1568];
	// offset 0x00000C00 (3072 decimal)
	char wifi_ssid2[64];
	// offset 0x00000C40 (3136 decimal)
	char wifi_pass2[68];
	// offset 0x00000C84 (3204 decimal)
	char webPassword[33];
	// offset 0x00000CA5 (3237 decimal)
	char unused[347];
#endif
} mainConfig_t; 

// one sector is 4096 so it we still have some expand possibility
#define MAGIC_CONFIG_SIZE_V3		2016
#define MAGIC_CONFIG_SIZE_V4		3584

extern mainConfig_t g_cfg;

extern char g_enable_pins;
extern int g_initialPinStates;

#define CHANNEL_SET_FLAG_FORCE		1
#define CHANNEL_SET_FLAG_SKIP_MQTT	2
#define CHANNEL_SET_FLAG_SILENT		4

void PIN_ticks(void* param);

void PIN_DeepSleep_SetWakeUpEdge(int pin, byte edgeCode);
void PIN_DeepSleep_SetAllWakeUpEdges(byte edgeCode);

void PIN_set_wifi_led(int value);
void PIN_AddCommands(void);
void PINS_BeginDeepSleepWithPinWakeUp(unsigned int wakeUpTime);
void PIN_SetupPins();
void PIN_OnReboot();
void CFG_ClearPins();
int PIN_CountPinsWithRole(int role);
int PIN_CountPinsWithRoleOrRole(int role, int role2);
int PIN_GetPinRoleForPinIndex(int index);
int PIN_GetPinChannelForPinIndex(int index);
int PIN_GetPinChannel2ForPinIndex(int index);
int PIN_FindPinIndexForRole(int role, int defaultIndexToReturnIfNotFound);
const char* PIN_GetPinNameAlias(int index);
void PIN_SetPinRoleForPinIndex(int index, int role);
void PIN_SetPinChannelForPinIndex(int index, int ch);
void PIN_SetPinChannel2ForPinIndex(int index, int ch);
void CHANNEL_Toggle(int ch);
void CHANNEL_DoSpecialToggleAll();
bool CHANNEL_Check(int ch);
void PIN_SetGenericDoubleClickCallback(void (*cb)(int pinIndex));
void CHANNEL_ClearAllChannels();
// CHANNEL_SET_FLAG_*
void CHANNEL_Set(int ch, int iVal, int iFlags);
void CHANNEL_Set_FloatPWM(int ch, float fVal, int iFlags);
void CHANNEL_Add(int ch, int iVal);
void CHANNEL_AddClamped(int ch, int iVal, int min, int max, int bWrapInsteadOfClamp);
int CHANNEL_Get(int ch);
float CHANNEL_GetFinalValue(int channel);
float CHANNEL_GetFloat(int ch);
int CHANNEL_GetRoleForOutputChannel(int ch);
bool CHANNEL_ShouldBePublished(int ch);
bool CHANNEL_IsPowerRelayChannel(int ch);
// See: enum channelType_t
void CHANNEL_SetType(int ch, int type);
int CHANNEL_GetType(int ch);
void CHANNEL_SetFirstChannelByType(int requiredType, int newVal);
// CHANNEL_SET_FLAG_*
void CHANNEL_SetAll(int iVal, int iFlags);
void CHANNEL_SetStateOnly(int iVal);
int CHANNEL_HasChannelPinWithRole(int ch, int iorType);
int CHANNEL_HasChannelPinWithRoleOrRole(int ch, int iorType, int iorType2);
bool CHANNEL_IsInUse(int ch);
void Channel_SaveInFlashIfNeeded(int ch);
int CHANNEL_FindMaxValueForChannel(int ch);
// cmd_channels.c
bool CHANNEL_HasLabel(int ch);
const char* CHANNEL_GetLabel(int ch);
bool CHANNEL_ShouldAddTogglePrefixToUI(int ch);
bool CHANNEL_HasNeverPublishFlag(int ch);
//ledRemap_t *CFG_GetLEDRemap();

void PIN_get_Relay_PWM_Count(int* relayCount, int* pwmCount, int* dInputCount);
int h_isChannelPWM(int tg_ch);
int h_isChannelRelay(int tg_ch);
int h_isChannelDigitalInput(int tg_ch);

const char *ChannelType_GetTitle(int type);
const char *ChannelType_GetUnit(int type);
int ChannelType_GetDivider(int type);
int ChannelType_GetDecimalPlaces(int type);

//int PIN_GetPWMIndexForPinIndex(int pin);

int PIN_ParsePinRoleName(const char* name);
const char* PIN_RoleToString(int role);
// return number of channels used for a role
// taken from code in http_fnc.c
int PIN_IOR_NofChan(int test);

extern const char* g_channelTypeNames[];

#endif
