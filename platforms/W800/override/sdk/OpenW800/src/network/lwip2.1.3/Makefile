TOP_DIR = ../../..
sinclude $(TOP_DIR)/tools/w800/conf.mk

ifndef PDIR
GEN_LIBS = liblwip$(LIB_EXT)
COMPONENTS_liblwip = api/libapi$(LIB_EXT) 	\
					 core/libcore$(LIB_EXT)	\
					 netif/libnetif$(LIB_EXT)
endif

#DEFINES +=

sinclude $(TOP_DIR)/tools/w800/rules.mk

INCLUDES := $(INCLUDES) -I $(PDIR)include
PDIR := ../$(PDIR)
sinclude $(PDIR)Makefile