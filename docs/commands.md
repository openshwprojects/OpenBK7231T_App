# Commands

| Command        | Arguments          | Description  |
|:------------- |:------------- | -----:|
| SetChannel |  | qqqqq0 |
| ToggleChannel |  | qqqqq0 |
| AddChannel |  | qqqqq0 |
| ClampChannel |  | qqqqq0 |
| SetPinRole |  | qqqqq0 |
| SetPinChannel |  | qqqqq0 |
| GetChannel |  | qqqqq0 |
| GetReadings |  | qqqqq0 |
| ShortName |  | qqqqq0 |
| AddEventHandler |  | qqqqq0 |
| AddChangeHandler |  | qqqqq0 |
| listEvents |  | qqqqq0 |
| clearAllHandlers |  | qqqqq0 |
| echo | [string to echo] | Echo something to logs and serial<br/>e.g.:echo hello |
| restart | [delay in seconds, default 3] | Reboots the module<br/>e.g.:restart 5 |
| clearConfig | none | Clears all config, and restarts in AP mode |
| simonirtest | depends? | Simons Special Test<br/>e.g.:don't do it |
| if |  |  |
| led_dimmer |  | set output dimmer 0..100 |
| add_dimmer |  | set output dimmer 0..100 |
| led_enableAll |  | qqqq |
| led_basecolor_rgb |  | set PWN color using #RRGGBB |
| led_basecolor_rgbcw |  | set PWN color using #RRGGBB[cw][ww] |
| led_temperature |  | set qqqq |
| led_brightnessMult |  | set qqqq |
| led_colorMult |  | set qqqq |
| led_saturation |  | set qqqq |
| led_hue |  | set qqqq |
| led_nextColor |  | set qqqq |
| led_lerpSpeed |  | set qqqq |
| addRepeatingEvent |  | qqqq |
| addRepeatingEventID |  | qqqq |
| cancelRepeatingEvent |  | qqqq |
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
| IRSend |  | Sends IR commands in the form PROT-ADDR-CMD-RE<br/>e.g.:IRSend NEC-1-1A-0 |
| IREnable |  | Enable/disable aspects of IR.  IREnable RXTX 0/1 - enable Rx whilst Tx.  IREnable [protocolname] 0/1 - enable/disable a specified protocol |
| startDriver |  | Starts driver |
| stopDriver |  | Stops driver |
| ntp_timeZoneOfs |  | Sets the time zone offset in hours |
| ntp_setServer |  | Sets the NTP server |
| ntp_info |  | Display NTP related settings |
| SM2135_RGBCW |  | qq |
| SM2135_Map |  | qq |
| SM2135_Current |  | qq |
| obkDeviceList |  | qqq |
| DGR_SendPower |  | qqq |
| DGR_SendBrightness |  | qqq |
| DGR_SendRGBCW |  | qqq |
| tuyaMcu_testSendTime |  | Sends a example date by TuyaMCU to clock/callendar MCU |
| tuyaMcu_sendCurTime |  | Sends a current date by TuyaMCU to clock/callendar MCU |
| uartSendHex |  | Sends raw data by TuyaMCU UART, you must write whole packet with checksum yourself |
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
| lfssize | NULL | Log or Set LFS size - will apply and re-format next boot<br/>e.g.:lfssize 0x10000 |
| lfsunmount | NULL | Un-mount LFS<br/>e.g.:lfsunmount |
| lfsmount | NULL | Mount LFS<br/>e.g.:lfsmount |
| lfsformat | NULL | Unmount and format LFS.  Optionally add new size as argument<br/>e.g.:lfsformat | lfsformat 0x18000 |
| loglevel |  | set log level <0..6> |
| logfeature |  | set log feature filter, <0..10> <0|1> |
| logtype |  | logtype direct|all - direct logs only to serial immediately |
| logdelay |  | logdelay -1|0..n - impose ms delay after every log, -1 to delay be character count<br/>e.g.:logdelay 200 |
| MQTT_COMMAND_PUBLISH |  | Sqqq |
| MQTT_COMMAND_PUBLISH_ALL |  | Sqqq |
| MQTT_COMMAND_PUBLISH_CHANNELS |  | Sqqq |
| MQTT_COMMAND_PUBLISH_BENCHMARK |  |  |
| showgpi | NULL | log stat of all GPIs |
| setChannelType | NULL | qqqqqqqq |
| showChannelValues | NULL | log channel values |
| setButtonTimes | NULL |  |
| setButtonHoldRepeat | NULL |  |

