# Component Makefile
#
## These include paths would be exported to project level
COMPONENT_ADD_INCLUDEDIRS += bl_os_adapter/include bl_os_adapter/include/bl_os_adapter bl_os_adapter
							 
## not be exported to project level
COMPONENT_PRIV_INCLUDEDIRS :=		 

## This component's src 
COMPONENT_SRCS := bl_os_adapter/bl_os_hal.c
				  
COMPONENT_OBJS := $(patsubst %.c,%.o, $(COMPONENT_SRCS))

COMPONENT_SRCDIRS := bl_os_adapter

CFLAGS   += -DCFG_FREERTOS 
##
#CPPFLAGS += 
