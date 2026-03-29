cmake_minimum_required(VERSION 3.15)

find_package(bouffalo_sdk REQUIRED HINTS $ENV{BL_SDK_BASE})

sdk_set_main_file(main.c)

sdk_add_compile_definitions(
	-DPLATFORM_BL_NEW=1
	-DPLATFORM_BL616=1
)
sdk_add_compile_options(
	-Os
	-Wno-undef
	-Wno-unused-variable
	-Wno-return-type
	-g0
)

if(DEFINED ENV{APP_VERSION})
	sdk_add_compile_definitions(-DUSER_SW_VER="$ENV{APP_VERSION}")
endif()
if(DEFINED ENV{OBK_VARIANT})
	sdk_add_compile_definitions(-DOBK_VARIANT=$ENV{OBK_VARIANT})
endif()

set(SDK_CJSON true)
set(SDK_LFS true)
set(OBK_SRCS "../../src/")
include($ENV{BL_SDK_BASE}/../../platforms/obk_main.cmake)

sdk_add_include_directories(.)
target_sources(app PRIVATE
#	${OBK_SRCS}hal/bl_new/hal_adc_bl_new.c
	${OBK_SRCS}hal/bl_new/hal_flashConfig_bl_new.c
	${OBK_SRCS}hal/bl_new/hal_flashVars_bl_new.c
	${OBK_SRCS}hal/bl_new/hal_generic_bl_new.c
	${OBK_SRCS}hal/bl_new/hal_hwtimer_bl_new.c
#	${OBK_SRCS}hal/bl_new/hal_main_bl_new.c
	${OBK_SRCS}hal/bl_new/hal_pins_bl_new.c
	${OBK_SRCS}hal/bl_new/hal_wifi_bl_new.c
#	${OBK_SRCS}hal/bl_new/hal_uart_bl_new.c
	${OBK_SRCS}hal/bl_new/hal_ota_bl_new.c
	${OBKM_SRC}
	${OBKM_SRC_CXX}
	${BERRY_SRC_C}
	../../libraries/mqtt_patched.c)

project(OpenBeken)
