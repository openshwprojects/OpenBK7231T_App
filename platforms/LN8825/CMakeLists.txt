add_compile_definitions(PLATFORM_LN8825=1)

if($ENV{APP_VERSION} MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+$")
    string(REPLACE "." ";" VERSION_LIST "$ENV{APP_VERSION}")
    list(GET VERSION_LIST 1 VER_MAJOR)
    list(GET VERSION_LIST 2 VER_MINOR)
else()
    set(VER_MAJOR 0)
    set(VER_MINOR 1)
endif()

add_definitions(
	-DUSER_SW_VER="$ENV{APP_VERSION}"
	-DOBK_VARIANT=$ENV{OBK_VARIANT}
	-DWRAP_PRINTF
)

set(PRO_DIR  ${CMAKE_CURRENT_SOURCE_DIR})
set(OBK_SRCS "app/src/")
include(app/platforms/obk_main.cmake)
set(BERRY_SRCPATH "app/libraries/berry/src")
set(BERRY_MODULEPATH "${OBK_SRCS}berry/modules")
include_directories(${BERRY_SRCPATH})
include_directories(app/include)
include(app/libraries/berry.cmake)

set(PROJ_ALL_SRC
	${OBK_SRCS}hal/ln882h/hal_adc_ln882h.c
	${OBK_SRCS}hal/ln882h/hal_flashConfig_ln882h.c
	${OBK_SRCS}hal/ln882h/hal_flashVars_ln882h.c
	${OBK_SRCS}hal/ln882h/hal_generic_ln882h.c
	${OBK_SRCS}hal/ln882h/hal_main_ln882h.c
	${OBK_SRCS}hal/ln882h/hal_pins_ln882h.c
	${OBK_SRCS}hal/ln882h/hal_wifi_ln882h.c
	${OBK_SRCS}hal/ln882h/hal_ota_ln882h.c
	${OBKM_SRC}
	${OBKM_SRC_CXX}
	${BERRY_SRC_C}
	${PRO_DIR}/main.c
	${PRO_DIR}/usr_app.c
	${PRO_DIR}/bsp/serial_hw.c
	${PRO_DIR}/bsp/drv_adc_measure.c
	${PRO_DIR}/startup/startup_ln882x_gcc.c
	${CMAKE_SOURCE_DIR}/components/libc/newlib_stub.c
	${CMAKE_SOURCE_DIR}/components/atcmd/console.c
	${COMP_LWIP_DIR}/apps/mqtt/mqtt.c
)

link_directories(${CMAKE_SOURCE_DIR}/lib/gcclib)

set(pro_executable_target  ${TARGET_ELF_NAME})

add_executable(${pro_executable_target}  ${PROJ_ALL_SRC})

set(COMP_DHCPD_SUPPORT          ON  PARENT_SCOPE)
set(COMP_LWIP_SUPPORT           ON  PARENT_SCOPE)

# NOTE: wifi library must be placed before utils library.
target_link_libraries(${pro_executable_target}
	PUBLIC
	components::net::lwip
	components::net::dhcpd
	wifi
	components::kernel
	components::utils
	components::serial
	components::phony
	components::fs::kv
	components::fs::nvds
	components::fs::partition_mgr
	#components::atcmd
	components::fota
	driver_ln882x
	-lc -lm -lnosys

	PRIVATE
	-T${LINKER_SCRIPT}
	${EXTRA_LINK_FLAGS}
)

target_include_directories(${pro_executable_target}
	PRIVATE
	${CMAKE_SOURCE_DIR}/project/${USER_PROJECT}/bsp
	${CMAKE_SOURCE_DIR}/project/${USER_PROJECT}/app
	${CMAKE_SOURCE_DIR}/project/${USER_PROJECT}/cfg
	${CMAKE_SOURCE_DIR}/components/wifi
	${MCU_LN882X_DIR}/driver_ln882x/inc/reg
)

include(${CMAKE_SOURCE_DIR}/project/${USER_PROJECT}/gcc/gcc-custom-build-stage.cmake)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-undef -Wno-unused-parameter -Wno-pedantic -Wno-float-conversion")