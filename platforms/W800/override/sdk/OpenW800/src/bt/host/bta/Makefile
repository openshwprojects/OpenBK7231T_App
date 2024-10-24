TOP_DIR = ../../../..
sinclude $(TOP_DIR)/tools/w800/conf.mk

ifndef PDIR
GEN_LIBS = libbta$(LIB_EXT)
COMPONENTS_libbta = ag/libag$(LIB_EXT) 	\
					ar/libar$(LIB_EXT)      \
                    av/libav$(LIB_EXT) 	\
					dm/libdm$(LIB_EXT)      \
                    gatt/libgatt$(LIB_EXT) 	\
					hf_client/libhf_client$(LIB_EXT)      \
                    hh/libhh$(LIB_EXT) 	\
					hl/libhl$(LIB_EXT)      \
                    jv/libjv$(LIB_EXT) 	\
					mce/libmce$(LIB_EXT)      \
                    pan/libpan$(LIB_EXT) 	\
					sdp/libsdp$(LIB_EXT)      \
                    sys/libbthsys$(LIB_EXT)
endif

#DEFINES +=

sinclude $(TOP_DIR)/tools/w800/rules.mk

INCLUDES := $(INCLUDES) -I $(PDIR)include
PDIR := ../$(PDIR)
sinclude $(PDIR)Makefile