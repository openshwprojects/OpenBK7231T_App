TOP_DIR = ../../../../..
sinclude $(TOP_DIR)/tools/w800/conf.mk

ifndef PDIR
GEN_LIBS = libhost$(LIB_EXT)
COMPONENTS_libhost = mesh/libmesh$(LIB_EXT) \
						services/libservices$(LIB_EXT) \
						src/libwmhostsrc$(LIB_EXT) \
						store/libstore$(LIB_EXT) \
						util/libutil$(LIB_EXT)
endif

#DEFINES +=

sinclude $(TOP_DIR)/tools/w800/rules.mk

INCLUDES := $(INCLUDES) -I ./ -I $(PDIR)include
PDIR := ../$(PDIR)
sinclude $(PDIR)Makefile