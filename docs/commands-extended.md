# Commands

| Command        | Arguments          | Description  | Loc |
|:------------- |:-------------:|:----- | ------:|
| SetChannel | [ChannelIndex][ChannelValue] | Sets a raw channel to given value. Relay channels are using 1 and 0 values. PWM channels are within [0,100] range. Do not use this for LED control, because there is a better and more advanced LED driver with dimming and configuration memory (remembers setting after on/off), LED driver commands has 'led_' prefix. | cmnds/cmd_channels.c<br/>CMD_SetChannel |
| ToggleChannel |  | qqqqq0 | cmnds/cmd_channels.c<br/>CMD_ToggleChannel |
| AddChannel | [ChannelIndex][ValueToAdd][ClampMin][ClampMax] | Adds a given value to the channel. Can be used to change PWM brightness. Clamp min and max arguments are optional. | cmnds/cmd_channels.c<br/>CMD_AddChannel |
| ClampChannel |  | qqqqq0 | cmnds/cmd_channels.c<br/>CMD_ClampChannel |
| SetPinRole | [PinRole][RoleIndexOrName] | This allows you to set a pin role, for example a Relay role, or Button, etc. Usually it's easier to do this through WWW panel, so you don't have to use this command. | cmnds/cmd_channels.c<br/>CMD_SetPinRole |
| SetPinChannel | [PinRole][ChannelIndex] | This allows you to set a channel linked to pin from console. Usually it's easier to do this through WWW panel, so you don't have to use this command. | cmnds/cmd_channels.c<br/>CMD_SetPinChannel |
| GetChannel |  | qqqqq0 | cmnds/cmd_channels.c<br/>CMD_GetChannel |
| GetReadings |  | qqqqq0 | cmnds/cmd_channels.c<br/>CMD_GetReadings |
| ShortName |  | qqqqq0 | cmnds/cmd_channels.c<br/>CMD_ShortName |
| FriendlyName |  | NULL | cmnds/cmd_channels.c<br/>CMD_FriendlyName |
| startDeepSleep |  | NULL | cmnds/cmd_channels.c<br/>CMD_StartDeepSleep |
| SetFlag |  | NULL | cmnds/cmd_channels.c<br/>CMD_SetFlag |
| FullBootTime |  | NULL | cmnds/cmd_channels.c<br/>CMD_FullBootTime |
| AddEventHandler |  | qqqqq0 | cmnds/cmd_eventHandlers.c<br/>CMD_AddEventHandler |
| AddChangeHandler |  | qqqqq0 | cmnds/cmd_eventHandlers.c<br/>CMD_AddChangeHandler |
| listEventHandlers |  | NULL | cmnds/cmd_eventHandlers.c<br/>CMD_ListEventHandlers |
| clearAllHandlers |  | qqqqq0 | cmnds/cmd_eventHandlers.c<br/>CMD_ClearAllHandlers |
| echo |  | qqqe | cmnds/cmd_main.c<br/>CMD_Echo |
| restart |  | Reboots the module | cmnds/cmd_main.c<br/>CMD_Restart |
| clearConfig |  | Clears all config | cmnds/cmd_main.c<br/>CMD_ClearConfig |
| clearAll |  | Clears all things | cmnds/cmd_main.c<br/>CMD_ClearAll |
| PowerSave |  | Enable power save on N & T | cmnds/cmd_main.c<br/>CMD_PowerSave |
| simonirtest |  | Simons Special Test | cmnds/cmd_main.c<br/>CMD_SimonTest |
| if |  |  | cmnds/cmd_main.c<br/>CMD_If |
| led_dimmer |  | set output dimmer 0..100 | cmnds/cmd_newLEDDriver.c<br/>dimmer |
| add_dimmer |  | set output dimmer 0..100 | cmnds/cmd_newLEDDriver.c<br/>add_dimmer |
| led_enableAll |  | qqqq | cmnds/cmd_newLEDDriver.c<br/>enableAll |
| led_basecolor_rgb |  | set PWN color using #RRGGBB | cmnds/cmd_newLEDDriver.c<br/>basecolor_rgb |
| led_basecolor_rgbcw |  | set PWN color using #RRGGBB[cw][ww] | cmnds/cmd_newLEDDriver.c<br/>basecolor_rgbcw |
| add_temperature |  | NULL | cmnds/cmd_newLEDDriver.c<br/>add_temperature |
| led_temperature |  | set qqqq | cmnds/cmd_newLEDDriver.c<br/>temperature |
| led_brightnessMult |  | set qqqq | cmnds/cmd_newLEDDriver.c<br/>brightnessMult |
| led_colorMult |  | set qqqq | cmnds/cmd_newLEDDriver.c<br/>colorMult |
| led_saturation |  | set qqqq | cmnds/cmd_newLEDDriver.c<br/>setSaturation |
| led_hue |  | set qqqq | cmnds/cmd_newLEDDriver.c<br/>setHue |
| led_nextColor |  | set qqqq | cmnds/cmd_newLEDDriver.c<br/>nextColor |
| led_lerpSpeed |  | set qqqq | cmnds/cmd_newLEDDriver.c<br/>lerpSpeed |
| led_expoMode |  | set brightness exponential mode 0..4<br/>e.g.:led_expoMode 4 | cmnds/cmd_newLEDDriver.c<br/>exponentialMode |
| HSBColor |  | NULL | cmnds/cmd_newLEDDriver.c<br/>LED_SetBaseColor_HSB |
| HSBColor1 |  | NULL | cmnds/cmd_newLEDDriver.c<br/>setHue |
| HSBColor2 |  | NULL | cmnds/cmd_newLEDDriver.c<br/>setSaturation |
| HSBColor3 |  | NULL | cmnds/cmd_newLEDDriver.c<br/>setBrightness |
| led_finishFullLerp |  | NULL | cmnds/cmd_newLEDDriver.c<br/>led_finishFullLerp |
| addRepeatingEvent |  | qqqq | cmnds/cmd_repeatingEvents.c<br/>RepeatingEvents_Cmd_AddRepeatingEvent |
| addRepeatingEventID |  | qqqq | cmnds/cmd_repeatingEvents.c<br/>RepeatingEvents_Cmd_AddRepeatingEvent |
| cancelRepeatingEvent |  | qqqq | cmnds/cmd_repeatingEvents.c<br/>RepeatingEvents_Cmd_CancelRepeatingEvent |
| clearRepeatingEvents |  | NULL | cmnds/cmd_repeatingEvents.c<br/>RepeatingEvents_Cmd_ClearRepeatingEvents |
| listRepeatingEvents |  | NULL | cmnds/cmd_repeatingEvents.c<br/>RepeatingEvents_Cmd_ListRepeatingEvents |
| startScript |  | qqqqq0 | cmnds/cmd_script.c<br/>CMD_StartScript |
| stopScript |  | qqqqq0 | cmnds/cmd_script.c<br/>CMD_StopScript |
| stopAllScripts |  | qqqqq0 | cmnds/cmd_script.c<br/>CMD_StopAllScripts |
| listScripts |  | qqqqq0 | cmnds/cmd_script.c<br/>CMD_ListScripts |
| goto |  | qqqqq0 | cmnds/cmd_script.c<br/>CMD_GoTo |
| delay_s |  | qqqqq0 | cmnds/cmd_script.c<br/>CMD_Delay_s |
| delay_ms |  | qqqqq0 | cmnds/cmd_script.c<br/>CMD_Delay_ms |
| return |  | qqqqq0 | cmnds/cmd_script.c<br/>CMD_Return |
| resetSVM |  | qqqqq0 | cmnds/cmd_script.c<br/>CMD_resetSVM |
| sendGet |  | qq | cmnds/cmd_send.c<br/>CMD_SendGET |
| power |  | set output POWERn 0..100 | cmnds/cmd_tasmota.c<br/>power |
| powerStateOnly |  | ensures that device is on or off without changing pwm values | cmnds/cmd_tasmota.c<br/>powerStateOnly |
| powerAll |  | set all outputs | cmnds/cmd_tasmota.c<br/>powerAll |
| color |  | set PWN color using #RRGGBB[cw][ww] | cmnds/cmd_tasmota.c<br/>color |
| backlog |  | run a sequence of ; separated commands | cmnds/cmd_tasmota.c<br/>cmnd_backlog |
| exec |  | exec <file> - run autoexec.bat or other file from LFS if present | cmnds/cmd_tasmota.c<br/>cmnd_lfsexec |
| lfs_test1 |  |  | cmnds/cmd_tasmota.c<br/>cmnd_lfs_test1 |
| lfs_test2 |  |  | cmnds/cmd_tasmota.c<br/>cmnd_lfs_test2 |
| lfs_test3 |  |  | cmnds/cmd_tasmota.c<br/>cmnd_lfs_test3 |
| SSID1 | NULL | NULL | cmnds/cmd_tasmota.c<br/>cmnd_SSID1 |
| Password1 | NULL | NULL | cmnds/cmd_tasmota.c<br/>cmnd_Password1 |
| MqttHost | NULL | NULL | cmnds/cmd_tasmota.c<br/>cmnd_MqttHost |
| MqttUser | NULL | NULL | cmnds/cmd_tasmota.c<br/>cmnd_MqttUser |
| MqttPassword | NULL | NULL | cmnds/cmd_tasmota.c<br/>cmnd_MqttPassword |
| MqttClient | NULL | NULL | cmnds/cmd_tasmota.c<br/>cmnd_MqttClient |
| aliasMem |  | custom | cmnds/cmd_test.c<br/>runcmd |
| alias |  | add a custom command | cmnds/cmd_test.c<br/>alias |
| testMallocFree |  |  | cmnds/cmd_test.c<br/>testMallocFree |
| testRealloc |  |  | cmnds/cmd_test.c<br/>testRealloc |
| testJSON |  |  | cmnds/cmd_test.c<br/>testJSON |
| testLog |  |  | cmnds/cmd_test.c<br/>testLog |
| testFloats |  |  | cmnds/cmd_test.c<br/>testFloats |
| testArgs |  |  | cmnds/cmd_test.c<br/>testArgs |
| testStrdup |  |  | cmnds/cmd_test.c<br/>testStrdup |
| PowerSet |  | Sets current power value for calibration | driver/drv_bl0937.c<br/>BL0937_PowerSet |
| VoltageSet |  | Sets current V value for calibration | driver/drv_bl0937.c<br/>BL0937_VoltageSet |
| CurrentSet |  | Sets current I value for calibration | driver/drv_bl0937.c<br/>BL0937_CurrentSet |
| PREF |  | Sets the calibration multiplier | driver/drv_bl0937.c<br/>BL0937_PowerRef |
| VREF |  | Sets the calibration multiplier | driver/drv_bl0937.c<br/>BL0937_VoltageRef |
| IREF |  | Sets the calibration multiplier | driver/drv_bl0937.c<br/>BL0937_CurrentRef |
| PowerMax |  | Sets Maximum power value measurement limiter | driver/drv_bl0937.c<br/>BL0937_PowerMax |
| EnergyCntReset |  | Reset Energy Counter | driver/drv_bl_shared.c<br/>BL09XX_ResetEnergyCounter |
| SetupEnergyStats |  | Setup Energy Statistic Parameters: [enable<0|1>] [sample_time<10..900>] [sample_count<10..180>] | driver/drv_bl_shared.c<br/>BL09XX_SetupEnergyStatistic |
| ConsumptionThresold |  | Setup value for automatic save of consumption data [1..100] | driver/drv_bl_shared.c<br/>BL09XX_SetupConsumptionThreshold |
| BP1658CJ_RGBCW |  | qq | driver/drv_bp1658cj.c<br/>BP1658CJ_RGBCW |
| BP1658CJ_Map |  | qq | driver/drv_bp1658cj.c<br/>BP1658CJ_Map |
| BP5758D_RGBCW |  | qq | driver/drv_bp5758d.c<br/>BP5758D_RGBCW |
| BP5758D_Map |  | qq | driver/drv_bp5758d.c<br/>BP5758D_Map |
| BP5758D_Current |  | qq | driver/drv_bp5758d.c<br/>BP5758D_Current |
| setButtonColor |  | NULL | driver/drv_httpButtons.c<br/>CMD_setButtonColor |
| setButtonCommand |  | NULL | driver/drv_httpButtons.c<br/>CMD_setButtonCommand |
| setButtonLabel |  | NULL | driver/drv_httpButtons.c<br/>CMD_setButtonLabel |
| setButtonEnabled |  | NULL | driver/drv_httpButtons.c<br/>CMD_setButtonEnabled |
| IRSend |  | Sends IR commands in the form PROT-ADDR-CMD-REP, e.g. NEC-1-1A-0 | driver/drv_ir.cpp<br/>IR_Send_Cmd |
| IREnable |  | Enable/disable aspects of IR.  IREnable RXTX 0/1 - enable Rx whilst Tx.  IREnable [protocolname] 0/1 - enable/disable a specified protocol | driver/drv_ir.cpp<br/>IR_Enable |
| startDriver |  | Starts driver | driver/drv_main.c<br/>DRV_Start |
| stopDriver |  | Stops driver | driver/drv_main.c<br/>DRV_Stop |
| ntp_timeZoneOfs |  | Sets the time zone offset in hours | driver/drv_ntp.c<br/>NTP_SetTimeZoneOfs |
| ntp_setServer |  | Sets the NTP server | driver/drv_ntp.c<br/>NTP_SetServer |
| ntp_info |  | Display NTP related settings | driver/drv_ntp.c<br/>NTP_Info |
| toggler_enable |  | NULL | driver/drv_pwmToggler.c<br/>Toggler_EnableX |
| toggler_set |  | NULL | driver/drv_pwmToggler.c<br/>Toggler_SetX |
| toggler_channel |  | NULL | driver/drv_pwmToggler.c<br/>Toggler_ChannelX |
| toggler_name |  | NULL | driver/drv_pwmToggler.c<br/>Toggler_NameX |
| SM16703P_Test |  | qq | driver/drv_ucs1912.c<br/>SM16703P_Test |
| SM16703P_Send |  | NULL | driver/drv_sm16703P.c<br/>SM16703P_Send_Cmd |
| SM16703P_Test_3xZero |  | NULL | driver/drv_sm16703P.c<br/>SM16703P_Test_3xZero |
| SM16703P_Test_3xOne |  | NULL | driver/drv_sm16703P.c<br/>SM16703P_Test_3xOne |
| SM2135_RGBCW |  | qq | driver/drv_sm2135.c<br/>SM2135_RGBCW |
| SM2135_Map |  | qq | driver/drv_sm2135.c<br/>SM2135_Map |
| SM2135_Current |  | qq | driver/drv_sm2135.c<br/>SM2135_Current |
| obkDeviceList |  | qqq | driver/drv_ssdp.c<br/>Cmd_obkDeviceList |
| DGR_SendPower |  | qqq | driver/drv_tasmotaDeviceGroups.c<br/>CMD_DGR_SendPower |
| DGR_SendBrightness |  | qqq | driver/drv_tasmotaDeviceGroups.c<br/>CMD_DGR_SendBrightness |
| DGR_SendRGBCW |  | qqq | driver/drv_tasmotaDeviceGroups.c<br/>CMD_DGR_SendRGBCW |
| DGR_SendFixedColor |  | qqq | driver/drv_tasmotaDeviceGroups.c<br/>CMD_DGR_SendFixedColor |
| tuyaMcu_testSendTime |  | Sends a example date by TuyaMCU to clock/callendar MCU | driver/drv_tuyaMCU.c<br/>TuyaMCU_Send_SetTime_Example |
| tuyaMcu_sendCurTime |  | Sends a current date by TuyaMCU to clock/callendar MCU | driver/drv_tuyaMCU.c<br/>TuyaMCU_Send_SetTime_Current |
| tuyaMcu_fakeHex |  | qq | driver/drv_tuyaMCU.c<br/>TuyaMCU_Fake_Hex |
| linkTuyaMCUOutputToChannel |  | Map value send from TuyaMCU (eg. humidity or temperature) to channel | driver/drv_tuyaMCU.c<br/>TuyaMCU_LinkTuyaMCUOutputToChannel |
| tuyaMcu_setDimmerRange |  | Set dimmer range used by TuyaMCU | driver/drv_tuyaMCU.c<br/>TuyaMCU_SetDimmerRange |
| tuyaMcu_sendHeartbeat |  | Send heartbeat to TuyaMCU | driver/drv_tuyaMCU.c<br/>TuyaMCU_SendHeartbeat |
| tuyaMcu_sendQueryState |  | Send query state command | driver/drv_tuyaMCU.c<br/>TuyaMCU_SendQueryState |
| tuyaMcu_sendProductInformation |  | Send qqq | driver/drv_tuyaMCU.c<br/>TuyaMCU_SendQueryProductInformation |
| tuyaMcu_sendState |  | Send set state command | driver/drv_tuyaMCU.c<br/>TuyaMCU_SendStateCmd |
| tuyaMcu_sendMCUConf |  | Send MCU conf command | driver/drv_tuyaMCU.c<br/>TuyaMCU_SendMCUConf |
| fakeTuyaPacket |  | qq | driver/drv_tuyaMCU.c<br/>TuyaMCU_FakePacket |
| tuyaMcu_setBaudRate |  | Set the serial baud rate used to communicate with the TuyaMCU | driver/drv_tuyaMCU.c<br/>TuyaMCU_SetBaudRate |
| tuyaMcu_sendRSSI |  | NULL | driver/drv_tuyaMCU.c<br/>Cmd_TuyaMCU_Send_RSSI |
| uartSendHex |  | Sends raw data by TuyaMCU UART, you must write whole packet with checksum yourself | driver/drv_tuyaMCU.c<br/>CMD_UART_Send_Hex |
| uartSendASCII | NULL | NULL | driver/drv_uart.c<br/>CMD_UART_Send_ASCII |
| UCS1912_Test |  | qq | driver/drv_ucs1912.c<br/>UCS1912_Test |
| lcd_clearAndGoto |  | Adds a new I2C device | i2c/drv_i2c_lcd_pcf8574t.c<br/>DRV_I2C_LCD_PCF8574_ClearAndGoTo |
| lcd_goto |  | Adds a new I2C device | i2c/drv_i2c_lcd_pcf8574t.c<br/>DRV_I2C_LCD_PCF8574_GoTo |
| lcd_print |  | Adds a new I2C device | i2c/drv_i2c_lcd_pcf8574t.c<br/>DRV_I2C_LCD_PCF8574_Print |
| lcd_clear |  | Adds a new I2C device | i2c/drv_i2c_lcd_pcf8574t.c<br/>DRV_I2C_LCD_PCF8574_Clear |
| addI2CDevice_TC74 |  | Adds a new I2C device | i2c/drv_i2c_main.c<br/>DRV_I2C_AddDevice_TC74 |
| addI2CDevice_MCP23017 |  | Adds a new I2C device | i2c/drv_i2c_main.c<br/>DRV_I2C_AddDevice_MCP23017 |
| addI2CDevice_LCM1602 |  | Adds a new I2C device | i2c/drv_i2c_main.c<br/>DRV_I2C_AddDevice_LCM1602 |
| addI2CDevice_LCD_PCF8574 |  | Adds a new I2C device | i2c/drv_i2c_main.c<br/>DRV_I2C_AddDevice_PCF8574 |
| MCP23017_MapPinToChannel |  | Adds a new I2C device | i2c/drv_i2c_main.c<br/>DRV_I2C_MCP23017_MapPinToChannel |
| lfssize | NULL | Log or Set LFS size - will apply and re-format next boot, usage setlfssize 0x10000 | littlefs/our_lfs.c<br/>CMD_LFS_Size |
| lfsunmount | NULL | Un-mount LFS | littlefs/our_lfs.c<br/>CMD_LFS_Unmount |
| lfsmount | NULL | Mount LFS | littlefs/our_lfs.c<br/>CMD_LFS_Mount |
| lfsformat | NULL | Unmount and format LFS.  Optionally add new size as argument | littlefs/our_lfs.c<br/>CMD_LFS_Format |
| loglevel |  | set log level <0..6> | logging/logging.c<br/>log_command |
| logfeature |  | set log feature filter, <0..10> <0|1> | logging/logging.c<br/>log_command |
| logtype |  | logtype direct|all - direct logs only to serial immediately | logging/logging.c<br/>log_command |
| logdelay |  | logdelay 0..n - impose ms delay after every log | logging/logging.c<br/>log_command |
| MQTT_COMMAND_PUBLISH |  | Sqqq | mqtt/new_mqtt.c<br/>MQTT_PublishCommand |
| MQTT_COMMAND_PUBLISH_ALL |  | Sqqq | mqtt/new_mqtt.c<br/>MQTT_PublishAll |
| MQTT_COMMAND_PUBLISH_CHANNELS |  | Sqqq | mqtt/new_mqtt.c<br/>MQTT_PublishChannels |
| MQTT_COMMAND_PUBLISH_BENCHMARK |  |  | mqtt/new_mqtt.c<br/>MQTT_StartMQTTTestThread |
| showgpi | NULL | log stat of all GPIs | new_pins.c<br/>showgpi |
| setChannelType | NULL | qqqqqqqq | new_pins.c<br/>CMD_SetChannelType |
| showChannelValues | NULL | log channel values | new_pins.c<br/>CMD_ShowChannelValues |
| setButtonTimes | NULL |  | new_pins.c<br/>CMD_SetButtonTimes |
| setButtonHoldRepeat | NULL |  | new_pins.c<br/>CMD_setButtonHoldRepeat |

