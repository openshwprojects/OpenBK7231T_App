OBK_SRCS = ../shared/src/

SRCS += $(OBK_SRCS)hal/xr809/hal_adc_xr809
SRCS += $(OBK_SRCS)hal/xr809/hal_wifi_xr809
SRCS += $(OBK_SRCS)hal/xr809/hal_generic_xr809
SRCS += $(OBK_SRCS)hal/xr809/hal_main_xr809
SRCS += $(OBK_SRCS)hal/xr809/hal_pins_xr809
SRCS += $(OBK_SRCS)hal/xr809/hal_flashConfig_xr809
SRCS += $(OBK_SRCS)hal/xr809/hal_flashVars_xr809

include ../shared/platforms/obk_main.mk
SRCS += $(OBKM_SRC:.c=)
CC_FLAGS += $(OBK_CFLAGS)
