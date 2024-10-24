TOP_DIR = ../../..
sinclude $(TOP_DIR)/tools/w800/conf.mk

EXCLUDES = fsdata_ap_config.c \
           fsdata.c
CSRCS = $(filter-out $(EXCLUDES), $(wildcard *.c))

ifndef PDIR
GEN_LIBS = libweb$(LIB_EXT)
endif

#DEFINES +=

sinclude $(TOP_DIR)/tools/w800/rules.mk

INCLUDES += -I ./
INCLUDES := $(INCLUDES) -I $(PDIR)include
PDIR := ../$(PDIR)
sinclude $(PDIR)Makefile