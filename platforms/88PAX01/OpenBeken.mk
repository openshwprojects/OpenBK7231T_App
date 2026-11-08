OBK_DIR = ../../..

CFLAGS += -DPLATFORM_88PAX01=1

SRC_C += $(OBK_DIR)/src/hal/88pax01/hal_flashConfig_88pax01.c
SRC_C += $(OBK_DIR)/src/hal/88pax01/hal_flashVars_88pax01.c
SRC_C += $(OBK_DIR)/src/hal/88pax01/hal_generic_88pax01.c
SRC_C += $(OBK_DIR)/src/hal/88pax01/hal_main_88pax01.c
SRC_C += $(OBK_DIR)/src/hal/88pax01/hal_ota_88pax01.c
SRC_C += $(OBK_DIR)/src/hal/88pax01/hal_pins_88pax01.c
SRC_C += $(OBK_DIR)/src/hal/88pax01/hal_uart_88pax01.c
SRC_C += $(OBK_DIR)/src/hal/88pax01/hal_wifi_88pax01.c

OBK_SRCS = $(OBK_DIR)/src/
include $(OBK_DIR)/platforms/obk_main.mk
SRC_C += $(OBKM_SRC)
CFLAGS += $(OBK_CFLAGS)
