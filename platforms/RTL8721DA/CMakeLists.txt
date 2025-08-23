ameba_internal_library(OpenBeken)

set(public_definitions)
set(SDK_CJSON true)
set(SDK_LFS true)
set(OBK_SRCS "../../src/")
set(BERRY_SRCPATH "../../libraries/berry/src")
set(BERRY_MODULEPATH "${OBK_SRCS}berry/modules")

include(../obk_main.cmake)
include(../../libraries/berry.cmake)

ameba_list_append(public_definitions LWIP_NETIF_HOSTNAME=1 TCP_MSL=1000)
ameba_global_define(${public_definitions})

add_compile_definitions(
	PLATFORM_REALTEK=1
	PLATFORM_REALTEK_NEW=1
	PLATFORM_RTL8721DA=1
)

if(DEFINED ENV{APP_VERSION})
	add_compile_definitions(USER_SW_VER="$ENV{APP_VERSION}")
endif()
if(DEFINED ENV{OBK_VARIANT})
	add_compile_definitions(OBK_VARIANT=$ENV{OBK_VARIANT})
endif()

target_compile_options(
	${CURRENT_LIB_NAME} PRIVATE
	-Wno-undef
	-Wno-unused-parameter
	-Wno-unused-variable
	-Wno-implicit-fallthrough
	-Wno-missing-field-initializers
	-Wno-error
	"$<$<COMPILE_LANGUAGE:C>:-Wno-strict-prototypes>"
)

target_include_directories(
	${CURRENT_LIB_NAME} PRIVATE
	../../include
	${CMAKE_CURRENT_SOURCE_DIR}
	${BERRY_SRCPATH}
)

target_sources(
	${CURRENT_LIB_NAME} PRIVATE
	
	${OBK_SRCS}hal/realtek/hal_flashConfig_realtek.c
	${OBK_SRCS}hal/realtek/hal_flashVars_realtek.c
	${OBK_SRCS}hal/realtek/hal_generic_realtek.c
	${OBK_SRCS}hal/realtek/hal_pins_realtek.c
	${OBK_SRCS}hal/realtek/hal_ota_realtek.c
	${OBK_SRCS}hal/realtek/hal_wifi_realtek_new.c
	${OBK_SRCS}hal/realtek/rtl8721da/hal_main_rtl8721da.c
	${OBK_SRCS}hal/realtek/rtl8721da/hal_pins_rtl8721da.c
	${OBK_SRCS}hal/realtek/rtl8721da/hal_uart_rtl8721da.c

	${OBKM_SRC}
	${OBKM_SRC_CXX}
	${BERRY_SRC_C}
	../../libraries/mqtt_patched.c
)
