TOP_DIR = ../../..
sinclude $(TOP_DIR)/tools/w800/conf.mk

EXCLUDES = minihuf.c \
			minilex.c \
			rewrite.c \
			server.c \
			server-handshake.c \
			sha-1.c \
			smtp.c \
			ssl-http2.c \
			ssl-server.c \
			alloc.c \
			daemonize.c \
			extension.c \
			extension-permessage-deflate.c \
			getifaddrs.c \
			hpack.c \
			http2.c \
			lejp.c \
			lejp-conf.c \
			libev.c \
			libuv.c \
			lws-plat-mbed3.c			

CSRCS = $(filter-out $(EXCLUDES), $(wildcard *.c))


ifndef PDIR
GEN_LIBS = libwebsockets$(LIB_EXT)
endif

#DEFINES +=

sinclude $(TOP_DIR)/tools/w800/rules.mk

INCLUDES := $(INCLUDES) -I $(PDIR)include
PDIR := ../$(PDIR)
sinclude $(PDIR)Makefile