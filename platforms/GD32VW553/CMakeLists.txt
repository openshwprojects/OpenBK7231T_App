cmake_minimum_required(VERSION 3.15)

set(OBK_SRCS "../../src/")
include(../../platforms/obk_main.cmake)

set(OBKFLAGS -DPLATFORM_GD32VW553=1)

if(DEFINED ENV{APP_VERSION})
	set(OBKFLAGS ${OBKFLAGS} -DUSER_SW_VER="$ENV{APP_VERSION}")
endif()
if(DEFINED ENV{OBK_VARIANT})
	set(OBKFLAGS ${OBKFLAGS} -DOBK_VARIANT=$ENV{OBK_VARIANT})
endif()

target_include_directories(${TARGET_EXE} PRIVATE
        ../include
)

target_compile_options(${TARGET_EXE} PRIVATE
	${OBKFLAGS}
)

target_sources(${TARGET_EXE} PRIVATE
	${OBK_SRCS}hal/gd32vw553/hal_adc_gd32vw553.c
	${OBK_SRCS}hal/gd32vw553/hal_bt_proxy_gd32vw553.c
	${OBK_SRCS}hal/gd32vw553/hal_flashConfig_gd32vw553.c
	${OBK_SRCS}hal/gd32vw553/hal_flashVars_gd32vw553.c
	${OBK_SRCS}hal/gd32vw553/hal_generic_gd32vw553.c
	${OBK_SRCS}hal/gd32vw553/hal_hwtimer_gd32vw553.c
	${OBK_SRCS}hal/gd32vw553/hal_main_gd32vw553.c
	${OBK_SRCS}hal/gd32vw553/hal_ota_gd32vw553.c
	${OBK_SRCS}hal/gd32vw553/hal_pins_gd32vw553.c
	${OBK_SRCS}hal/gd32vw553/hal_uart_gd32vw553.c
	${OBK_SRCS}hal/gd32vw553/hal_wifi_gd32vw553.c
	${OBKM_SRC}
	${OBKM_SRC_CXX}
	${BERRY_SRC_C}
	../../libraries/mqtt_patched.c
)
