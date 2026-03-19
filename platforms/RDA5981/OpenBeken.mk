OBK_DIR = ../../..

CC_FLAGS += -DPLATFORM_RDA5981=1 -DMBED_HEAP_STATS_ENABLED -DWRAP_PRINTF
CPPC_FLAGS += -DPLATFORM_RDA5981=1 -DMBED_HEAP_STATS_ENABLED -DWRAP_PRINTF

INCLUDE_PATHS += -I$(OBK_DIR)/libraries/easyflash/inc

SRC_C  += $(OBK_DIR)/src/hal/rda5981/hal_flashConfig_rda5981.c
SRC_C  += $(OBK_DIR)/src/hal/rda5981/hal_flashVars_rda5981.c
SRC_C  += $(OBK_DIR)/src/hal/rda5981/hal_generic_rda5981.c
SRC_C  += $(OBK_DIR)/src/hal/rda5981/hal_pins_rda5981.c
SRC_C  += $(OBK_DIR)/src/hal/rda5981/hal_ota_rda5981.c
SRC_C  += $(OBK_DIR)/src/hal/rda5981/hal_uart_rda5981.c
SRC_CPP += $(OBK_DIR)/src/hal/rda5981/hal_main_rda5981.cpp
SRC_CPP += $(OBK_DIR)/src/hal/rda5981/hal_wifi_rda5981.cpp

OBK_SRCS = $(OBK_DIR)/src/
include $(OBK_DIR)/platforms/obk_main.mk
SRC_C += $(OBKM_SRC)
#SRC_CPP += $(OBKM_SRC_CXX)
CC_FLAGS += $(OBK_CFLAGS)
CPPC_FLAGS += $(OBK_CFLAGS)
#CPPFLAGS += $(INCLUDES) -fpermissive

SRC_C += $(OBK_DIR)/libraries/easyflash/ports/ef_port.c
SRC_C += $(OBK_DIR)/libraries/easyflash/src/easyflash.c
#SRC_C += $(OBK_DIR)/libraries/easyflash/src/ef_cmd.c
SRC_C += $(OBK_DIR)/libraries/easyflash/src/ef_env.c
SRC_C += $(OBK_DIR)/libraries/easyflash/src/ef_env_legacy.c
SRC_C += $(OBK_DIR)/libraries/easyflash/src/ef_env_legacy_wl.c
SRC_C += $(OBK_DIR)/libraries/easyflash/src/ef_iap.c
SRC_C += $(OBK_DIR)/libraries/easyflash/src/ef_log.c
SRC_C += $(OBK_DIR)/libraries/easyflash/src/ef_utils.c

INCLUDE_PATHS += -I$(OBK_DIR)/include -I$(OBK_DIR)/libraries/berry/src
BERRY_MODULEPATH = $(OBK_DIR)/src/berry/modules
BERRY_SRCPATH = $(OBK_DIR)/libraries/berry/src

include $(OBK_DIR)/libraries/berry.mk

SRC_C += $(BERRY_SRC_C)

OBJECTS += $(SRC_C:.c=.o)
OBJECTS += $(SRC_CPP:.cpp=.o)

LD_FLAGS += -Wl,--wrap,vsnprintf -Wl,--wrap,snprintf -Wl,--wrap,sprintf -Wl,--wrap,vsprintf
