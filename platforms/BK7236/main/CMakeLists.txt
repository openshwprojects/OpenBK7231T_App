set(incs  $ENV{ARMINO_PATH}/../../include)
set(srcs)
set(OBK_SRCS "$ENV{ARMINO_PATH}/../../src/")
include($ENV{ARMINO_PATH}/../../platforms/obk_main.cmake)

set(OBKFLAGS -DPLATFORM_ARMINO=1 -DPLATFORM_BK7236=1)

if(DEFINED ENV{APP_VERSION})
	set(OBKFLAGS ${OBKFLAGS} -DUSER_SW_VER="$ENV{APP_VERSION}")
endif()
if(DEFINED ENV{OBK_VARIANT})
	set(OBKFLAGS ${OBKFLAGS} -DOBK_VARIANT=$ENV{OBK_VARIANT})
endif()

list(APPEND srcs
	${OBKM_SRC}
	${OBKM_SRC_CXX}
	${BERRY_SRC_C}
	${OBK_SRCS}hal/bk7231/hal_main_bk7231.c
	${OBK_SRCS}hal/bk7231/hal_flashVars_bk7231.c
	${OBK_SRCS}hal/bk_armino/hal_generic_bk_armino.c
	${OBK_SRCS}hal/bk_armino/hal_hwtimer_bk_armino.c
	${OBK_SRCS}hal/bk_armino/hal_main_bk_armino.c
	${OBK_SRCS}hal/bk_armino/hal_flashConfig_bk_armino.c
	${OBK_SRCS}hal/bk_armino/hal_pins_bk_armino.c
	${OBK_SRCS}hal/bk_armino/hal_ota_bk_armino.c
	${OBK_SRCS}hal/bk_armino/hal_wifi_bk_armino.c
	${OBK_SRCS}hal/bk_armino/hal_uart_bk_armino.c
	$ENV{ARMINO_PATH}/../../libraries/mqtt_patched.c
)
armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}")
armino_build_set_property(COMPILE_OPTIONS "${OBKFLAGS}" APPEND)
armino_build_set_property(LINK_OPTIONS "-Wl,--wrap=vApplicationIdleHook" APPEND)

