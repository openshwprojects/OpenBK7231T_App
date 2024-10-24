TOP_DIR = ../../..
sinclude $(TOP_DIR)/tools/w800/conf.mk

EXCLUDES = address.c \
			async.c \
			block.c \
			coap_io.c \
			coap_io_lwip.c \
			coap_time.c \
			debug.c \
			encode.c \
			hashkey.c \
			mem_libcoap.c \
			net.c \
			option.c \
			pdu.c \
			resource.c \
			sr.c \
			subscribe.c \
			uri_libcoap.c

CSRCS = $(filter-out $(EXCLUDES), $(wildcard *.c))

ifndef PDIR
GEN_LIBS = libcoap$(LIB_EXT)
endif

#DEFINES +=

sinclude $(TOP_DIR)/tools/w800/rules.mk

INCLUDES := $(INCLUDES) -I $(PDIR)include
PDIR := ../$(PDIR)
sinclude $(PDIR)Makefile