include(ProjModuleCollect.cmake)

add_compile_definitions(PLATFORM_LN882H=1)

add_definitions( -DUSER_SW_VER="$ENV{APP_VERSION}" )
add_definitions( -DOBK_VARIANT=$ENV{OBK_VARIANT} )

set(OBK_SRCS "app/src/")
include(app/platforms/obk_main.cmake)

set(BERRY_SRCPATH "app/libraries/berry/src")
set(BERRY_MODULEPATH "${OBK_SRCS}berry/modules")
include_directories(${BERRY_SRCPATH})
include_directories(app/platforms/LN882H)
include(app/libraries/berry.cmake)

set(PROJ_ALL_SRC
	${OBKM_SRC_CXX}
	${OBKM_SRC}
	${BERRY_SRC_C}
	${OBK_SRCS}hal/ln882h/hal_adc_ln882h.c
	${OBK_SRCS}hal/ln882h/hal_flashConfig_ln882h.c
	${OBK_SRCS}hal/ln882h/hal_flashVars_ln882h.c
	${OBK_SRCS}hal/ln882h/hal_generic_ln882h.c
	${OBK_SRCS}hal/ln882h/hal_hwtimer_ln882h.c
	${OBK_SRCS}hal/ln882h/hal_main_ln882h.c
	${OBK_SRCS}hal/ln882h/hal_pins_ln882h.c
	${OBK_SRCS}hal/ln882h/hal_wifi_ln882h.c
	${OBK_SRCS}hal/ln882h/hal_ota_ln882h.c
	${OBK_SRCS}hal/ln882h/hal_uart_ln882h.c
	${OBK_SRCS}hal/ln882h/hal_bt_proxy_ln882h.c
	main.c
	bsp/serial_hw.c
	startup/startup_${CHIP_SERIAL}_gcc.c
	${MODULE_SRC}
	${COMP_LWIP_DIR}/apps/mqtt/mqtt.c
	${MCU_LN882X_DIR}/driver_${CHIP_SERIAL}/hal/hal_dma.c
	${MCU_LN882X_DIR}/driver_${CHIP_SERIAL}/hal/hal_spi.c
)

set(COMP_DHCPD_SUPPORT          ON  PARENT_SCOPE)
set(COMP_LWIP_SUPPORT           ON  PARENT_SCOPE)

###############################   BLE (export) #################################
file(GLOB_RECURSE  BLE_LIB_IMPORT_SRC   ${COMP_BLE_DIR}/ble_lib_import/*.c)
file(GLOB_RECURSE  BLE_ARCH             ${COMP_BLE_DIR}/ble_arch/*.c)
file(GLOB_RECURSE  BLE_PROFILE_COMMON   ${COMP_BLE_DIR}/ble_profiles/prf_common/*.c)
file(GLOB_RECURSE  BLE_APP_COMP         ${COMP_BLE_DIR}/ble_app/*.c)
include_directories(${COMP_BLE_DIR})
include_directories(${COMP_BLE_DIR}/ble_arch)
include_directories(${COMP_BLE_DIR}/ble_profiles/prf_common)
include_directories(${COMP_BLE_DIR}/ble_lib_import)

include_directories(${COMP_BLE_DIR}/ble_app/ble_common)
include_directories(${COMP_BLE_DIR}/ble_app/ble_connection_manager)
include_directories(${COMP_BLE_DIR}/ble_app/ble_device_manager)
include_directories(${COMP_BLE_DIR}/ble_app/ble_event)
include_directories(${COMP_BLE_DIR}/ble_app/ble_gap/gap_advertising)
include_directories(${COMP_BLE_DIR}/ble_app/ble_gap/gap_misc)
include_directories(${COMP_BLE_DIR}/ble_app/ble_gap/gap_scan)
include_directories(${COMP_BLE_DIR}/ble_app/ble_gatt/gatt_client)
include_directories(${COMP_BLE_DIR}/ble_app/ble_gatt/gatt_common)
include_directories(${COMP_BLE_DIR}/ble_app/ble_gatt/gatt_server)
include_directories(${COMP_BLE_DIR}/ble_app/ble_import)
include_directories(${COMP_BLE_DIR}/ble_app/ble_smp)
include_directories(${COMP_BLE_DIR}/ble_app/ble_store)
include_directories(${COMP_BLE_DIR}/ble_app/ble_test)

include_directories(${COMP_BLE_DIR}/mac/ble/hl/api)
include_directories(${COMP_BLE_DIR}/mac/ble/hl/inc)
include_directories(${COMP_BLE_DIR}/mac/ble/ll/api)
include_directories(${COMP_BLE_DIR}/mac/ble/ll/import)
include_directories(${COMP_BLE_DIR}/mac/ble/ll/src)
include_directories(${COMP_BLE_DIR}/mac/ble/ll/src/llm)
include_directories(${COMP_BLE_DIR}/mac/em/api)
include_directories(${COMP_BLE_DIR}/mac/hci/api)
include_directories(${COMP_BLE_DIR}/mac/sch/api)
include_directories(${COMP_BLE_DIR}/mac/sch/import)
include_directories(${COMP_BLE_DIR}/modules/aes/api)
include_directories(${COMP_BLE_DIR}/modules/aes/api)
include_directories(${COMP_BLE_DIR}/modules/common/api)
include_directories(${COMP_BLE_DIR}/modules/dbg/api)
include_directories(${COMP_BLE_DIR}/modules/ecc_p256/api)
include_directories(${COMP_BLE_DIR}/modules/h4tl/api)
include_directories(${COMP_BLE_DIR}/modules/ke/api)
include_directories(${COMP_BLE_DIR}/modules/lib_ver/api)
include_directories(${COMP_BLE_DIR}/modules/nvds/api)
include_directories(${COMP_BLE_DIR}/modules/rf/api)
include_directories(${COMP_BLE_DIR}/modules/rwip/api)

list(APPEND PROJ_ALL_SRC ${BLE_LIB_IMPORT_SRC})
list(APPEND PROJ_ALL_SRC ${BLE_ARCH})
list(APPEND PROJ_ALL_SRC ${BLE_PROFILE_COMMON})
list(APPEND PROJ_ALL_SRC ${BLE_APP_COMP})

#----------------------------   project output elf   ---------------------------
set(TARGET_ELF_NAME                         ${USER_PROJECT})
set(pro_executable_target                   ${TARGET_ELF_NAME}.elf)
add_executable(${pro_executable_target}     ${PROJ_ALL_SRC})

target_link_libraries(${pro_executable_target}
    PUBLIC
    ln::dhcpd
    lwip
    ${CHIP_SERIAL}_wifi
    ${CHIP_SERIAL}_ble_full_stack
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

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-undef -Wno-unused-parameter -Wno-pedantic -Wno-float-conversion")
