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
	${OBK_SRCS}hal/espidf/hal_main_espidf.c
	${OBK_SRCS}hal/espidf/hal_pins_espidf.c
	${OBK_SRCS}hal/espidf/hal_wifi_espidf.c
	${OBK_SRCS}hal/espidf/hal_uart_espidf.c
	${OBK_SRCS}hal/espidf/hal_ota_espidf.c
	${OBKM_SRC}
	${BERRY_SRC_C}
	../../../libraries/mqtt_patched.c
)
idf_component_register(SRCS ${PROJ_ALL_SRC}
			PRIV_REQUIRES lwip nvs_flash app_update)