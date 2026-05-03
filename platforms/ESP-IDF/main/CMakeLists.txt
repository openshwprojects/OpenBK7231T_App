set(OBK_SRCS "../../../src/")
include($ENV{IDF_PATH}/../../platforms/obk_main.cmake)

set(BERRY_SRCPATH "../../../libraries/berry/src")
set(BERRY_MODULEPATH "${OBK_SRCS}berry/modules")
include($ENV{IDF_PATH}/../../libraries/berry.cmake)

set(PROJ_ALL_SRC
	${OBK_SRCS}hal/espidf/hal_adc_espidf.c
	${OBK_SRCS}hal/espidf/hal_flashConfig_espidf.c
	${OBK_SRCS}hal/espidf/hal_flashVars_espidf.c
	${OBK_SRCS}hal/espidf/hal_generic_espidf.c
	${OBK_SRCS}hal/espidf/hal_hwtimer_espidf.c
	${OBK_SRCS}hal/espidf/hal_main_espidf.c
	${OBK_SRCS}hal/espidf/hal_bt_proxy_espidf.c
	${OBK_SRCS}hal/espidf/hal_pins_espidf.c
	${OBK_SRCS}hal/espidf/hal_wifi_espidf.c
	${OBK_SRCS}hal/espidf/hal_uart_espidf.c
	${OBK_SRCS}hal/espidf/hal_ota_espidf.c
	${OBKM_SRC}
	${OBKM_SRC_CXX}
	${BERRY_SRC_C}
	../../../libraries/mqtt_patched.c
)
set(ENABLE_MDNS_ESPIDF 0)
if("$ENV{IDF_TARGET}" STREQUAL "esp32" OR "$ENV{IDF_TARGET}" STREQUAL "esp32c6" OR "$ENV{IDF_TARGET}" STREQUAL "esp32s3")
	set(ENABLE_MDNS_ESPIDF 1)
# C3 2M CI image overflows app partition; keep mDNS sources only for 4M variant for now.
elseif("$ENV{IDF_TARGET}" STREQUAL "esp32c3" AND "$ENV{OBK_VARIANT}" STREQUAL "2")
	set(ENABLE_MDNS_ESPIDF 1)
endif()
if(ENABLE_MDNS_ESPIDF)
	list(APPEND PROJ_ALL_SRC
		../../../sdk/esp-idf/components/lwip/lwip/src/apps/mdns/mdns.c
		../../../sdk/esp-idf/components/lwip/lwip/src/apps/mdns/mdns_domain.c
		../../../sdk/esp-idf/components/lwip/lwip/src/apps/mdns/mdns_out.c
	)
endif()
set(PRIV_REQ_COMPONENTS mqtt lwip esp_wifi nvs_flash esp_driver_tsens esp_driver_gpio esp_pm esp_partition app_update esp_adc esp_driver_uart esp_driver_ledc spi_flash esp_driver_spi esp_driver_gptimer)
message(STATUS "ENABLE_DISPLAY is '${ENABLE_DISPLAY}' in main/CMakeLists.txt")
if(ENABLE_DISPLAY OR (DEFINED ENV{ENABLE_DISPLAY} AND "$ENV{ENABLE_DISPLAY}" EQUAL "1"))
	list(APPEND PRIV_REQ_COMPONENTS ESP32_Display_Panel lvgl)
endif()

idf_component_register(SRCS ${PROJ_ALL_SRC} WHOLE_ARCHIVE
			PRIV_REQUIRES ${PRIV_REQ_COMPONENTS})

target_compile_options(${COMPONENT_LIB} PRIVATE "-Wno-error=format-truncation" "-Wno-error=incompatible-pointer-types" "-Wno-error=int-conversion" "-Wno-error=unused-but-set-variable" "-Wno-error=unused-function" "-Wno-error=unused-variable" "-Wno-error=implicit-fallthrough" "-Wno-error=maybe-uninitialized")

target_compile_definitions(${COMPONENT_LIB} PRIVATE USER_SW_VER="$ENV{APP_VERSION}")
target_compile_definitions(${COMPONENT_LIB} PRIVATE OBK_VARIANT=$ENV{OBK_VARIANT})
