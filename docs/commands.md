# Commands

| Command        | Arguments          | Description  |
|:------------- |:------------- | -----:|
| SetChannel | [ChannelIndex][ChannelValue] | Sets a raw channel to given value. Relay channels are using 1 and 0 values. PWM channels are within [0,100] range. Do not use this for LED control, because there is a better and more advanced LED driver with dimming and configuration memory (remembers setting after on/off), LED driver commands has 'led_' prefix. |
| ToggleChannel |  | qqqqq0 |
| AddChannel | [ChannelIndex][ValueToAdd][ClampMin][ClampMax] | Adds a given value to the channel. Can be used to change PWM brightness. Clamp min and max arguments are optional. |
| ClampChannel |  | qqqqq0 |
| SetPinRole | [PinRole][RoleIndexOrName] | This allows you to set a pin role, for example a Relay role, or Button, etc. Usually it's easier to do this through WWW panel, so you don't have to use this command. |
| SetPinChannel | [PinRole][ChannelIndex] | This allows you to set a channel linked to pin from console. Usually it's easier to do this through WWW panel, so you don't have to use this command. |
| GetChannel |  | qqqqq0 |
| GetReadings |  | qqqqq0 |
| ShortName |  | qqqqq0 |
| FriendlyName |  | NULL |
| startDeepSleep |  | NULL |
| SetFlag |  | NULL |
| FullBootTime |  | NULL |
| AddEventHandler |  | qqqqq0 |
| AddChangeHandler |  | qqqqq0 |
| listEventHandlers |  | NULL |
| clearAllHandlers |  | qqqqq0 |
| echo |  | qqqe |
| restart |  | Reboots the module |
| clearConfig |  | Clears all config |
| clearAll |  | Clears all things |
| PowerSave |  | Enable power save on N & T |
| simonirtest |  | Simons Special Test |
| if |  |  |
| led_dimmer |  | set output dimmer 0..100 |
| add_dimmer |  | set output dimmer 0..100 |
| led_enableAll |  | qqqq |
| led_basecolor_rgb |  | set PWN color using #RRGGBB |
| led_basecolor_rgbcw |  | set PWN color using #RRGGBB[cw][ww] |
| add_temperature |  | NULL |
| led_temperature |  | set qqqq |
| led_brightnessMult |  | set qqqq |
| led_colorMult |  | set qqqq |
| led_saturation |  | set qqqq |
| led_hue |  | set qqqq |
| led_nextColor |  | set qqqq |
| led_lerpSpeed |  | set qqqq |
| led_expoMode |  | set brightness exponential mode 0..4<br/>e.g.:led_expoMode 4 |
| HSBColor |  | NULL |
| HSBColor1 |  | NULL |
| HSBColor2 |  | NULL |
| HSBColor3 |  | NULL |
| led_finishFullLerp |  | NULL |
| addRepeatingEvent |  | qqqq |
| addRepeatingEventID |  | qqqq |
| cancelRepeatingEvent |  | qqqq |
| clearRepeatingEvents |  | NULL |
| listRepeatingEvents |  | NULL |
| startScript |  | qqqqq0 |
| stopScript |  | qqqqq0 |
| stopAllScripts |  | qqqqq0 |
| listScripts |  | qqqqq0 |
| goto |  | qqqqq0 |
| delay_s |  | qqqqq0 |
| delay_ms |  | qqqqq0 |
| return |  | qqqqq0 |
| resetSVM |  | qqqqq0 |
| sendGet |  | qq |
| power |  | set output POWERn 0..100 |
| powerStateOnly |  | ensures that device is on or off without changing pwm values |
| powerAll |  | set all outputs |
| color |  | set PWN color using #RRGGBB[cw][ww] |
| backlog |  | run a sequence of ; separated commands |
| exec |  | exec <file> - run autoexec.bat or other file from LFS if present |
| lfs_test1 |  |  |
| lfs_test2 |  |  |
| lfs_test3 |  |  |
| SSID1 | NULL | NULL |
| Password1 | NULL | NULL |
| MqttHost | NULL | NULL |
| MqttUser | NULL | NULL |
| MqttPassword | NULL | NULL |
| MqttClient | NULL | NULL |
| aliasMem |  | custom |
| alias |  | add a custom command |
| testMallocFree |  |  |
| testRealloc |  |  |
| testJSON |  |  |
| testLog |  |  |
| testFloats |  |  |
| testArgs |  |  |
| testStrdup |  |  |
| PowerSet |  | Sets current power value for calibration |
| VoltageSet |  | Sets current V value for calibration |
| CurrentSet |  | Sets current I value for calibration |
| PREF |  | Sets the calibration multiplier |
| VREF |  | Sets the calibration multiplier |
| IREF |  | Sets the calibration multiplier |
| PowerMax |  | Sets Maximum power value measurement limiter |
| EnergyCntReset |  | Reset Energy Counter |
| SetupEnergyStats |  | Setup Energy Statistic Parameters: [enable<0|1>] [sample_time<10..900>] [sample_count<10..180>] |
| ConsumptionThresold |  | Setup value for automatic save of consumption data [1..100] |
| BP1658CJ_RGBCW |  | qq |
| BP1658CJ_Map |  | qq |
| BP5758D_RGBCW |  | qq |
| BP5758D_Map |  | qq |
| BP5758D_Current |  | qq |
| setButtonColor |  | NULL |
| setButtonCommand |  | NULL |
| setButtonLabel |  | NULL |
| setButtonEnabled |  | NULL |
| IRSend |  | Sends IR commands in the form PROT-ADDR-CMD-REP, e.g. NEC-1-1A-0 |
| IREnable |  | Enable/disable aspects of IR.  IREnable RXTX 0/1 - enable Rx whilst Tx.  IREnable [protocolname] 0/1 - enable/disable a specified protocol |
| startDriver |  | Starts driver |
| stopDriver |  | Stops driver |
| ntp_timeZoneOfs |  | Sets the time zone offset in hours |
| ntp_setServer |  | Sets the NTP server |
| ntp_info |  | Display NTP related settings |
| toggler_enable |  | NULL |
| toggler_set |  | NULL |
| toggler_channel |  | NULL |
| toggler_name |  | NULL |
| SM16703P_Test |  | qq |
| SM16703P_Send |  | NULL |
| SM16703P_Test_3xZero |  | NULL |
| SM16703P_Test_3xOne |  | NULL |
| SM2135_RGBCW |  | qq |
| SM2135_Map |  | qq |
| SM2135_Current |  | qq |
| obkDeviceList |  | qqq |
| DGR_SendPower |  | qqq |
| DGR_SendBrightness |  | qqq |
| DGR_SendRGBCW |  | qqq |
| DGR_SendFixedColor |  | qqq |
| tuyaMcu_testSendTime |  | Sends a example date by TuyaMCU to clock/callendar MCU |
| tuyaMcu_sendCurTime |  | Sends a current date by TuyaMCU to clock/callendar MCU |
| tuyaMcu_fakeHex |  | qq |
| linkTuyaMCUOutputToChannel |  | Map value send from TuyaMCU (eg. humidity or temperature) to channel |
| tuyaMcu_setDimmerRange |  | Set dimmer range used by TuyaMCU |
| tuyaMcu_sendHeartbeat |  | Send heartbeat to TuyaMCU |
| tuyaMcu_sendQueryState |  | Send query state command |
| tuyaMcu_sendProductInformation |  | Send qqq |
| tuyaMcu_sendState |  | Send set state command |
| tuyaMcu_sendMCUConf |  | Send MCU conf command |
| fakeTuyaPacket |  | qq |
| tuyaMcu_setBaudRate |  | Set the serial baud rate used to communicate with the TuyaMCU |
| tuyaMcu_sendRSSI |  | NULL |
| uartSendHex |  | Sends raw data by TuyaMCU UART, you must write whole packet with checksum yourself |
| uartSendASCII | NULL | NULL |
| UCS1912_Test |  | qq |
| lcd_clearAndGoto |  | Adds a new I2C device |
| lcd_goto |  | Adds a new I2C device |
| lcd_print |  | Adds a new I2C device |
| lcd_clear |  | Adds a new I2C device |
| addI2CDevice_TC74 |  | Adds a new I2C device |
| addI2CDevice_MCP23017 |  | Adds a new I2C device |
| addI2CDevice_LCM1602 |  | Adds a new I2C device |
| addI2CDevice_LCD_PCF8574 |  | Adds a new I2C device |
| MCP23017_MapPinToChannel |  | Adds a new I2C device |
| lfssize | NULL | Log or Set LFS size - will apply and re-format next boot, usage setlfssize 0x10000 |
| lfsunmount | NULL | Un-mount LFS |
| lfsmount | NULL | Mount LFS |
| lfsformat | NULL | Unmount and format LFS.  Optionally add new size as argument |
| loglevel |  | set log level <0..6> |
| logfeature |  | set log feature filter, <0..10> <0|1> |
| logtype |  | logtype direct|all - direct logs only to serial immediately |
| logdelay |  | logdelay 0..n - impose ms delay after every log |
| MQTT_COMMAND_PUBLISH |  | Sqqq |
| MQTT_COMMAND_PUBLISH_ALL |  | Sqqq |
| MQTT_COMMAND_PUBLISH_CHANNELS |  | Sqqq |
| MQTT_COMMAND_PUBLISH_BENCHMARK |  |  |
| showgpi | NULL | log stat of all GPIs |
| setChannelType | NULL | qqqqqqqq |
| showChannelValues | NULL | log channel values |
| setButtonTimes | NULL |  |
| setButtonHoldRepeat | NULL |  |

