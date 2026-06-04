OBK_SRCS = ../shared/src/
CC_FLAGS += -DPLATFORM_XRADIO
CXX_FLAGS += -DPLATFORM_XRADIO

SRCS += $(OBK_SRCS)hal/xradio/hal_adc_xradio
SRCS += $(OBK_SRCS)hal/xradio/hal_wifi_xradio
SRCS += $(OBK_SRCS)hal/xradio/hal_generic_xradio
SRCS += $(OBK_SRCS)hal/xradio/hal_hwtimer_xradio
SRCS += $(OBK_SRCS)hal/xradio/hal_main_xradio
SRCS += $(OBK_SRCS)hal/xradio/hal_pins_xradio
SRCS += $(OBK_SRCS)hal/xradio/hal_flashConfig_xradio
SRCS += $(OBK_SRCS)hal/xradio/hal_flashVars_xradio
SRCS += $(OBK_SRCS)hal/xradio/hal_uart_xradio
SRCS += $(OBK_SRCS)hal/xradio/hal_ota_xradio
SRCS += $(OBK_SRCS)hal/xradio/xr809/hal_pins_xr809
#SRCS += $(OBK_SRCS)hal/xradio/xr806/hal_bt_proxy_xr806
SRCS += $(OBK_SRCS)hal/xradio/xr806/hal_pins_xr806
SRCS += $(OBK_SRCS)hal/xradio/xr872/hal_pins_xr872

include ../shared/platforms/obk_main.mk
SRCS += $(OBKM_SRC:.c=)
#needs sdk changes
ifeq ($(OPLATFORM),3)
#__CONFIG_CPLUSPLUS = y
SRCS += $(OBKM_SRC_CXX:.cpp=)
CXX_FLAGS += $(OBK_CFLAGS)
endif
CC_FLAGS += $(OBK_CFLAGS)

INCLUDE_PATHS += -I../shared/libraries/easyflash/inc

SRCS += ../shared/libraries/easyflash/ports/ef_port
SRCS += ../shared/libraries/easyflash/src/easyflash
SRCS += ../shared/libraries/easyflash/src/ef_env
SRCS += ../shared/libraries/easyflash/src/ef_utils

BERRY_MODULEPATH = $(OBK_SRCS)berry/modules
BERRY_SRCPATH = ../shared/libraries/berry/src
INCLUDE_PATHS += -I$(BERRY_SRCPATH)
INCLUDE_PATHS += -I../shared/include
include ../shared/libraries/berry.mk

SRCS += $(BERRY_SRC_C:.c=)
