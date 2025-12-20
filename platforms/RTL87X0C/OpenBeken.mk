OBK_DIR = ../../../../..

CFLAGS += -DPLATFORM_RTL87X0C -DPLATFORM_REALTEK

INCLUDES += -I$(OBK_DIR)/libraries/easyflash/inc

SRC_C  += ../../../../../platforms/RTL87X0C/main.c

#SRC_C  += $(OBK_DIR)/src/hal/realtek/rtl87x0c/hal_adc_rtl87x0c.c
#SRC_C  += $(OBK_DIR)/src/hal/realtek/rtl87x0c/hal_main_rtl87x0c.c
SRC_C  += $(OBK_DIR)/src/hal/realtek/rtl87x0c/hal_pins_rtl87x0c.c
SRC_C  += $(OBK_DIR)/src/hal/realtek/rtl87x0c/hal_uart_rtl87x0c.c
SRC_C  += $(OBK_DIR)/src/hal/realtek/hal_flashConfig_realtek.c
SRC_C  += $(OBK_DIR)/src/hal/realtek/hal_flashVars_realtek.c
SRC_C  += $(OBK_DIR)/src/hal/realtek/hal_generic_realtek.c
SRC_C  += $(OBK_DIR)/src/hal/realtek/hal_pins_realtek.c
SRC_C  += $(OBK_DIR)/src/hal/realtek/hal_wifi_realtek.c
SRC_C  += $(OBK_DIR)/src/hal/realtek/hal_ota_realtek.c

OBK_SRCS = $(OBK_DIR)/src/
include $(OBK_DIR)/platforms/obk_main.mk
SRC_C += $(OBKM_SRC)
SRC_CPP += $(OBKM_SRC_CXX)
CFLAGS += $(OBK_CFLAGS)
CPPFLAGS += $(INCLUDES) -fpermissive

SRC_C += $(OBK_DIR)/libraries/easyflash/ports/ef_port.c
SRC_C += $(OBK_DIR)/libraries/easyflash/src/easyflash.c
#SRC_C += $(OBK_DIR)/libraries/easyflash/src/ef_cmd.c
SRC_C += $(OBK_DIR)/libraries/easyflash/src/ef_env.c
SRC_C += $(OBK_DIR)/libraries/easyflash/src/ef_env_legacy.c
SRC_C += $(OBK_DIR)/libraries/easyflash/src/ef_env_legacy_wl.c
SRC_C += $(OBK_DIR)/libraries/easyflash/src/ef_iap.c
SRC_C += $(OBK_DIR)/libraries/easyflash/src/ef_log.c
SRC_C += $(OBK_DIR)/libraries/easyflash/src/ef_utils.c

INCLUDES += -I$(OBK_DIR)/include
BERRY_MODULEPATH = $(OBK_DIR)/src/berry/modules
BERRY_SRCPATH = $(OBK_DIR)/libraries/berry/src

include $(OBK_DIR)/libraries/berry.mk

SRC_C += $(BERRY_SRC_C)
