TOP_DIR = ..

$(info    TOP_DIR is $(TOP_DIR)) 
sinclude $(TOP_DIR)/tools/w800/conf.mk

ifndef PDIR
GEN_LIBS = libuser$(LIB_EXT)
endif

#DEFINES +=
EXTRA_CCFLAGS += -DPLATFORM_W800

_SHARED_APP = $(TOP_DIR)/sharedAppContainer/sharedApp/src

$(info    _SHARED_APP is $(_SHARED_APP)) 

CSRCS += $(_SHARED_APP)/hal/w800/hal_adc_w800.c
CSRCS += $(_SHARED_APP)/hal/w800/hal_flashConfig_w800.c
CSRCS += $(_SHARED_APP)/hal/w800/hal_flashVars_w800.c
CSRCS += $(_SHARED_APP)/hal/w800/hal_generic_w800.c
CSRCS += $(_SHARED_APP)/hal/w800/hal_main_w800.c
CSRCS += $(_SHARED_APP)/hal/w800/hal_pins_w800.c
CSRCS += $(_SHARED_APP)/hal/w800/hal_wifi_w800.c
CSRCS += $(_SHARED_APP)/hal/w800/hal_uart_w800.c
CSRCS += $(_SHARED_APP)/hal/w800/hal_ota_w800.c

OBK_SRCS = $(_SHARED_APP)/
include $(_SHARED_APP)/../platforms/obk_main.mk
CSRCS += $(OBKM_SRC)
EXTRA_CCFLAGS += $(OBK_CFLAGS)

INCLUDES += -I$(_SHARED_APP)/../libraries/easyflash/inc
CSRCS += $(_SHARED_APP)/../libraries/easyflash/ports/ef_port.c
CSRCS += $(_SHARED_APP)/../libraries/easyflash/src/easyflash.c
CSRCS += $(_SHARED_APP)/../libraries/easyflash/src/ef_env.c
CSRCS += $(_SHARED_APP)/../libraries/easyflash/src/ef_env_legacy.c
CSRCS += $(_SHARED_APP)/../libraries/easyflash/src/ef_env_legacy_wl.c
CSRCS += $(_SHARED_APP)/../libraries/easyflash/src/ef_iap.c
CSRCS += $(_SHARED_APP)/../libraries/easyflash/src/ef_log.c
CSRCS += $(_SHARED_APP)/../libraries/easyflash/src/ef_utils.c

INCLUDES += -I$(_SHARED_APP)/../include
BERRY_MODULEPATH = $(_SHARED_APP)/berry/modules
BERRY_SRCPATH = $(_SHARED_APP)/../libraries/berry/src
include $(_SHARED_APP)/../libraries/berry.mk

CSRCS += $(BERRY_SRC_C)

CSRCS += $(TOP_DIR)/src/network/lwip2.0.3/apps/mqtt/mqtt.c

sinclude $(TOP_DIR)/tools/w800/rules.mk

INCLUDES := $(INCLUDES) -I $(PDIR)include

PDIR := ../$(PDIR)
sinclude $(PDIR)Makefile
