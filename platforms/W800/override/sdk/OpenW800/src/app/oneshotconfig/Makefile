TOP_DIR = ../../..
sinclude $(TOP_DIR)/tools/w800/conf.mk



CSRCS = $(filter-out $(EXCLUDES), $(wildcard *.c))

ifndef PDIR
GEN_LIBS = liboneshotconfig$(LIB_EXT)
endif

#DEFINES +=

sinclude $(TOP_DIR)/tools/w800/rules.mk

INCLUDES := $(INCLUDES) -I $(PDIR)include
PDIR := ../$(PDIR)
sinclude $(PDIR)Makefile