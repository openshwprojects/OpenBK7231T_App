# Component Makefile
#
## These include paths would be exported to project level
COMPONENT_ADD_INCLUDEDIRS += src/ src/httpserver/ src/cmnds/ src/logging/ src/hal/bl602/ src/mqtt/

## not be exported to project level
COMPONENT_PRIV_INCLUDEDIRS :=							 


## This component's src 
COMPONENT_SRCS := 
                  

COMPONENT_OBJS := $(patsubst %.c,%.o, $(COMPONENT_SRCS))
COMPONENT_OBJS := $(patsubst %.S,%.o, $(COMPONENT_OBJS))

COMPONENT_SRCDIRS := src/ src/httpserver/ src/cmnds/ src/logging/ src/hal/bl602/ src/mqtt/




