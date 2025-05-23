OBK_CFLAGS =
ifdef OBK_VARIANT
OBK_CFLAGS += -DOBK_VARIANT=${OBK_VARIANT}
endif
ifdef USER_SW_VER
OBK_CFLAGS += -DUSER_SW_VER='"${USER_SW_VER}"'
else 
ifdef APP_VERSION
OBK_CFLAGS += -DUSER_SW_VER='"${APP_VERSION}"'
endif
endif

OBKM_SRC =
OBKM_SRC_CXX =

OBKM_SRC  += $(OBK_SRCS)user_main.c

OBKM_SRC  += $(OBK_SRCS)base64/base64.c
OBKM_SRC  += $(OBK_SRCS)bitmessage/bitmessage_read.c
OBKM_SRC  += $(OBK_SRCS)bitmessage/bitmessage_write.c
OBKM_SRC  += $(OBK_SRCS)cJSON/cJSON.c
OBKM_SRC  += $(OBK_SRCS)cmnds/cmd_berry.c
OBKM_SRC  += $(OBK_SRCS)cmnds/cmd_channels.c
OBKM_SRC  += $(OBK_SRCS)cmnds/cmd_eventHandlers.c
OBKM_SRC  += $(OBK_SRCS)cmnds/cmd_if.c
OBKM_SRC  += $(OBK_SRCS)cmnds/cmd_main.c
OBKM_SRC  += $(OBK_SRCS)cmnds/cmd_newLEDDriver_colors.c
OBKM_SRC  += $(OBK_SRCS)cmnds/cmd_newLEDDriver.c
OBKM_SRC  += $(OBK_SRCS)cmnds/cmd_repeatingEvents.c
OBKM_SRC  += $(OBK_SRCS)cmnds/cmd_send.c
OBKM_SRC  += $(OBK_SRCS)cmnds/cmd_script.c
OBKM_SRC  += $(OBK_SRCS)cmnds/cmd_simulatorOnly.c
OBKM_SRC  += $(OBK_SRCS)cmnds/cmd_tasmota.c
OBKM_SRC  += $(OBK_SRCS)cmnds/cmd_tcp.c
OBKM_SRC  += $(OBK_SRCS)cmnds/cmd_test.c
OBKM_SRC  += $(OBK_SRCS)cmnds/cmd_tokenizer.c
OBKM_SRC  += $(OBK_SRCS)devicegroups/deviceGroups_read.c
OBKM_SRC  += $(OBK_SRCS)devicegroups/deviceGroups_util.c
OBKM_SRC  += $(OBK_SRCS)devicegroups/deviceGroups_write.c
OBKM_SRC  += $(OBK_SRCS)hal/generic/hal_adc_generic.c
OBKM_SRC  += $(OBK_SRCS)hal/generic/hal_flashConfig_generic.c
OBKM_SRC  += $(OBK_SRCS)hal/generic/hal_flashVars_generic.c
OBKM_SRC  += $(OBK_SRCS)hal/generic/hal_generic.c
OBKM_SRC  += $(OBK_SRCS)hal/generic/hal_main_generic.c
OBKM_SRC  += $(OBK_SRCS)hal/generic/hal_pins_generic.c
OBKM_SRC  += $(OBK_SRCS)hal/generic/hal_wifi_generic.c
OBKM_SRC  += $(OBK_SRCS)hal/generic/hal_uart_generic.c
OBKM_SRC  += $(OBK_SRCS)httpserver/hass.c
OBKM_SRC  += $(OBK_SRCS)httpserver/http_basic_auth.c
OBKM_SRC  += $(OBK_SRCS)httpserver/http_fns.c
OBKM_SRC  += $(OBK_SRCS)httpserver/http_tcp_server.c
OBKM_SRC  += $(OBK_SRCS)httpserver/new_tcp_server.c
OBKM_SRC  += $(OBK_SRCS)httpserver/json_interface.c
OBKM_SRC  += $(OBK_SRCS)httpserver/new_http.c
OBKM_SRC  += $(OBK_SRCS)httpserver/rest_interface.c
OBKM_SRC  += $(OBK_SRCS)mqtt/new_mqtt_deduper.c
OBKM_SRC  += $(OBK_SRCS)jsmn/jsmn.c
OBKM_SRC  += $(OBK_SRCS)logging/logging.c
OBKM_SRC  += $(OBK_SRCS)mqtt/new_mqtt.c
OBKM_SRC  += $(OBK_SRCS)new_cfg.c
OBKM_SRC  += $(OBK_SRCS)new_common.c
OBKM_SRC  += $(OBK_SRCS)new_ping.c
OBKM_SRC  += $(OBK_SRCS)new_pins.c
OBKM_SRC  += $(OBK_SRCS)rgb2hsv.c
OBKM_SRC  += $(OBK_SRCS)tiny_crc8.c
OBKM_SRC  += $(OBK_SRCS)httpclient/http_client.c
OBKM_SRC  += $(OBK_SRCS)httpclient/utils_net.c
OBKM_SRC  += $(OBK_SRCS)httpclient/utils_timer.c
OBKM_SRC  += $(OBK_SRCS)littlefs/lfs_util.c
OBKM_SRC  += $(OBK_SRCS)littlefs/lfs.c
OBKM_SRC  += $(OBK_SRCS)littlefs/our_lfs.c
OBKM_SRC  += $(OBK_SRCS)ota/ota.c

OBKM_SRC  += $(OBK_SRCS)driver/drv_main.c

OBKM_SRC  += $(OBK_SRCS)driver/drv_adcButton.c
OBKM_SRC  += $(OBK_SRCS)driver/drv_adcSmoother.c
OBKM_SRC  += $(OBK_SRCS)driver/drv_aht2x.c
OBKM_SRC  += $(OBK_SRCS)driver/drv_battery.c
OBKM_SRC  += $(OBK_SRCS)driver/drv_bl0937.c
OBKM_SRC  += $(OBK_SRCS)driver/drv_bl0942.c
OBKM_SRC  += $(OBK_SRCS)driver/drv_bl_shared.c
#OBKM_SRC += $(OBK_SRCS)driver/drv_bmp280.c
OBKM_SRC  += $(OBK_SRCS)driver/drv_bmpi2c.c
OBKM_SRC  += $(OBK_SRCS)driver/drv_bp1658cj.c
OBKM_SRC  += $(OBK_SRCS)driver/drv_bp5758d.c
OBKM_SRC  += $(OBK_SRCS)driver/drv_bridge_driver.c
OBKM_SRC  += $(OBK_SRCS)driver/drv_chargingLimit.c
OBKM_SRC  += $(OBK_SRCS)driver/drv_charts.c
OBKM_SRC  += $(OBK_SRCS)driver/drv_cht8305.c
OBKM_SRC  += $(OBK_SRCS)driver/drv_cse7761.c
OBKM_SRC  += $(OBK_SRCS)driver/drv_cse7766.c
OBKM_SRC  += $(OBK_SRCS)driver/drv_ddp.c
OBKM_SRC  += $(OBK_SRCS)driver/drv_debouncer.c
OBKM_SRC  += $(OBK_SRCS)driver/drv_dht_internal.c
OBKM_SRC  += $(OBK_SRCS)driver/drv_dht.c
OBKM_SRC  += $(OBK_SRCS)driver/drv_drawers.c
OBKM_SRC  += $(OBK_SRCS)driver/drv_doorSensorWithDeepSleep.c
OBKM_SRC  += $(OBK_SRCS)driver/drv_ds1820_simple.c
OBKM_SRC  += $(OBK_SRCS)driver/drv_freeze.c
OBKM_SRC  += $(OBK_SRCS)driver/drv_gn6932.c
OBKM_SRC  += $(OBK_SRCS)driver/drv_hd2015.c
OBKM_SRC  += $(OBK_SRCS)driver/drv_hgs02.c
OBKM_SRC  += $(OBK_SRCS)driver/drv_ht16k33.c
OBKM_SRC  += $(OBK_SRCS)driver/drv_httpButtons.c
OBKM_SRC  += $(OBK_SRCS)driver/drv_hue.c
OBKM_SRC  += $(OBK_SRCS)driver/drv_ir2.c
OBKM_SRC  += $(OBK_SRCS)driver/drv_kp18058.c
OBKM_SRC  += $(OBK_SRCS)driver/drv_kp18068.c
OBKM_SRC  += $(OBK_SRCS)driver/drv_max6675.c
OBKM_SRC  += $(OBK_SRCS)driver/drv_max72xx_clock.c
OBKM_SRC  += $(OBK_SRCS)driver/drv_max72xx_internal.c
OBKM_SRC  += $(OBK_SRCS)driver/drv_max72xx_single.c
OBKM_SRC  += $(OBK_SRCS)driver/drv_mcp9808.c
OBKM_SRC  += $(OBK_SRCS)driver/drv_ntp.c
OBKM_SRC  += $(OBK_SRCS)driver/drv_ntp_events.c
OBKM_SRC  += $(OBK_SRCS)driver/drv_openWeatherMap.c
OBKM_SRC  += $(OBK_SRCS)driver/drv_pir.c
OBKM_SRC  += $(OBK_SRCS)driver/drv_pixelAnim.c
OBKM_SRC  += $(OBK_SRCS)driver/drv_pt6523.c
OBKM_SRC  += $(OBK_SRCS)driver/drv_pwm_groups.c
OBKM_SRC  += $(OBK_SRCS)driver/drv_pwmToggler.c
OBKM_SRC  += $(OBK_SRCS)driver/drv_pwrCal.c
OBKM_SRC  += $(OBK_SRCS)driver/drv_rn8209.c
OBKM_SRC  += $(OBK_SRCS)driver/drv_sgp.c
OBKM_SRC  += $(OBK_SRCS)driver/drv_shiftRegister.c
OBKM_SRC  += $(OBK_SRCS)driver/drv_sht3x.c
#OBKM_SRC += $(OBK_SRCS)driver/drv_sm15155e.c
#OBKM_SRC += $(OBK_SRCS)driver/drv_sm16703P.c
OBKM_SRC  += $(OBK_SRCS)driver/drv_sm2135.c
OBKM_SRC  += $(OBK_SRCS)driver/drv_sm2235.c
OBKM_SRC  += $(OBK_SRCS)driver/drv_soft_i2c.c
OBKM_SRC  += $(OBK_SRCS)driver/drv_soft_spi.c
OBKM_SRC  += $(OBK_SRCS)driver/drv_spi.c
OBKM_SRC  += $(OBK_SRCS)driver/drv_spiLED.c
OBKM_SRC  += $(OBK_SRCS)driver/drv_spi_flash.c
OBKM_SRC  += $(OBK_SRCS)driver/drv_spidma.c
OBKM_SRC  += $(OBK_SRCS)driver/drv_ssdp.c
OBKM_SRC  += $(OBK_SRCS)driver/drv_tasmotaDeviceGroups.c
OBKM_SRC  += $(OBK_SRCS)driver/drv_tclAC.c
#OBKM_SRC += $(OBK_SRCS)driver/drv_test.c
#OBKM_SRC += $(OBK_SRCS)driver/drv_test_charts.c
OBKM_SRC  += $(OBK_SRCS)driver/drv_test_drivers.c
OBKM_SRC  += $(OBK_SRCS)driver/drv_textScroller.c
OBKM_SRC  += $(OBK_SRCS)driver/drv_tm1637.c
OBKM_SRC  += $(OBK_SRCS)driver/drv_tm1638.c
OBKM_SRC  += $(OBK_SRCS)driver/drv_tm_gn_display_shared.c
OBKM_SRC  += $(OBK_SRCS)driver/drv_tuyaMCU.c
OBKM_SRC  += $(OBK_SRCS)driver/drv_tuyaMCUSensor.c
OBKM_SRC  += $(OBK_SRCS)driver/drv_uart.c
OBKM_SRC  += $(OBK_SRCS)driver/drv_uart_tcp.c
OBKM_SRC  += $(OBK_SRCS)driver/drv_ucs1912.c
OBKM_SRC  += $(OBK_SRCS)driver/drv_wemo.c
OBKM_SRC  += $(OBK_SRCS)driver/drv_widget.c
OBKM_SRC  += $(OBK_SRCS)i2c/drv_i2c_ads1115.c
OBKM_SRC  += $(OBK_SRCS)i2c/drv_i2c_lcd_pcf8574t.c
OBKM_SRC  += $(OBK_SRCS)i2c/drv_i2c_main.c
OBKM_SRC  += $(OBK_SRCS)i2c/drv_i2c_mcp23017.c
OBKM_SRC  += $(OBK_SRCS)i2c/drv_i2c_tc74.c

OBKM_SRC_CXX += $(OBK_SRCS)driver/drv_ir.cpp
OBKM_SRC_CXX += $(OBK_SRCS)driver/drv_ir_new.cpp