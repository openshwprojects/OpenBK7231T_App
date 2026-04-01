cmake_minimum_required(VERSION 3.15)

find_package(bouffalo_sdk REQUIRED HINTS $ENV{BL_SDK_BASE})

sdk_set_main_file(main.c)

sdk_add_compile_definitions(
	-DPLATFORM_BL_NEW=1
	-DPLATFORM_BL602=1
	-DDHCPD_SERVER_IP="192.168.4.1"
	-DLFS_THREADSAFE
)
sdk_add_compile_options(
	-Os
	-Wno-undef
	-Wno-unused-variable
	-Wno-return-type
	-g0
	-fstrict-aliasing -fno-strict-overflow
	-falign-functions=32 -falign-jumps=32 -falign-loops=32
)

if(DEFINED ENV{APP_VERSION})
	sdk_add_compile_definitions(-DUSER_SW_VER="$ENV{APP_VERSION}")
endif()
if(DEFINED ENV{OBK_VARIANT})
	sdk_add_compile_definitions(-DOBK_VARIANT=$ENV{OBK_VARIANT})
endif()

set(SDK_CJSON true)
#set(SDK_LFS true)
set(OBK_SRCS "../../src/")
include($ENV{BL_SDK_BASE}/../../platforms/obk_main.cmake)

sdk_add_include_directories(.)
sdk_add_include_directories(${OBK_SRCS}../libraries/easyflash/inc)

target_sources(app PRIVATE
	${OBK_SRCS}hal/bl602/hal_flashConfig_bl602.c
	${OBK_SRCS}hal/bl602/hal_flashVars_bl602.c
	${OBK_SRCS}hal/bl602/hal_generic_bl602.c
	${OBK_SRCS}hal/bl602/hal_hwtimer_bl_new.c
	${OBK_SRCS}hal/bl602/hal_pins_bl_new.c
	${OBK_SRCS}hal/bl602/hal_wifi_bl602.c
	${OBK_SRCS}hal/bl602/hal_ota_bl602.c
	${OBKM_SRC}
	${OBKM_SRC_CXX}
	${BERRY_SRC_C}
	${OBK_SRCS}../libraries/easyflash/ports/ef_port.c
	${OBK_SRCS}../libraries/easyflash/src/easyflash.c
	${OBK_SRCS}../libraries/easyflash/src/ef_env.c
	${OBK_SRCS}../libraries/easyflash/src/ef_utils.c
	../../libraries/mqtt_patched.c)

project(OpenBeken)
