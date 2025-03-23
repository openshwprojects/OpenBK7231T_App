OBK_DIR = ./app

INCLUDES += -I$(OBK_DIR)/../include
INCLUDES += -I./fixes

CCFLAGS += -DPLATFORM_BEKEN -DPLATFORM_BEKEN_NEW

ifdef USER_SW_VER
CCFLAGS += -DUSER_SW_VER='"${USER_SW_VER}"'
endif

ifeq ($(CFG_SOC_NAME), 1)
CFLAGS += -DPLATFORM_BK7231T
else ifeq ($(CFG_SOC_NAME), 2)
CCFLAGS += -DPLATFORM_BK7231U
else ifeq ($(CFG_SOC_NAME), 3)
CCFLAGS += -DPLATFORM_BK7251
else ifeq ($(CFG_SOC_NAME), 5)
CCFLAGS += -DPLATFORM_BK7231N
else ifeq ($(CFG_SOC_NAME), 7)
CCFLAGS += -DPLATFORM_BK7238
endif

APP_C += $(OBK_DIR)/../platforms/BK723x/ps.c

APP_C += $(OBK_DIR)/base64/base64.c
APP_C += $(OBK_DIR)/bitmessage/bitmessage_read.c
APP_C += $(OBK_DIR)/bitmessage/bitmessage_write.c
APP_C += $(OBK_DIR)/cJSON/cJSON.c
APP_C += $(OBK_DIR)/cmnds/cmd_channels.c
APP_C += $(OBK_DIR)/cmnds/cmd_eventHandlers.c
APP_C += $(OBK_DIR)/cmnds/cmd_if.c
APP_C += $(OBK_DIR)/cmnds/cmd_main.c
APP_C += $(OBK_DIR)/cmnds/cmd_newLEDDriver_colors.c
APP_C += $(OBK_DIR)/cmnds/cmd_newLEDDriver.c
APP_C += $(OBK_DIR)/cmnds/cmd_repeatingEvents.c
APP_C += $(OBK_DIR)/cmnds/cmd_script.c
APP_C += $(OBK_DIR)/cmnds/cmd_simulatorOnly.c
APP_C += $(OBK_DIR)/cmnds/cmd_tasmota.c
APP_C += $(OBK_DIR)/cmnds/cmd_tcp.c
APP_C += $(OBK_DIR)/cmnds/cmd_test.c
APP_C += $(OBK_DIR)/cmnds/cmd_tokenizer.c
APP_C += $(OBK_DIR)/devicegroups/deviceGroups_read.c
APP_C += $(OBK_DIR)/devicegroups/deviceGroups_util.c
APP_C += $(OBK_DIR)/devicegroups/deviceGroups_write.c
APP_C += $(OBK_DIR)/driver/drv_main.c
APP_C += $(OBK_DIR)/hal/bk7231/hal_adc_bk7231.c
APP_C += $(OBK_DIR)/hal/bk7231/hal_flashConfig_bk7231.c
APP_C += $(OBK_DIR)/hal/bk7231/hal_flashVars_bk7231.c
APP_C += $(OBK_DIR)/hal/bk7231/hal_generic_bk7231.c
APP_C += $(OBK_DIR)/hal/bk7231/hal_main_bk7231.c
APP_C += $(OBK_DIR)/hal/bk7231/hal_pins_bk7231.c
APP_C += $(OBK_DIR)/hal/bk7231/hal_wifi_bk7231.c
APP_C += $(OBK_DIR)/hal/bk7231/hal_uart_bk7231.c
APP_C += $(OBK_DIR)/httpserver/hass.c
APP_C += $(OBK_DIR)/httpserver/http_basic_auth.c
APP_C += $(OBK_DIR)/httpserver/http_fns.c
APP_C += $(OBK_DIR)/httpserver/new_tcp_server.c
#APP_C += $(OBK_DIR)/httpserver/http_tcp_server.c
APP_C += $(OBK_DIR)/httpserver/json_interface.c
APP_C += $(OBK_DIR)/httpserver/new_http.c
APP_C += $(OBK_DIR)/httpserver/rest_interface.c
APP_C += $(OBK_DIR)/mqtt/new_mqtt_deduper.c
APP_C += $(OBK_DIR)/jsmn/jsmn.c
APP_C += $(OBK_DIR)/logging/logging.c
APP_C += $(OBK_DIR)/mqtt/new_mqtt.c
APP_C += $(OBK_DIR)/new_cfg.c
APP_C += $(OBK_DIR)/new_common.c
APP_C += $(OBK_DIR)/new_ping.c
APP_C += $(OBK_DIR)/new_pins.c
APP_C += $(OBK_DIR)/rgb2hsv.c
APP_C += $(OBK_DIR)/tiny_crc8.c
APP_C += $(OBK_DIR)/user_main.c
APP_C += $(OBK_DIR)/cmnds/cmd_send.c
APP_C += $(OBK_DIR)/driver/drv_adcButton.c
APP_C += $(OBK_DIR)/driver/drv_adcSmoother.c
APP_C += $(OBK_DIR)/driver/drv_aht2x.c
APP_C += $(OBK_DIR)/driver/drv_battery.c
APP_C += $(OBK_DIR)/driver/drv_bl_shared.c
APP_C += $(OBK_DIR)/driver/drv_bl0937.c
APP_C += $(OBK_DIR)/driver/drv_bl0942.c
APP_C += $(OBK_DIR)/driver/drv_bmp280.c
APP_C += $(OBK_DIR)/driver/drv_bmpi2c.c
APP_C += $(OBK_DIR)/driver/drv_bp1658cj.c
APP_C += $(OBK_DIR)/driver/drv_bp5758d.c
APP_C += $(OBK_DIR)/driver/drv_bridge_driver.c
APP_C += $(OBK_DIR)/driver/drv_chargingLimit.c
APP_C += $(OBK_DIR)/driver/drv_charts.c
APP_C += $(OBK_DIR)/driver/drv_cht8305.c
APP_C += $(OBK_DIR)/driver/drv_cse7766.c
APP_C += $(OBK_DIR)/driver/drv_ddp.c
APP_C += $(OBK_DIR)/driver/drv_debouncer.c
APP_C += $(OBK_DIR)/driver/drv_dht_internal.c
APP_C += $(OBK_DIR)/driver/drv_dht.c
APP_C += $(OBK_DIR)/driver/drv_doorSensorWithDeepSleep.c
APP_C += $(OBK_DIR)/driver/drv_gn6932.c
APP_C += $(OBK_DIR)/driver/drv_hd2015.c
APP_C += $(OBK_DIR)/driver/drv_ht16k33.c
APP_C += $(OBK_DIR)/driver/drv_httpButtons.c
APP_C += $(OBK_DIR)/driver/drv_hue.c
APP_C += $(OBK_DIR)/driver/drv_kp18058.c
APP_C += $(OBK_DIR)/driver/drv_kp18068.c
APP_C += $(OBK_DIR)/driver/drv_max72xx_clock.c
APP_C += $(OBK_DIR)/driver/drv_max72xx_internal.c
APP_C += $(OBK_DIR)/driver/drv_max72xx_single.c
APP_C += $(OBK_DIR)/driver/drv_mcp9808.c
APP_C += $(OBK_DIR)/driver/drv_ntp_events.c
APP_C += $(OBK_DIR)/driver/drv_ntp.c
APP_C += $(OBK_DIR)/driver/drv_pt6523.c
APP_C += $(OBK_DIR)/driver/drv_pwm_groups.c
APP_C += $(OBK_DIR)/driver/drv_pwmToggler.c
APP_C += $(OBK_DIR)/driver/drv_pwrCal.c
APP_C += $(OBK_DIR)/driver/drv_rn8209.c
APP_C += $(OBK_DIR)/driver/drv_sgp.c
APP_C += $(OBK_DIR)/driver/drv_shiftRegister.c
APP_C += $(OBK_DIR)/driver/drv_sht3x.c
#APP_C += $(OBK_DIR)/driver/drv_sm15155e.c
#APP_C += $(OBK_DIR)/driver/drv_sm16703P.c
APP_C += $(OBK_DIR)/driver/drv_sm2135.c
APP_C += $(OBK_DIR)/driver/drv_sm2235.c
APP_C += $(OBK_DIR)/driver/drv_soft_i2c.c
APP_C += $(OBK_DIR)/driver/drv_soft_spi.c
APP_C += $(OBK_DIR)/driver/drv_spi_flash.c
APP_C += $(OBK_DIR)/driver/drv_spi.c
APP_C += $(OBK_DIR)/driver/drv_spidma.c
APP_C += $(OBK_DIR)/driver/drv_ssdp.c
APP_C += $(OBK_DIR)/driver/drv_tasmotaDeviceGroups.c
APP_C += $(OBK_DIR)/driver/drv_test_drivers.c
APP_C += $(OBK_DIR)/driver/drv_textScroller.c
APP_C += $(OBK_DIR)/driver/drv_tm_gn_display_shared.c
APP_C += $(OBK_DIR)/driver/drv_tm1637.c
APP_C += $(OBK_DIR)/driver/drv_tm1638.c
APP_C += $(OBK_DIR)/driver/drv_tuyaMCU.c
APP_C += $(OBK_DIR)/driver/drv_tuyaMCUSensor.c
APP_C += $(OBK_DIR)/driver/drv_uart.c
APP_C += $(OBK_DIR)/driver/drv_ucs1912.c
APP_C += $(OBK_DIR)/driver/drv_wemo.c
APP_C += $(OBK_DIR)/driver/drv_ds1820_simple.c
APP_C += $(OBK_DIR)/httpclient/http_client.c
APP_C += $(OBK_DIR)/httpclient/utils_net.c
APP_C += $(OBK_DIR)/httpclient/utils_timer.c
APP_C += $(OBK_DIR)/i2c/drv_i2c_lcd_pcf8574t.c
APP_C += $(OBK_DIR)/i2c/drv_i2c_main.c
APP_C += $(OBK_DIR)/i2c/drv_i2c_mcp23017.c
APP_C += $(OBK_DIR)/i2c/drv_i2c_tc74.c
APP_C += $(OBK_DIR)/littlefs/lfs_util.c
APP_C += $(OBK_DIR)/littlefs/lfs.c
APP_C += $(OBK_DIR)/littlefs/our_lfs.c
APP_C += $(OBK_DIR)/memory/memtest.c
APP_C += $(OBK_DIR)/ota/ota.c
APP_C += $(OBK_DIR)/sim/sim_uart.c
APP_C += $(OBK_DIR)/driver/drv_ir2.c
#APP_C += $(OBK_DIR)/driver/drv_pixelAnim.c

SRC_C += ./fixes/blank.c

APP_CXX += $(OBK_DIR)/driver/drv_ir.cpp
