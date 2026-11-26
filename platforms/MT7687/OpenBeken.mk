CFLAGS += -DPLATFORM_MT7687=1 -DTCP_MSL=3000 -DMTK_DEBUG_LEVEL_NONE

CFLAGS += -DWRAP_PRINTF
LDFLAGS += -Wl,-wrap=sprintf -Wl,-wrap=snprintf -Wl,-wrap=vsprintf -Wl,-wrap=vsnprintf

C_FILES  += $(SOURCE_DIR)/../libraries/mqtt_patched.c
C_FILES  += $(SOURCE_DIR)/../src/hal/mt7687/hal_flashConfig_mt7687.c
C_FILES  += $(SOURCE_DIR)/../src/hal/mt7687/hal_flashVars_mt7687.c
C_FILES  += $(SOURCE_DIR)/../src/hal/mt7687/hal_generic_mt7687.c
C_FILES  += $(SOURCE_DIR)/../src/hal/mt7687/hal_main_mt7687.c
C_FILES  += $(SOURCE_DIR)/../src/hal/mt7687/hal_ota_mt7687.c
C_FILES  += $(SOURCE_DIR)/../src/hal/mt7687/hal_pins_mt7687.c
C_FILES  += $(SOURCE_DIR)/../src/hal/mt7687/hal_wifi_mt7687.c

C_FILES += $(SOURCE_DIR)/../libraries/easyflash/ports/ef_port.c
C_FILES += $(SOURCE_DIR)/../libraries/easyflash/src/easyflash.c
C_FILES += $(SOURCE_DIR)/../libraries/easyflash/src/ef_env.c
C_FILES += $(SOURCE_DIR)/../libraries/easyflash/src/ef_utils.c

CFLAGS += -I$(SOURCE_DIR)/../../libraries/easyflash/inc

OBK_SRCS = $(SOURCE_DIR)/../src/
include $(SOURCE_DIR)/../../platforms/obk_main.mk
C_FILES += $(OBKM_SRC)
CFLAGS += $(OBK_CFLAGS)

CFLAGS += -I$(SOURCE_DIR)/../../include
BERRY_MODULEPATH = $(SOURCE_DIR)/../src/berry/modules
BERRY_SRCPATH = $(SOURCE_DIR)/../libraries/berry/src
CFLAGS += -I$(SOURCE_DIR)/../../libraries/berry/src
include $(SOURCE_DIR)/../../libraries/berry.mk

C_FILES += $(BERRY_SRC_C)
