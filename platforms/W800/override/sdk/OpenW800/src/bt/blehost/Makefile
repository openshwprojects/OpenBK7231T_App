TOP_DIR = ../../..
sinclude $(TOP_DIR)/tools/w800/conf.mk

ifndef PDIR
GEN_LIBS = libblehost$(LIB_EXT)
COMPONENTS_libblehost = ext/libext$(LIB_EXT) \
							nimble/libnimble$(LIB_EXT) \
							porting/libporting$(LIB_EXT)
endif

#DEFINES +=

sinclude $(TOP_DIR)/tools/w800/rules.mk

INCLUDES := $(INCLUDES) -I $(PDIR)include \
			-I $(PDIR)nimble/host/include \
			-I $(PDIR)nimble/host/mesh/include \
			-I $(PDIR)nimble/host/services/gap/include \
			-I $(PDIR)nimble/host/services/gatt/include \
			-I $(PDIR)nimble/host/config/include \
			-I $(PDIR)nimble/host/ram/include \
			-I $(PDIR)nimble/host/util/include \
			-I $(PDIR)nimble/include \
			-I $(PDIR)nimble/transport/uart/include \
			-I $(PDIR)porting/w800/include \
			-I $(PDIR)ext/tinycrypt/include
			
INCLUDES := $(INCLUDES) -I ./ -I $(PDIR)include
PDIR := ../$(PDIR)
sinclude $(PDIR)Makefile