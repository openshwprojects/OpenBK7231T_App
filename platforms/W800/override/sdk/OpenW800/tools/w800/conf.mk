sinclude $(TOP_DIR)/tools/w800/.config
CONFIG_W800_USE_LIB ?= n
CONFIG_W800_FIRMWARE_DEBUG ?= n
CONFIG_ARCH_TYPE ?= w800
CONFIG_W800_TOOLCHAIN_PREFIX ?= csky-abiv2-elf
CONFIG_W800_USE_NIMBLE ?= n

TARGET ?= $(subst ",,$(CONFIG_W800_TARGET_NAME))

ODIR := $(TOP_DIR)/bin/build
OBJODIR := $(ODIR)/$(TARGET)/obj
UP_EXTRACT_DIR := ../../lib
LIBODIR := $(ODIR)/$(TARGET)/lib
IMAGEODIR := $(ODIR)/$(TARGET)/image
BINODIR := $(ODIR)/$(TARGET)/bin
FIRMWAREDIR := $(TOP_DIR)/bin
SDK_TOOLS := $(TOP_DIR)/tools/$(CONFIG_ARCH_TYPE)
CA_PATH := $(SDK_TOOLS)/ca
VER_TOOL ?= $(SDK_TOOLS)/wm_getver
WM_TOOL ?= $(SDK_TOOLS)/wm_tool

SECBOOT_HEADER_POS=8002000
SECBOOT_ADDRESS_POS=8002400
SEC_BOOT_BIN := $(SDK_TOOLS)/w800_secboot.bin
SEC_BOOT_IMG := $(SDK_TOOLS)/w800_secboot
SEC_BOOT := $(SDK_TOOLS)/w800_secboot.img

ifeq ($(MAKECMDGOALS),lib)
USE_LIB = 0
else ifeq ($(CONFIG_W800_USE_LIB),y)
USE_LIB = 1
else
USE_LIB = 0
endif

DL_PORT ?= $(subst ",,$(CONFIG_W800_DOWNLOAD_PORT))
DL_BAUD ?= $(CONFIG_W800_DOWNLOAD_RATE)

IMG_HEADER := $(CONFIG_W800_IMAGE_HEADER)
RUN_ADDRESS := $(CONFIG_W800_RUN_ADDRESS)
UPD_ADDRESS := $(CONFIG_W800_UPDATE_ADDRESS)

SIGNATURE := $(CONFIG_W800_IMAGE_SIGNATURE)
CODE_ENCRYPT := $(CONFIG_W800_CODE_ENCRYPT)

#real image type include basic image type, key number in chip, encrypt flag, signature flag, signature pubkey flag
IMG_TYPE :=$(shell expr $(CONFIG_W800_IMAGE_TYPE) + $(CONFIG_W800_PRIKEY_SEL) '*' 32 + $(CONFIG_W800_CODE_ENCRYPT) '*' 16 + $(CONFIG_W800_IMAGE_SIGNATURE) '*' 256)
SECBOOT_IMG_TYPE :=$(shell expr $(CONFIG_W800_SECBOOT_IMAGE_TYPE) + $(CONFIG_W800_IMAGE_SIGNATURE) '*' 256)

#when encrypt image info, real key number must be CONFIG_W800_PRIKEY_SEL plus 1
ifeq ($(CONFIG_W800_CODE_ENCRYPT), 0)
PRIKEY_SEL := $(CONFIG_W800_PRIKEY_SEL)
else
PRIKEY_SEL := $(shell expr $(CONFIG_W800_PRIKEY_SEL) + 1)
endif

optimization ?= -O2

ifeq ($(CONFIG_W800_FIRMWARE_DEBUG),y)
optimization += -g -DWM_SWD_ENABLE=1
endif

ifeq ($(CONFIG_W800_USE_NIMBLE),y)
USE_NIMBLE = 1
optimization += -DNIMBLE_FTR=1
else
USE_NIMBLE = 0
optimization += -DNIMBLE_FTR=0
endif

# YES; NO
VERBOSE ?= NO

UNAME_O:=$(shell uname -o)
UNAME_S:=$(shell uname -s)

$(shell gcc $(SDK_TOOLS)/wm_getver.c -Wall -O2 -o $(VER_TOOL))

TOOL_CHAIN_PREFIX = $(CONFIG_W800_TOOLCHAIN_PREFIX)
TOOL_CHAIN_PATH = $(subst ",,$(CONFIG_W800_TOOLCHAIN_PATH))

# select which tools to use as compiler, librarian and linker
ifeq ($(VERBOSE),YES)
    AR = $(TOOL_CHAIN_PATH)$(TOOL_CHAIN_PREFIX)-ar
    ASM = $(TOOL_CHAIN_PATH)$(TOOL_CHAIN_PREFIX)-gcc
    CC = $(TOOL_CHAIN_PATH)$(TOOL_CHAIN_PREFIX)-gcc
    CPP = $(TOOL_CHAIN_PATH)$(TOOL_CHAIN_PREFIX)-g++
    LINK = $(TOOL_CHAIN_PATH)$(TOOL_CHAIN_PREFIX)-gcc
    OBJCOPY = $(TOOL_CHAIN_PATH)$(TOOL_CHAIN_PREFIX)-objcopy
    OBJDUMP = $(TOOL_CHAIN_PATH)$(TOOL_CHAIN_PREFIX)-objdump
else
    AR      = @echo "AR       $(notdir $@)" 2>/dev/null; $(TOOL_CHAIN_PATH)$(TOOL_CHAIN_PREFIX)-ar
    ASM     = @echo "ASM      $<"; $(TOOL_CHAIN_PATH)$(TOOL_CHAIN_PREFIX)-gcc
    CC      = @echo "CC       $<"; $(TOOL_CHAIN_PATH)$(TOOL_CHAIN_PREFIX)-gcc
    CPP     = @echo "CPP      $<"; $(TOOL_CHAIN_PATH)$(TOOL_CHAIN_PREFIX)-g++
    LINK    = @echo "LINK     $(notdir $(IMAGEODIR)/$(TARGET).elf)"; $(TOOL_CHAIN_PATH)$(TOOL_CHAIN_PREFIX)-gcc
    OBJCOPY = @echo "OBJCOPY  $(notdir $(FIRMWAREDIR)/$(TARGET)/$(TARGET).bin)"; $(TOOL_CHAIN_PATH)$(TOOL_CHAIN_PREFIX)-objcopy
    OBJDUMP = @echo "OBJDUMP  $<"; $(TOOL_CHAIN_PATH)$(TOOL_CHAIN_PREFIX)-objdump
endif

LDDIR = $(TOP_DIR)/ld/$(CONFIG_ARCH_TYPE)
ifeq ($(CONFIG_W800_USE_NIMBLE),y)
LD_FILE = $(LDDIR)/gcc_csky.ld
else
LD_FILE = $(LDDIR)/gcc_csky_bt.ld
endif
LIB_EXT = .a

CCFLAGS := -Wall \
    -DTLS_CONFIG_CPU_XT804=1 \
    -DGCC_COMPILE=1 \
    -mcpu=ck804ef \
    $(optimization) \
    -std=gnu99 \
    -c  \
    -mhard-float  \
    -Wall  \
    -fdata-sections  \
    -ffunction-sections

ASMFLAGS := -Wall \
    -DTLS_CONFIG_CPU_XT804=1 \
    -DGCC_COMPILE=1 \
    -mcpu=ck804ef \
    $(optimization) \
    -std=gnu99 \
    -c  \
    -mhard-float \
    -Wa,--gdwarf2 \
    -fdata-sections  \
    -ffunction-sections

ARFLAGS := ru

ARFLAGS_2 = xo

LINKFLAGS := -mcpu=ck804ef \
    -nostartfiles \
    -mhard-float \
    -lm \
    -Wl,-T$(LD_FILE)

MAP := -Wl,-ckmap=$(IMAGEODIR)/$(TARGET).map

sinclude $(TOP_DIR)/tools/w800/inc.mk