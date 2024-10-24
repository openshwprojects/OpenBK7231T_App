TOP_DIR = ../../../..
sinclude $(TOP_DIR)/tools/w800/conf.mk

ifndef PDIR
GEN_LIBS = libnimble$(LIB_EXT)
COMPONENTS_libnimble = host/libhost$(LIB_EXT) transport/libtransport$(LIB_EXT)
endif

#DEFINES +=

sinclude $(TOP_DIR)/tools/w800/rules.mk

INCLUDES := $(INCLUDES) -I ./ -I $(PDIR)include
PDIR := ../$(PDIR)
sinclude $(PDIR)Makefile