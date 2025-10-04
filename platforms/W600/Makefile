
#############################################################
# Required variables for each makefile
# Discard this section from all parent makefiles
# Expected variables (with automatic defaults):
#   CSRCS (all "C" files in the dir)
#   SUBDIRS (all subdirs with a Makefile)
#   GEN_LIBS - list of libs to be generated ()
#   GEN_IMAGES - list of images to be generated ()
#   COMPONENTS_xxx - a list of libs/objs in the form
#     subdir/lib to be extracted and rolled up into
#     a generated lib/image xxx.a ()
#

TOP_DIR = ..
sinclude $(TOP_DIR)/tools/tool_chain.def

ifndef PDIR
GEN_LIBS = libuser$(LIB_EXT)
endif

_SHARED_APP = $(TOP_DIR)/sharedAppContainer/sharedApp/src

CSRCS += $(_SHARED_APP)/hal/w800/hal_adc_w800.c
CSRCS += $(_SHARED_APP)/hal/w800/hal_flashConfig_w800.c
CSRCS += $(_SHARED_APP)/hal/w800/hal_flashVars_w800.c
CSRCS += $(_SHARED_APP)/hal/w800/hal_generic_w800.c
CSRCS += $(_SHARED_APP)/hal/w800/hal_main_w800.c
CSRCS += $(_SHARED_APP)/hal/w800/hal_pins_w800.c
CSRCS += $(_SHARED_APP)/hal/w800/hal_wifi_w800.c
CSRCS += $(_SHARED_APP)/hal/w800/hal_ota_w800.c
CSRCS += $(_SHARED_APP)/hal/w800/hal_uart_w800.c
CSRCS += main.c

OBK_SRCS = $(_SHARED_APP)/
include $(_SHARED_APP)/../platforms/obk_main.mk
CSRCS += $(OBKM_SRC)
EXTRA_CCFLAGS += $(OBK_CFLAGS)

CSRCS += $(TOP_DIR)/src/network/lwip2.0.3/apps/mqtt/mqtt.c


#############################################################
# Configuration i.e. compile options etc.
# Target specific stuff (defines etc.) goes in here!
# Generally values applying to a tree are captured in the
#   makefile at its root level - these are then overridden
#   for a subtree within the makefile rooted therein
#
#DEFINES +=

#############################################################
# Recursion Magic - Don't touch this!!
#
# Each subtree potentially has an include directory
#   corresponding to the common APIs applicable to modules
#   rooted at that subtree. Accordingly, the INCLUDE PATH
#   of a module can only contain the include directories up
#   its parent path, and not its siblings
#
# Required for each makefile to inherit from the parent
#

sinclude $(TOP_DIR)/tools/rules.mk

INCLUDES := $(INCLUDES) -I $(PDIR)include

PDIR := ../$(PDIR)
sinclude $(PDIR)Makefile


