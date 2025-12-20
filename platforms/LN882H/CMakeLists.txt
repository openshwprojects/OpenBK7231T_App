include(ProjModuleCollect.cmake)

add_compile_definitions(PLATFORM_LN882H=1)

add_definitions( -DUSER_SW_VER="$ENV{APP_VERSION}" )
add_definitions( -DOBK_VARIANT=$ENV{OBK_VARIANT} )

set(OBK_SRCS "app/src/")
include(app/platforms/obk_main.cmake)

set(BERRY_SRCPATH "app/libraries/berry/src")
set(BERRY_MODULEPATH "${OBK_SRCS}berry/modules")
include_directories(${BERRY_SRCPATH})
include(app/libraries/berry.cmake)

set(PROJ_ALL_SRC
	${OBKM_SRC_CXX}
	${OBKM_SRC}
	${BERRY_SRC_C}
	${OBK_SRCS}hal/ln882h/hal_adc_ln882h.c
	${OBK_SRCS}hal/ln882h/hal_flashConfig_ln882h.c
	${OBK_SRCS}hal/ln882h/hal_flashVars_ln882h.c
	${OBK_SRCS}hal/ln882h/hal_generic_ln882h.c
	${OBK_SRCS}hal/ln882h/hal_main_ln882h.c
	${OBK_SRCS}hal/ln882h/hal_pins_ln882h.c
	${OBK_SRCS}hal/ln882h/hal_wifi_ln882h.c
	${OBK_SRCS}hal/ln882h/hal_ota_ln882h.c
	main.c
	usr_app.c
	bsp/serial_hw.c
	startup/startup_${CHIP_SERIAL}_gcc.c
	${MODULE_SRC}
	${COMP_LWIP_DIR}/apps/mqtt/mqtt.c
	${MCU_LN882X_DIR}/driver_${CHIP_SERIAL}/hal/hal_dma.c
	${MCU_LN882X_DIR}/driver_${CHIP_SERIAL}/hal/hal_spi.c
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

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-undef -Wno-unused-parameter -Wno-pedantic -Wno-float-conversion")
