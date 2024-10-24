TOP_DIR = ../../..
sinclude $(TOP_DIR)/tools/w800/conf.mk

EXCLUDES = wm_httpclient_if.c

CSRCS = $(filter-out $(EXCLUDES), $(wildcard *.c))

ifndef PDIR
GEN_LIBS = libhttpclient$(LIB_EXT)
endif

#DEFINES +=

sinclude $(TOP_DIR)/tools/w800/rules.mk
INCLUDES := $(INCLUDES) -I $(PDIR)include
PDIR := ../$(PDIR)
sinclude $(PDIR)Makefile