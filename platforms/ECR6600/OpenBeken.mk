CFLAGS += -DPLATFORM_ECR6600 -Wno-error -DTCP_MSL=3000
VPATH += $(TOPDIR)/../../src
VPATH += $(TOPDIR)/../../platforms/ECR6600

INCPATHS += $(TOPDIR)/../../src
INCPATHS += $(TOPDIR)/include/ota
INCPATHS += $(TOPDIR)/include
INCPATHS += $(TOPDIR)

CSRCS   = hal/ecr6600/hal_adc_ecr6600.c
CSRCS  += hal/ecr6600/hal_flashConfig_ecr6600.c
CSRCS  += hal/ecr6600/hal_flashVars_ecr6600.c
CSRCS  += hal/ecr6600/hal_generic_ecr6600.c
CSRCS  += hal/ecr6600/hal_main_ecr6600.c
CSRCS  += hal/ecr6600/hal_pins_ecr6600.c
CSRCS  += hal/ecr6600/hal_wifi_ecr6600.c
CSRCS  += hal/ecr6600/hal_uart_ecr6600.c
CSRCS  += hal/ecr6600/hal_ota_ecr6600.c

OBK_SRCS = 
include $(TOPDIR)/../../platforms/obk_main.mk
CSRCS += $(OBKM_SRC)
CFLAGS += $(OBK_CFLAGS)

VPATH += $(TOPDIR)/../../libraries
INCPATHS += $(TOPDIR)/../../include
BERRY_MODULEPATH = berry/modules
BERRY_SRCPATH = berry/src
INCPATHS += $(TOPDIR)/../../libraries/$(BERRY_SRCPATH)
include $(TOPDIR)/../../libraries/berry.mk

CSRCS += $(BERRY_SRC_C)
