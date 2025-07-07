ameba_internal_library(OpenBeken)

add_compile_definitions(PLATFORM_REALTEK=1)
add_compile_definitions(PLATFORM_REALTEK_NEW=1)
add_compile_definitions(PLATFORM_RTL8721DA=1)
add_compile_definitions(LWIP_NETIF_HOSTNAME=1)
add_compile_definitions(TCP_MSL=1000)

if(DEFINED ENV{APP_VERSION})
	add_definitions( -DUSER_SW_VER="$ENV{APP_VERSION}" )
endif()
if(DEFINED ENV{OBK_VARIANT})
	add_definitions( -DOBK_VARIANT=$ENV{OBK_VARIANT} )
endif()

target_compile_options(
    ${CURRENT_LIB_NAME} PRIVATE
    -Wno-undef
    -Wno-unused-parameter
    -Wno-unused-variable
    -Wno-strict-prototypes
	-Wno-implicit-fallthrough
	-Wno-missing-field-initializers
	-Wno-error=implicit-function-declaration
	-Wno-error=unused-but-set-variable
	-Wno-error=sign-compare
	-Wno-error=discarded-qualifiers
	-Wno-error=incompatible-pointer-types
	-Wno-error=unused-function
	-Wno-error=type-limits
	-Wno-error
)
set(SDK_CJSON true)
set(SDK_LFS true)
set(OBK_SRCS "../../src/")
include(../obk_main.cmake)

set(BERRY_SRCPATH "../../libraries/berry/src")
set(BERRY_MODULEPATH "${OBK_SRCS}berry/modules")
include_directories(${BERRY_SRCPATH})
include(../../libraries/berry.cmake)
target_include_directories(
	${CURRENT_LIB_NAME} PRIVATE
	../../libraries/berry/src
	../../include
	${CMAKE_CURRENT_SOURCE_DIR}
	#../../libraries/easyflash/inc
)
target_sources(
	${CURRENT_LIB_NAME} PRIVATE

	${OBK_SRCS}hal/realtek/hal_flashConfig_realtek.c
	${OBK_SRCS}hal/realtek/hal_flashVars_realtek.c
	${OBK_SRCS}hal/realtek/hal_generic_realtek.c
	${OBK_SRCS}hal/realtek/hal_pins_realtek.c
	${OBK_SRCS}hal/realtek/hal_wifi_realtek_new.c
	${OBK_SRCS}hal/realtek/rtl8721da/hal_main_rtl8721da.c
	${OBK_SRCS}hal/realtek/rtl8721da/hal_pins_rtl8721da.c
	${OBK_SRCS}hal/realtek/rtl8721da/hal_uart_rtl8721da.c

	${OBKM_SRC}
	${BERRY_SRC_C}
	../../libraries/mqtt_patched.c
	#../../libraries/easyflash/ports/ef_port.c
	#../../libraries/easyflash/src/easyflash.c
	#../../libraries/easyflash/src/ef_env.c
	#../../libraries/easyflash/src/ef_env_legacy.c
	#../../libraries/easyflash/src/ef_env_legacy_wl.c
	#../../libraries/easyflash/src/ef_iap.c
	#../../libraries/easyflash/src/ef_log.c
	#../../libraries/easyflash/src/ef_utils.c
)