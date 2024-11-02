
# Component Makefile
#
COMPONENT_ADD_INCLUDEDIRS += include portable/GCC/RISC-V portable/GCC/RISC-V/chip_specific_extensions/RV32I_CLINT_no_extensions panic misaligned

COMPONENT_OBJS := $(patsubst %.c,%.o, \
                    event_groups.c \
                    list.c \
                    queue.c \
                    stream_buffer.c \
                    tasks.c \
                    timers.c \
                    panic/panic_c.c \
                    portable/GCC/RISC-V/port.c \
                    portable/GCC/RISC-V/port_stat_trap.c \
                    portable/GCC/RISC-V/portASM.S \
                    portable/MemMang/heap_5.c)

COMPONENT_OBJS := $(patsubst %.S,%.o, $(COMPONENT_OBJS))

COMPONENT_SRCDIRS := . portable portable/GCC/RISC-V portable/MemMang misaligned panic

OPT_FLAG_G := $(findstring -Og, $(CFLAGS))
ifeq ($(strip $(OPT_FLAG_G)),-Og)
CFLAGS := $(patsubst -Og,-O2,$(CFLAGS))
endif
OPT_FLAG_S := $(findstring -Os, $(CFLAGS))
ifeq ($(strip $(OPT_FLAG_S)),-Os)
CFLAGS := $(patsubst -Os,-O2,$(CFLAGS))
endif

ASMFLAGS += -DportasmHANDLE_INTERRUPT=interrupt_entry
