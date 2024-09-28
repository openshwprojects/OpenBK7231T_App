include(ProjModuleCollect.cmake)

add_compile_definitions(PLATFORM_LN882H=1)


add_definitions( -DUSER_SW_VER="$(APP_VERSION)" )

set(PROJ_ALL_SRC
    app/src/base64/base64.c
    app/src/bitmessage/bitmessage_read.c
    app/src/bitmessage/bitmessage_write.c
    app/src/cJSON/cJSON.c
    app/src/cmnds/cmd_channels.c
    app/src/cmnds/cmd_eventHandlers.c
    app/src/cmnds/cmd_if.c
    app/src/cmnds/cmd_main.c
    app/src/cmnds/cmd_newLEDDriver_colors.c
    app/src/cmnds/cmd_newLEDDriver.c
    app/src/cmnds/cmd_repeatingEvents.c
    app/src/cmnds/cmd_script.c
#    app/src/cmnds/cmd_send.c
    app/src/cmnds/cmd_simulatorOnly.c
    app/src/cmnds/cmd_tasmota.c
    app/src/cmnds/cmd_tcp.c
    app/src/cmnds/cmd_test.c
    app/src/cmnds/cmd_tokenizer.c
    app/src/devicegroups/deviceGroups_read.c
    app/src/devicegroups/deviceGroups_util.c
    app/src/devicegroups/deviceGroups_write.c
    app/src/driver/drv_adcButton.c
    app/src/driver/drv_adcSmoother.c
    app/src/driver/drv_battery.c
    app/src/driver/drv_bl_shared.c
    app/src/driver/drv_bl0937.c
#    app/src/driver/drv_bl0942.c
    app/src/driver/drv_bmp280.c
    app/src/driver/drv_bp1658cj.c
    app/src/driver/drv_bp5758d.c
    app/src/driver/drv_bridge_driver.c
    app/src/driver/drv_chargingLimit.c
    app/src/driver/drv_cht8305.c
    app/src/driver/drv_cse7766.c
    app/src/driver/drv_ddp.c
    app/src/driver/drv_debouncer.c
    app/src/driver/drv_dht_internal.c
    app/src/driver/drv_dht.c
#    app/src/driver/drv_doorSensorWithDeepSleep.c
    app/src/driver/drv_gn6932.c
    app/src/driver/drv_hd2015.c
    app/src/driver/drv_ht16k33.c
    app/src/driver/drv_httpButtons.c
    app/src/driver/drv_hue.c
    app/src/driver/drv_kp18058.c
    app/src/driver/drv_kp18068.c
    app/src/driver/drv_main.c
    app/src/driver/drv_max72xx_clock.c
    app/src/driver/drv_max72xx_internal.c
    app/src/driver/drv_max72xx_single.c
    app/src/driver/drv_mcp9808.c
#    app/src/driver/drv_ntp_events.c
    app/src/driver/drv_ntp.c
    app/src/driver/drv_pt6523.c
    app/src/driver/drv_pwm_groups.c
    app/src/driver/drv_pwmToggler.c
    app/src/driver/drv_pwrCal.c
#    app/src/driver/drv_rn8209.c
    app/src/driver/drv_sgp.c
    app/src/driver/drv_shiftRegister.c
    app/src/driver/drv_sht3x.c
#    app/src/driver/drv_sm16703P.c
    app/src/driver/drv_sm2135.c
    app/src/driver/drv_sm2235.c
    app/src/driver/drv_soft_i2c.c
    app/src/driver/drv_soft_spi.c
#    app/src/driver/drv_spi_flash.c
#    app/src/driver/drv_spi.c
#    app/src/driver/drv_spidma.c
    app/src/driver/drv_ssdp.c
    app/src/driver/drv_tasmotaDeviceGroups.c
    app/src/driver/drv_test_drivers.c
    app/src/driver/drv_textScroller.c
    app/src/driver/drv_tm_gn_display_shared.c
    app/src/driver/drv_tm1637.c
    app/src/driver/drv_tm1638.c
#    app/src/driver/drv_tuyaMCU.c
#    app/src/driver/drv_tuyaMCUSensor.c
#    app/src/driver/drv_uart.c
#    app/src/driver/drv_ucs1912.c
    app/src/driver/drv_wemo.c
    app/src/driver/drv_ds1820_simple.c
    app/src/hal/ln882h/hal_adc_ln882h.c
    app/src/hal/ln882h/hal_flashConfig_ln882h.c
    app/src/hal/ln882h/hal_flashVars_ln882h.c
    app/src/hal/ln882h/hal_generic_ln882h.c
    app/src/hal/ln882h/hal_main_ln882h.c
    app/src/hal/ln882h/hal_pins_ln882h.c
    app/src/hal/ln882h/hal_wifi_ln882h.c
    app/src/httpserver/hass.c
    app/src/httpserver/http_basic_auth.c
    app/src/httpserver/http_fns.c
    app/src/httpserver/http_tcp_server_nonblocking.c
    app/src/httpserver/http_tcp_server.c
    app/src/httpserver/json_interface.c
    app/src/httpserver/new_http.c
    app/src/httpserver/rest_interface.c
#    app/src/httpclient/http_client.c
#    app/src/httpclient/utils_net.c
#    app/src/httpclient/utils_timer.c
#    app/src/i2c/drv_i2c_lcd_pcf8574t.c
#    app/src/i2c/drv_i2c_main.c
#    app/src/i2c/drv_i2c_mcp23017.c
#    app/src/i2c/drv_i2c_tc74.c
    app/src/jsmn/jsmn.c
    app/src/littlefs/lfs_util.c
    app/src/littlefs/lfs.c
    app/src/littlefs/our_lfs.c
    app/src/logging/logging.c
#    app/src/memory/memtest.c
    app/src/mqtt/new_mqtt_deduper.c
    app/src/mqtt/new_mqtt.c
    app/src/new_cfg.c
    app/src/new_common.c
    app/src/new_ping.c
    app/src/new_pins.c
#    app/src/ota/ota.c
    app/src/rgb2hsv.c
#    app/src/sim/sim_uart.c
    app/src/tiny_crc8.c
    app/src/user_main.c
    main.c
    usr_app.c
    bsp/serial_hw.c
    startup/startup_${CHIP_SERIAL}_gcc.c
    ${MODULE_SRC}
	${COMP_LWIP_DIR}/apps/mqtt/mqtt.c
)

set(COMP_DHCPD_SUPPORT          ON  PARENT_SCOPE)
set(COMP_LWIP_SUPPORT           ON  PARENT_SCOPE)

#----------------------------   project output elf   ---------------------------
set(TARGET_ELF_NAME                         ${USER_PROJECT})
set(pro_executable_target                   ${TARGET_ELF_NAME}.elf)
add_executable(${pro_executable_target}     ${PROJ_ALL_SRC})

target_link_libraries(${pro_executable_target}
    PUBLIC
    ln::dhcpd
    lwip
    ${CHIP_SERIAL}_wifi
    -lc -lm -lnosys

    PRIVATE
    -T${LINKER_SCRIPT}
    ${EXTRA_LINK_FLAGS}
)

target_link_directories(${pro_executable_target}
    PRIVATE
    ${LN_SDK_ROOT}/lib/gcclib
)

target_include_directories(${pro_executable_target}
    PRIVATE
    app
    app/include
    bsp
    cfg
)

include(gcc/gcc-custom-build-stage.cmake)
