TOP_DIR = ../../..
sinclude $(TOP_DIR)/tools/w800/conf.mk

ifndef PDIR
GEN_LIBS = libbthost_br_edr$(LIB_EXT)
COMPONENTS_libbthost_br_edr = bta/libbta$(LIB_EXT) 	    \
                       btcore/libbtcore$(LIB_EXT)	\
                       btif/libbtif$(LIB_EXT)     \
                       embdrv/libembdrv$(LIB_EXT) \
                       gki/libgki$(LIB_EXT)	    \
                       hci/libhci$(LIB_EXT)       \
                       main/libmain$(LIB_EXT)	    \
                       osi/libosi$(LIB_EXT)       \
                       stack/libstack$(LIB_EXT)
endif

#DEFINES +=

sinclude $(TOP_DIR)/tools/w800/rules.mk

INCLUDES := $(INCLUDES) -I $(PDIR)include \
			-I $(TOP_DIR)/src/bt/host \
			-I $(TOP_DIR)/src/bt/host/main \
			-I $(TOP_DIR)/src/bt/host/include \
			-I $(TOP_DIR)/src/bt/host/stack \
			-I $(TOP_DIR)/src/bt/host/stack/include \
			-I $(TOP_DIR)/src/bt/host/gki/wm \
			-I $(TOP_DIR)/src/bt/host/gki/common \
			-I $(TOP_DIR)/src/bt/host/vnd/include \
			-I $(TOP_DIR)/src/bt/host/bta/include \
			-I $(TOP_DIR)/src/bt/host/osi/include \
			-I $(TOP_DIR)/src/bt/host/hci/include \
			-I $(TOP_DIR)/src/bt/host/btif/include \
			-I $(TOP_DIR)/src/bt/host/embdrv/sbc/encoder/include \
			-I $(TOP_DIR)/src/bt/host/embdrv/sbc/decoder/include \
			-I $(TOP_DIR)/src/bt/host/btcore/include

PDIR := ../$(PDIR)
sinclude $(PDIR)Makefile