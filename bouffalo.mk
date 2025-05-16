# Component Makefile
#
## These include paths would be exported to project level
COMPONENT_ADD_INCLUDEDIRS += src/ src/httpserver/ src/cmnds/ src/logging/ src/hal/bl602/ src/mqtt/ src/cJSON src/base64 src/driver src/devicegroups src/bitmessage src/littlefs libraries/berry/src include/

## not be exported to project level
COMPONENT_PRIV_INCLUDEDIRS :=

CFLAGS += -DOBK_VARIANT=${OBK_VARIANT}

## This component's src 
COMPONENT_SRCS := 

COMPONENT_OBJS := $(patsubst %.c,%.o, $(COMPONENT_SRCS))
COMPONENT_OBJS := $(patsubst %.S,%.o, $(COMPONENT_OBJS))

COMPONENT_SRCDIRS := src/ src/jsmn src/httpserver/ src/cmnds/ src/logging/ src/hal/bl602/ src/mqtt/ src/i2c/ src/cJSON src/base64 src/driver src/devicegroups src/bitmessage src/littlefs src/hal/generic libraries/berry/src src/berry src/berry/modules

COMPONENT_ADD_LDFLAGS = -Wl,--whole-archive -l$(COMPONENT_NAME) -Wl,--no-whole-archive
