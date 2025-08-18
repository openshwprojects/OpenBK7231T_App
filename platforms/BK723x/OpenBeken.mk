OBK_DIR = ./app

INCLUDES += -I$(OBK_DIR)/../include
INCLUDES += -I./fixes

CCFLAGS += -DPLATFORM_BEKEN -DPLATFORM_BEKEN_NEW

ifeq ($(CFG_SOC_NAME), 1)
CCFLAGS += -DPLATFORM_BK7231T
else ifeq ($(CFG_SOC_NAME), 2)
ifeq ($(SOC_BK7231T), 1)
CCFLAGS += -DPLATFORM_BK7231T
else
CCFLAGS += -DPLATFORM_BK7231U
endif
else ifeq ($(CFG_SOC_NAME), 3)
CCFLAGS += -DPLATFORM_BK7252
else ifeq ($(CFG_SOC_NAME), 5)
CCFLAGS += -DPLATFORM_BK7231N
else ifeq ($(CFG_SOC_NAME), 7)
CCFLAGS += -DPLATFORM_BK7238
else ifeq ($(CFG_SOC_NAME), 8)
CCFLAGS += -DPLATFORM_BK7252N
endif

SRC_C += ./fixes/blank.c
APP_C += $(OBK_DIR)/../platforms/BK723x/ps.c

APP_C += $(OBK_DIR)/hal/bk7231/hal_adc_bk7231.c
APP_C += $(OBK_DIR)/hal/bk7231/hal_flashConfig_bk7231.c
APP_C += $(OBK_DIR)/hal/bk7231/hal_flashVars_bk7231.c
APP_C += $(OBK_DIR)/hal/bk7231/hal_generic_bk7231.c
APP_C += $(OBK_DIR)/hal/bk7231/hal_main_bk7231.c
APP_C += $(OBK_DIR)/hal/bk7231/hal_pins_bk7231.c
APP_C += $(OBK_DIR)/hal/bk7231/hal_wifi_bk7231.c
APP_C += $(OBK_DIR)/hal/bk7231/hal_uart_bk7231.c
APP_C += $(OBK_DIR)/hal/bk7231/hal_ota_bk7231.c

OBK_SRCS = $(OBK_DIR)/
include $(OBK_DIR)/../platforms/obk_main.mk
APP_C += $(OBKM_SRC)
APP_CXX += $(OBKM_SRC_CXX)
CCFLAGS += $(OBK_CFLAGS)


BERRY_MODULEPATH = $(OBK_DIR)/berry/modules
BERRY_SRCPATH = $(OBK_DIR)/../libraries/berry/src
include $(OBK_DIR)/../libraries/berry.mk

APP_C += $(BERRY_SRC_C)
