# Commands

| Command        | Arguments          | Description  | fn | File |
| ------------- |:-------------:| -----:| ------ | ------ |
| SetChannel |  | qqqqq0 | CMD_SetChannel | cmnds/cmd_channels.c |
| ToggleChannel |  | qqqqq0 | CMD_ToggleChannel | cmnds/cmd_channels.c |
| AddChannel |  | qqqqq0 | CMD_AddChannel | cmnds/cmd_channels.c |
| ClampChannel |  | qqqqq0 | CMD_ClampChannel | cmnds/cmd_channels.c |
| SetPinRole |  | qqqqq0 | CMD_SetPinRole | cmnds/cmd_channels.c |
| SetPinChannel |  | qqqqq0 | CMD_SetPinChannel | cmnds/cmd_channels.c |
| GetChannel |  | qqqqq0 | CMD_GetChannel | cmnds/cmd_channels.c |
| GetReadings |  | qqqqq0 | CMD_GetReadings | cmnds/cmd_channels.c |
| ShortName |  | qqqqq0 | CMD_ShortName | cmnds/cmd_channels.c |
| AddEventHandler |  | qqqqq0 | CMD_AddEventHandler | cmnds/cmd_eventHandlers.c |
| AddChangeHandler |  | qqqqq0 | CMD_AddChangeHandler | cmnds/cmd_eventHandlers.c |
| listEvents |  | qqqqq0 | CMD_ListEvents | cmnds/cmd_eventHandlers.c |
| clearAllHandlers |  | qqqqq0 | CMD_ClearAllHandlers | cmnds/cmd_eventHandlers.c |
| echo |  | qqqe | CMD_Echo | cmnds/cmd_main.c |
| restart |  | Reboots the module | CMD_Restart | cmnds/cmd_main.c |
| clearConfig |  | Clears all config | CMD_ClearConfig | cmnds/cmd_main.c |
| simonirtest |  | Simons Special Test | CMD_SimonTest | cmnds/cmd_main.c |
| if |  |  | CMD_If | cmnds/cmd_main.c |
| led_dimmer |  | set output dimmer 0..100 | dimmer | cmnds/cmd_newLEDDriver.c |
| add_dimmer |  | set output dimmer 0..100 | add_dimmer | cmnds/cmd_newLEDDriver.c |
| led_enableAll |  | qqqq | enableAll | cmnds/cmd_newLEDDriver.c |
| led_basecolor_rgb |  | set PWN color using #RRGGBB | basecolor_rgb | cmnds/cmd_newLEDDriver.c |
| led_basecolor_rgbcw |  | set PWN color using #RRGGBB[cw][ww] | basecolor_rgbcw | cmnds/cmd_newLEDDriver.c |
| led_temperature |  | set qqqq | temperature | cmnds/cmd_newLEDDriver.c |
| led_brightnessMult |  | set qqqq | brightnessMult | cmnds/cmd_newLEDDriver.c |
| led_colorMult |  | set qqqq | colorMult | cmnds/cmd_newLEDDriver.c |
| led_saturation |  | set qqqq | setSaturation | cmnds/cmd_newLEDDriver.c |
| led_hue |  | set qqqq | setHue | cmnds/cmd_newLEDDriver.c |
| led_nextColor |  | set qqqq | nextColor | cmnds/cmd_newLEDDriver.c |
| led_lerpSpeed |  | set qqqq | lerpSpeed | cmnds/cmd_newLEDDriver.c |
| addRepeatingEvent |  | qqqq | RepeatingEvents_Cmd_AddRepeatingEvent | cmnds/cmd_repeatingEvents.c |
| addRepeatingEventID |  | qqqq | RepeatingEvents_Cmd_AddRepeatingEvent | cmnds/cmd_repeatingEvents.c |
| cancelRepeatingEvent |  | qqqq | RepeatingEvents_Cmd_CancelRepeatingEvent | cmnds/cmd_repeatingEvents.c |
| startScript |  | qqqqq0 | CMD_StartScript | cmnds/cmd_script.c |
| stopScript |  | qqqqq0 | CMD_StopScript | cmnds/cmd_script.c |
| stopAllScripts |  | qqqqq0 | CMD_StopAllScripts | cmnds/cmd_script.c |
| listScripts |  | qqqqq0 | CMD_ListScripts | cmnds/cmd_script.c |
| goto |  | qqqqq0 | CMD_GoTo | cmnds/cmd_script.c |
| delay_s |  | qqqqq0 | CMD_Delay_s | cmnds/cmd_script.c |
| delay_ms |  | qqqqq0 | CMD_Delay_ms | cmnds/cmd_script.c |
| return |  | qqqqq0 | CMD_Return | cmnds/cmd_script.c |
| resetSVM |  | qqqqq0 | CMD_resetSVM | cmnds/cmd_script.c |
| sendGet |  | qq | CMD_SendGET | cmnds/cmd_send.c |
| power |  | set output POWERn 0..100 | power | cmnds/cmd_tasmota.c |
| powerStateOnly |  | ensures that device is on or off without changing pwm values | powerStateOnly | cmnds/cmd_tasmota.c |
| powerAll |  | set all outputs | powerAll | cmnds/cmd_tasmota.c |
| color |  | set PWN color using #RRGGBB[cw][ww] | color | cmnds/cmd_tasmota.c |
| backlog |  | run a sequence of ; separated commands | cmnd_backlog | cmnds/cmd_tasmota.c |
| exec |  | exec <file> - run autoexec.bat or other file from LFS if present | cmnd_lfsexec | cmnds/cmd_tasmota.c |
| lfs_test1 |  |  | cmnd_lfs_test1 | cmnds/cmd_tasmota.c |
| lfs_test2 |  |  | cmnd_lfs_test2 | cmnds/cmd_tasmota.c |
| lfs_test3 |  |  | cmnd_lfs_test3 | cmnds/cmd_tasmota.c |
| aliasMem |  | custom | runcmd | cmnds/cmd_test.c |
| alias |  | add a custom command | alias | cmnds/cmd_test.c |
| testMallocFree |  |  | testMallocFree | cmnds/cmd_test.c |
| testRealloc |  |  | testRealloc | cmnds/cmd_test.c |
| testJSON |  |  | testJSON | cmnds/cmd_test.c |
| testLog |  |  | testLog | cmnds/cmd_test.c |
| testFloats |  |  | testFloats | cmnds/cmd_test.c |
| testArgs |  |  | testArgs | cmnds/cmd_test.c |
| testStrdup |  |  | testStrdup | cmnds/cmd_test.c |
| PowerSet |  | Sets current power value for calibration | BL0937_PowerSet | driver/drv_bl0937.c |
| VoltageSet |  | Sets current V value for calibration | BL0937_VoltageSet | driver/drv_bl0937.c |
| CurrentSet |  | Sets current I value for calibration | BL0937_CurrentSet | driver/drv_bl0937.c |
| PREF |  | Sets the calibration multiplier | BL0937_PowerRef | driver/drv_bl0937.c |
| VREF |  | Sets the calibration multiplier | BL0937_VoltageRef | driver/drv_bl0937.c |
| IREF |  | Sets the calibration multiplier | BL0937_CurrentRef | driver/drv_bl0937.c |
| PowerMax |  | Sets Maximum power value measurement limiter | BL0937_PowerMax | driver/drv_bl0937.c |
| EnergyCntReset |  | Reset Energy Counter | BL09XX_ResetEnergyCounter | driver/drv_bl_shared.c |
| SetupEnergyStats |  | Setup Energy Statistic Parameters: [enable<0|1>] [sample_time<10..900>] [sample_count<10..180>] | BL09XX_SetupEnergyStatistic | driver/drv_bl_shared.c |
| ConsumptionThresold |  | Setup value for automatic save of consumption data [1..100] | BL09XX_SetupConsumptionThreshold | driver/drv_bl_shared.c |
| BP1658CJ_RGBCW |  | qq | BP1658CJ_RGBCW | driver/drv_bp1658cj.c |
| BP1658CJ_Map |  | qq | BP1658CJ_Map | driver/drv_bp1658cj.c |
| BP5758D_RGBCW |  | qq | BP5758D_RGBCW | driver/drv_bp5758d.c |
| BP5758D_Map |  | qq | BP5758D_Map | driver/drv_bp5758d.c |
| BP5758D_Current |  | qq | BP5758D_Current | driver/drv_bp5758d.c |
| IRSend |  | Sends IR commands in the form PROT-ADDR-CMD-RE | IR_Send_Cmd | driver/drv_ir.cpp |
| IREnable |  | Enable/disable aspects of IR.  IREnable RXTX 0/1 - enable Rx whilst Tx.  IREnable [protocolname] 0/1 - enable/disable a specified protocol | IR_Enable | driver/drv_ir.cpp |
| startDriver |  | Starts driver | DRV_Start | driver/drv_main.c |
| stopDriver |  | Stops driver | DRV_Stop | driver/drv_main.c |
| ntp_timeZoneOfs |  | Sets the time zone offset in hours | NTP_SetTimeZoneOfs | driver/drv_ntp.c |
| ntp_setServer |  | Sets the NTP server | NTP_SetServer | driver/drv_ntp.c |
| ntp_info |  | Display NTP related settings | NTP_Info | driver/drv_ntp.c |
| SM2135_RGBCW |  | qq | SM2135_RGBCW | driver/drv_sm2135.c |
| SM2135_Map |  | qq | SM2135_Map | driver/drv_sm2135.c |
| SM2135_Current |  | qq | SM2135_Current | driver/drv_sm2135.c |
| obkDeviceList | none | Lists OBK devices<br/>seen sending SSDP NOTIFY | Cmd_obkDeviceList | driver/drv_ssdp.c |
| DGR_SendPower |  | qqq | CMD_DGR_SendPower | driver/drv_tasmotaDeviceGroups.c |
| DGR_SendBrightness |  | qqq | CMD_DGR_SendBrightness | driver/drv_tasmotaDeviceGroups.c |
| DGR_SendRGBCW |  | qqq | CMD_DGR_SendRGBCW | driver/drv_tasmotaDeviceGroups.c |
| tuyaMcu_testSendTime |  | Sends a example date by TuyaMCU to clock/callendar MCU | TuyaMCU_Send_SetTime_Example | driver/drv_tuyaMCU.c |
| tuyaMcu_sendCurTime |  | Sends a current date by TuyaMCU to clock/callendar MCU | TuyaMCU_Send_SetTime_Current | driver/drv_tuyaMCU.c |
| uartSendHex |  | Sends raw data by TuyaMCU UAR | TuyaMCU_Send_Hex | driver/drv_tuyaMCU.c |
| tuyaMcu_fakeHex |  | qq | TuyaMCU_Fake_Hex | driver/drv_tuyaMCU.c |
| linkTuyaMCUOutputToChannel |  | Map value send from TuyaMCU (eg. humidity or temperature) to channel | TuyaMCU_LinkTuyaMCUOutputToChannel | driver/drv_tuyaMCU.c |
| tuyaMcu_setDimmerRange |  | Set dimmer range used by TuyaMCU | TuyaMCU_SetDimmerRange | driver/drv_tuyaMCU.c |
| tuyaMcu_sendHeartbeat |  | Send heartbeat to TuyaMCU | TuyaMCU_SendHeartbeat | driver/drv_tuyaMCU.c |
| tuyaMcu_sendQueryState |  | Send query state command | TuyaMCU_SendQueryState | driver/drv_tuyaMCU.c |
| tuyaMcu_sendProductInformation |  | Send qqq | TuyaMCU_SendQueryProductInformation | driver/drv_tuyaMCU.c |
| tuyaMcu_sendState |  | Send set state command | TuyaMCU_SendStateCmd | driver/drv_tuyaMCU.c |
| tuyaMcu_sendMCUConf |  | Send MCU conf command | TuyaMCU_SendMCUConf | driver/drv_tuyaMCU.c |
| fakeTuyaPacket |  | qq | TuyaMCU_FakePacket | driver/drv_tuyaMCU.c |
| tuyaMcu_setBaudRate |  | Set the serial baud rate used to communicate with the TuyaMCU | TuyaMCU_SetBaudRate | driver/drv_tuyaMCU.c |
| UCS1912_Test |  | qq | UCS1912_Test | driver/drv_ucs1912.c |
| lcd_clearAndGoto |  | Adds a new I2C device | DRV_I2C_LCD_PCF8574_ClearAndGoTo | i2c/drv_i2c_lcd_pcf8574t.c |
| lcd_goto |  | Adds a new I2C device | DRV_I2C_LCD_PCF8574_GoTo | i2c/drv_i2c_lcd_pcf8574t.c |
| lcd_print |  | Adds a new I2C device | DRV_I2C_LCD_PCF8574_Print | i2c/drv_i2c_lcd_pcf8574t.c |
| lcd_clear |  | Adds a new I2C device | DRV_I2C_LCD_PCF8574_Clear | i2c/drv_i2c_lcd_pcf8574t.c |
| addI2CDevice_TC74 |  | Adds a new I2C device | DRV_I2C_AddDevice_TC74 | i2c/drv_i2c_main.c |
| addI2CDevice_MCP23017 |  | Adds a new I2C device | DRV_I2C_AddDevice_MCP23017 | i2c/drv_i2c_main.c |
| addI2CDevice_LCM1602 |  | Adds a new I2C device | DRV_I2C_AddDevice_LCM1602 | i2c/drv_i2c_main.c |
| addI2CDevice_LCD_PCF8574 |  | Adds a new I2C device | DRV_I2C_AddDevice_PCF8574 | i2c/drv_i2c_main.c |
| MCP23017_MapPinToChannel |  | Adds a new I2C device | DRV_I2C_MCP23017_MapPinToChannel | i2c/drv_i2c_main.c |
| lfssize | NULL | Log or Set LFS size - will apply and re-format next boo | CMD_LFS_Size | littlefs/our_lfs.c |
| lfsunmount | NULL | Un-mount LFS | CMD_LFS_Unmount | littlefs/our_lfs.c |
| lfsmount | NULL | Mount LFS | CMD_LFS_Mount | littlefs/our_lfs.c |
| lfsformat | NULL | Unmount and format LFS.  Optionally add new size as argument | CMD_LFS_Format | littlefs/our_lfs.c |
| loglevel |  | set log level <0..6> | log_command | logging/logging.c |
| logfeature |  | set log feature filte | log_command | logging/logging.c |
| logtype |  | logtype direct|all - direct logs only to serial immediately | log_command | logging/logging.c |
| logdelay |  | logdelay 0..n - impose ms delay after every log | log_command | logging/logging.c |
| MQTT_COMMAND_PUBLISH |  | Sqqq | MQTT_PublishCommand | mqtt/new_mqtt.c |
| MQTT_COMMAND_PUBLISH_ALL |  | Sqqq | MQTT_PublishAll | mqtt/new_mqtt.c |
| MQTT_COMMAND_PUBLISH_CHANNELS |  | Sqqq | MQTT_PublishChannels | mqtt/new_mqtt.c |
| MQTT_COMMAND_PUBLISH_BENCHMARK |  |  | MQTT_StartMQTTTestThread | mqtt/new_mqtt.c |
| showgpi | NULL | log stat of all GPIs | showgpi | new_pins.c |
| setChannelType | NULL | qqqqqqqq | CMD_SetChannelType | new_pins.c |
| showChannelValues | NULL | log channel values | CMD_ShowChannelValues | new_pins.c |
| setButtonTimes | NULL |  | CMD_SetButtonTimes | new_pins.c |
| setButtonHoldRepeat | NULL |  | CMD_setButtonHoldRepeat | new_pins.c |

