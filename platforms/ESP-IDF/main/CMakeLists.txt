set(PROJ_ALL_SRC
	../../../src/base64/base64.c
	../../../src/bitmessage/bitmessage_read.c
	../../../src/bitmessage/bitmessage_write.c
	../../../src/cJSON/cJSON.c
	../../../src/cmnds/cmd_channels.c
	../../../src/cmnds/cmd_eventHandlers.c
	../../../src/cmnds/cmd_if.c
	../../../src/cmnds/cmd_main.c
	../../../src/cmnds/cmd_newLEDDriver_colors.c
	../../../src/cmnds/cmd_newLEDDriver.c
	../../../src/cmnds/cmd_repeatingEvents.c
	../../../src/cmnds/cmd_script.c
#	../../../src/cmnds/cmd_send.c
	../../../src/cmnds/cmd_simulatorOnly.c
	../../../src/cmnds/cmd_tasmota.c
	../../../src/cmnds/cmd_tcp.c
	../../../src/cmnds/cmd_test.c
	../../../src/cmnds/cmd_tokenizer.c
	../../../src/devicegroups/deviceGroups_read.c
	../../../src/devicegroups/deviceGroups_util.c
	../../../src/devicegroups/deviceGroups_write.c
	../../../src/driver/drv_adcButton.c
	../../../src/driver/drv_adcSmoother.c
	../../../src/driver/drv_aht2x.c
	../../../src/driver/drv_battery.c
	../../../src/driver/drv_bl_shared.c
	../../../src/driver/drv_bl0937.c
#	../../../src/driver/drv_bl0942.c
	../../../src/driver/drv_bmp280.c
	../../../src/driver/drv_bmpi2c.c
	../../../src/driver/drv_bp1658cj.c
	../../../src/driver/drv_bp5758d.c
	../../../src/driver/drv_bridge_driver.c
	../../../src/driver/drv_chargingLimit.c
	../../../src/driver/drv_charts.c
	../../../src/driver/drv_cht8305.c
	../../../src/driver/drv_cse7766.c
	../../../src/driver/drv_ddp.c
	../../../src/driver/drv_debouncer.c
	../../../src/driver/drv_dht_internal.c
	../../../src/driver/drv_dht.c
#	../../../src/driver/drv_doorSensorWithDeepSleep.c
	../../../src/driver/drv_gn6932.c
	../../../src/driver/drv_hd2015.c
	../../../src/driver/drv_ht16k33.c
	../../../src/driver/drv_httpButtons.c
	../../../src/driver/drv_hue.c
	../../../src/driver/drv_kp18058.c
	../../../src/driver/drv_kp18068.c
	../../../src/driver/drv_main.c
	../../../src/driver/drv_max72xx_clock.c
	../../../src/driver/drv_max72xx_internal.c
	../../../src/driver/drv_max72xx_single.c
	../../../src/driver/drv_mcp9808.c
	../../../src/driver/drv_ntp_events.c
	../../../src/driver/drv_ntp.c
	../../../src/driver/drv_pt6523.c
	../../../src/driver/drv_pwm_groups.c
	../../../src/driver/drv_pwmToggler.c
	../../../src/driver/drv_pwrCal.c
#	../../../src/driver/drv_rn8209.c
	../../../src/driver/drv_sgp.c
	../../../src/driver/drv_shiftRegister.c
	../../../src/driver/drv_sht3x.c
#	../../../src/driver/drv_sm15155e.c
#	../../../src/driver/drv_sm16703P.c
	../../../src/driver/drv_sm2135.c
	../../../src/driver/drv_sm2235.c
	../../../src/driver/drv_soft_i2c.c
	../../../src/driver/drv_soft_spi.c
#	../../../src/driver/drv_spi_flash.c
#	../../../src/driver/drv_spi.c
#	../../../src/driver/drv_spidma.c
	../../../src/driver/drv_ssdp.c
	../../../src/driver/drv_tasmotaDeviceGroups.c
	../../../src/driver/drv_test_drivers.c
	../../../src/driver/drv_textScroller.c
	../../../src/driver/drv_tm_gn_display_shared.c
	../../../src/driver/drv_tm1637.c
	../../../src/driver/drv_tm1638.c
	../../../src/driver/drv_tuyaMCU.c
	../../../src/driver/drv_tuyaMCUSensor.c
	../../../src/driver/drv_uart.c
#	../../../src/driver/drv_ucs1912.c
	../../../src/driver/drv_wemo.c
	../../../src/driver/drv_ds1820_simple.c
	../../../src/hal/espidf/hal_adc_espidf.c
	../../../src/hal/espidf/hal_flashConfig_espidf.c
	../../../src/hal/espidf/hal_flashVars_espidf.c
	../../../src/hal/espidf/hal_generic_espidf.c
	../../../src/hal/espidf/hal_main_espidf.c
	../../../src/hal/espidf/hal_pins_espidf.c
	../../../src/hal/espidf/hal_wifi_espidf.c
	../../../src/httpserver/hass.c
	../../../src/httpserver/http_basic_auth.c
	../../../src/httpserver/http_fns.c
	../../../src/httpserver/http_tcp_server_nonblocking.c
	../../../src/httpserver/http_tcp_server.c
	../../../src/httpserver/json_interface.c
	../../../src/httpserver/new_http.c
	../../../src/httpserver/rest_interface.c
#	../../../src/httpclient/http_client.c
#	../../../src/httpclient/utils_net.c
#	../../../src/httpclient/utils_timer.c
	../../../src/i2c/drv_i2c_lcd_pcf8574t.c
	../../../src/i2c/drv_i2c_main.c
	../../../src/i2c/drv_i2c_mcp23017.c
	../../../src/i2c/drv_i2c_tc74.c
	../../../src/jsmn/jsmn.c
	../../../src/littlefs/lfs_util.c
	../../../src/littlefs/lfs.c
	../../../src/littlefs/our_lfs.c
	../../../src/logging/logging.c
#	../../../src/memory/memtest.c
	../../../src/mqtt/new_mqtt_deduper.c
	../../../src/mqtt/new_mqtt.c
	../../../src/new_cfg.c
	../../../src/new_common.c
	../../../src/new_ping.c
	../../../src/new_pins.c
#	../../../src/ota/ota.c
	../../../src/rgb2hsv.c
#	../../../src/sim/sim_uart.c
	../../../src/tiny_crc8.c
	../../../src/user_main.c
	$ENV{IDF_PATH}/components/lwip/lwip/src/apps/mqtt/mqtt.c
	app.c
)
idf_component_register(SRCS ${PROJ_ALL_SRC} 
			PRIV_REQUIRES mqtt lwip esp_wifi nvs_flash esp_driver_tsens esp_driver_gpio esp_pm esp_partition app_update esp_adc esp_driver_uart esp_driver_ledc)
