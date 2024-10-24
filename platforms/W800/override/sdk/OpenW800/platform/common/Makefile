TOP_DIR = ../..
sinclude $(TOP_DIR)/tools/w800/conf.mk

ifndef PDIR
GEN_LIBS = libwmcommon$(LIB_EXT)
COMPONENTS_libwmcommon = crypto/libcrypto$(LIB_EXT) 	\
					     fwup/libfwup$(LIB_EXT)	\
					     mem/libmem$(LIB_EXT)	\
					     params/libparams$(LIB_EXT)	\
					     task/libtask$(LIB_EXT)	\
					     utils/libutils$(LIB_EXT)
endif

#DEFINES +=

sinclude $(TOP_DIR)/tools/w800/rules.mk
INCLUDES := $(INCLUDES) -I $(PDIR)include
PDIR := ../$(PDIR)
sinclude $(PDIR)Makefile