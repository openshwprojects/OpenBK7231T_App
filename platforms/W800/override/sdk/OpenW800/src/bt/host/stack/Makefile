TOP_DIR = ../../../..
sinclude $(TOP_DIR)/tools/w800/conf.mk

ifndef PDIR
GEN_LIBS = libstack$(LIB_EXT)
COMPONENTS_libstack = a2dp/liba2dp$(LIB_EXT) 	\
					  avct/libavct$(LIB_EXT)      \
                      avdt/libavdt$(LIB_EXT) 	\
					  avrc/libavrc$(LIB_EXT)      \
                      bnep/libbnep$(LIB_EXT) 	\
					  btm/libbtm$(LIB_EXT)      \
                      btu/libbtu$(LIB_EXT) 	\
					  gap/libgap$(LIB_EXT)      \
                      gatt/libgatt$(LIB_EXT) 	\
					  hcic/libhcic$(LIB_EXT)      \
                      hid/libhid$(LIB_EXT) 	\
					  l2cap/libl2cap$(LIB_EXT)      \
                      mcap/libmcap$(LIB_EXT) \
                      pan/libpan$(LIB_EXT) \
                      rfcomm/librfcomm$(LIB_EXT) \
                      sdp/libsdp$(LIB_EXT) \
                      smp/libsmp$(LIB_EXT) \
                      srvc/libsrvc$(LIB_EXT)
endif

#DEFINES +=

sinclude $(TOP_DIR)/tools/w800/rules.mk

INCLUDES := $(INCLUDES) -I $(PDIR)include
PDIR := ../$(PDIR)
sinclude $(PDIR)Makefile