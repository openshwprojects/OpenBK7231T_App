TOP_DIR = ../../../..
sinclude $(TOP_DIR)/tools/w800/conf.mk

EXCLUDES = timers.c

CSRCS = $(filter-out $(EXCLUDES), $(wildcard *.c))

ifndef PDIR
GEN_LIBS = libcore$(LIB_EXT)
COMPONENTS_libcore = ipv4/libipv4$(LIB_EXT) 	\
					 ipv6/libipv6$(LIB_EXT)
endif

#DEFINES +=

sinclude $(TOP_DIR)/tools/w800/rules.mk

INCLUDES := $(INCLUDES) -I $(PDIR)include
PDIR := ../$(PDIR)
sinclude $(PDIR)Makefile