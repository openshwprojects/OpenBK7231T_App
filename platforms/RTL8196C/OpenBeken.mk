OBK_DIR = ../../..

CFLAGS += -DPLATFORM_RTL8196C=1

SRC_C += $(OBK_DIR)/src/hal/rtl8196c/hal_flashConfig_rtl8196c.c
SRC_C += $(OBK_DIR)/src/hal/rtl8196c/hal_flashVars_rtl8196c.c
SRC_C += $(OBK_DIR)/src/hal/rtl8196c/hal_generic_rtl8196c.c
SRC_C += $(OBK_DIR)/src/hal/rtl8196c/hal_main_rtl8196c.c
SRC_C += $(OBK_DIR)/src/hal/rtl8196c/hal_ota_rtl8196c.c
SRC_C += $(OBK_DIR)/src/hal/rtl8196c/hal_pins_rtl8196c.c
SRC_C += $(OBK_DIR)/src/hal/rtl8196c/hal_uart_rtl8196c.c
SRC_C += $(OBK_DIR)/src/hal/rtl8196c/hal_vfs_rtl8196c.c
SRC_C += $(OBK_DIR)/src/hal/rtl8196c/hal_wifi_rtl8196c.c

OBK_SRCS = $(OBK_DIR)/src/
include $(OBK_DIR)/platforms/obk_main.mk
SRC_C += $(OBKM_SRC)
CFLAGS += $(OBK_CFLAGS)

BERRY_MODULEPATH = $(OBK_SRCS)berry/modules
BERRY_SRCPATH = $(OBK_DIR)/libraries/berry/src
CFLAGS += -I$(BERRY_SRCPATH) -I$(OBK_DIR)/include
include $(OBK_DIR)/libraries/berry.mk
SRC_C += $(BERRY_SRC_C)
