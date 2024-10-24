ifeq ($(IMAGEODIR)/$(TARGET).elf,$(wildcard $(IMAGEODIR)/$(TARGET).elf))
ifeq ($(MAKECMDGOALS),)
$(shell $(RM) -rf $(IMAGEODIR)/$(TARGET).elf)
else ifeq ($(MAKECMDGOALS),all)
$(shell $(RM) -rf $(IMAGEODIR)/$(TARGET).elf)
else ifeq ($(MAKECMDGOALS),image)
$(shell $(RM) -rf $(IMAGEODIR)/$(TARGET).elf)
else ifeq ($(MAKECMDGOALS),flash)
$(shell $(RM) -rf $(IMAGEODIR)/$(TARGET).elf)
else ifeq ($(MAKECMDGOALS),run)
$(shell $(RM) -rf $(IMAGEODIR)/$(TARGET).elf)
endif
endif

CSRCS ?= $(wildcard *.c)
CPPSRCS ?= $(wildcard *.cpp)
ASRCS ?= $(wildcard *.S)

subdir_path := $(subst $(abspath $(TOP_DIR))/,,$(shell pwd))

SUBDIRS ?= $(patsubst %/,%,$(dir $(wildcard */Makefile)))

OBJS := $(CSRCS:%.c=$(OBJODIR)/$(subdir_path)/%.o) \
        $(ASRCS:%.S=$(OBJODIR)/$(subdir_path)/%.o) \
	$(CPPSRCS:%.cpp=$(OBJODIR)/$(subdir_path)/%.o)

OBJS-DEPS := $(patsubst %.c, $(OBJODIR)/$(subdir_path)/%.o.d, $(CSRCS))
OBJS-CPPDEPS := $(patsubst %.cpp, $(OBJODIR)/$(subdir_path)/%.o.d, $(CPPSRCS))

OLIBS := $(GEN_LIBS:%=$(LIBODIR)/%)

OIMAGES := $(GEN_IMAGES:%=$(IMAGEODIR)/%)

OBINS := $(GEN_BINS:%=$(BINODIR)/%)

CFLAGS = $(CCFLAGS) $(DEFINES) $(EXTRA_CCFLAGS) $(INCLUDES)

define ShortcutRule
$(1): .subdirs $(2)/$(1)
endef

define MakeLibrary
DEP_LIBS_$(1) = $$(foreach lib,$$(filter %$(LIB_EXT),$$(COMPONENTS_$(1))),$$(LIBODIR)/$$(notdir $$(lib)))
DEP_OBJS_$(1) = $$(foreach obj,$$(filter %.o,$$(COMPONENTS_$(1))),$$(OBJODIR)/$$(notdir $$(obj)))
$$(LIBODIR)/$(1)$(LIB_EXT): $$(OBJS) $$(DEP_OBJS_$(1)) $$(DEP_LIBS_$(1)) $$(DEPENDS_$(1))
	@mkdir -p $$(LIBODIR)
	$$(if $$(filter %$(LIB_EXT),$$?),@mkdir -p $$(OBJODIR)/_$(1))
	$$(if $$(filter %$(LIB_EXT),$$?),@cd $$(OBJODIR)/_$(1); $$(foreach lib,$$(filter %$(LIB_EXT),$$?),$$(AR) $(ARFLAGS_2) $$(UP_EXTRACT_DIR)/$$(notdir $$(lib));))
	$$(AR) $(ARFLAGS) $$@ $$(filter %.o,$$?) $$(if $$(filter %$(LIB_EXT),$$?),$$(OBJODIR)/_$(1)/*.o)
	$$(if $$(filter %$(LIB_EXT),$$?),@$$(RM) -r $$(OBJODIR)/_$(1))
endef

define MakeImage
DEP_LIBS_$(1) = $$(foreach lib,$$(filter %$(LIB_EXT),$$(COMPONENTS_$(1))),$$(LIBODIR)/$$(notdir $$(lib)))
DEP_OBJS_$(1) = $$(foreach obj,$$(filter %.o,$$(COMPONENTS_$(1))),$$(OBJODIR)/$$(notdir $$(obj)))
$$(IMAGEODIR)/$(1).elf: $$(OBJS) $$(DEP_OBJS_$(1)) $$(DEP_LIBS_$(1)) $$(DEPENDS_$(1))
	@mkdir -p $$(IMAGEODIR)
	$(LINK) -Wl,--gc-sections -Wl,-zmax-page-size=1024 -Wl,--whole-archive $$(OBJS) $$(DEP_OBJS_$(1)) $$(DEP_LIBS_$(1)) $$(if $$(LINKFLAGS_$(1)),$$(LINKFLAGS_$(1))) -Wl,--no-whole-archive $(LINKFLAGS) $(MAP) -o $$@
endef

$(BINODIR)/%.bin: $(IMAGEODIR)/%.elf
	@mkdir -p $(FIRMWAREDIR)
	@mkdir -p $(FIRMWAREDIR)/$(TARGET)
	$(OBJCOPY) -O binary $(IMAGEODIR)/$(TARGET).elf $(FIRMWAREDIR)/$(TARGET)/$(TARGET).bin

ifeq ($(UNAME_S),Linux)
	@$(RM) -rf $(SDK_TOOLS)/wm_tool
	@make -C $(SDK_TOOLS)/zlib-1.2.11
	@gcc $(SDK_TOOLS)/wm_tool.c -lpthread -o $(WM_TOOL) -L. $(SDK_TOOLS)/zlib-1.2.11/libz.a
else
ifeq ($(UNAME_O),Darwin)
	@$(RM) -rf $(SDK_TOOLS)/wm_tool
	@make -C $(SDK_TOOLS)/zlib-1.2.11
	@gcc $(SDK_TOOLS)/wm_tool.c -lpthread -o $(WM_TOOL) -L. $(SDK_TOOLS)/zlib-1.2.11/libz.a
else
#if you want to make wm_tool.c to produce your wm_tool.exe,you should unmask script below by #
#ifeq ($(UNAME_O),Msys)
#	@$(RM) -rf $(SDK_TOOLS)/wm_tool
#	@gcc $(SDK_TOOLS)/wm_tool.c -o $(WM_TOOL)
#endif
# windows, cygwin-gcc exist bug for uart rts/cts
endif
endif

ifeq ($(CODE_ENCRYPT),1)
	@openssl enc -aes-128-ecb -in $(FIRMWAREDIR)/$(TARGET)/$(TARGET).bin -out $(FIRMWAREDIR)/$(TARGET)/$(TARGET)_enc.bin -K 30313233343536373839616263646566 -iv 01010101010101010101010101010101
	@openssl rsautl -encrypt -in $(CA_PATH)/key.txt -inkey $(CA_PATH)/capub_$(PRIKEY_SEL).pem -pubin -out $(FIRMWAREDIR)/$(TARGET)/key_en.dat
	@cat $(FIRMWAREDIR)/$(TARGET)/$(TARGET)_enc.bin $(FIRMWAREDIR)/$(TARGET)/key_en.dat > $(FIRMWAREDIR)/$(TARGET)/$(TARGET)_enc_key.bin
	@cat $(FIRMWAREDIR)/$(TARGET)/$(TARGET)_enc_key.bin $(CA_PATH)/capub_$(PRIKEY_SEL)_N.dat > $(FIRMWAREDIR)/$(TARGET)/$(TARGET)_enc_key_N.bin
	@$(WM_TOOL) -b $(FIRMWAREDIR)/$(TARGET)/$(TARGET)_enc_key_N.bin -it $(IMG_TYPE) -fc 0 -ra $(RUN_ADDRESS) -ih $(IMG_HEADER) -ua $(UPD_ADDRESS) -nh 0 -un 0 -vs $(shell $(VER_TOOL) $(TOP_DIR)/platform/sys/wm_main.c) -o $(FIRMWAREDIR)/$(TARGET)/$(TARGET)
else
	@$(WM_TOOL) -b $(FIRMWAREDIR)/$(TARGET)/$(TARGET).bin -fc 0 -it $(IMG_TYPE) -ih $(IMG_HEADER) -ra $(RUN_ADDRESS) -ua $(UPD_ADDRESS) -nh 0 -un 0 -vs $(shell $(VER_TOOL) $(TOP_DIR)/platform/sys/wm_main.c) -o $(FIRMWAREDIR)/$(TARGET)/$(TARGET)
endif
	@cp $(IMAGEODIR)/$(TARGET).map $(FIRMWAREDIR)/$(TARGET)/$(TARGET).map

	@$(WM_TOOL) -b  $(SEC_BOOT_BIN) -fc 0 -it $(SECBOOT_IMG_TYPE) -ih $(SECBOOT_HEADER_POS) -ra $(SECBOOT_ADDRESS_POS) -ua $(UPD_ADDRESS) -nh $(IMG_HEADER) -un 0 -o $(SEC_BOOT_IMG)

ifeq ($(SIGNATURE),1)
	@openssl dgst -sign $(CA_PATH)/cakey.pem -sha1 -out $(FIRMWAREDIR)/$(TARGET)/$(TARGET)_sign.dat $(FIRMWAREDIR)/$(TARGET)/$(TARGET).img
	@cat $(FIRMWAREDIR)/$(TARGET)/$(TARGET).img $(FIRMWAREDIR)/$(TARGET)/$(TARGET)_sign.dat > $(FIRMWAREDIR)/$(TARGET)/$(TARGET)_sign.img
	@openssl dgst -sign $(CA_PATH)/cakey.pem -sha1 -out $(SEC_BOOT_IMG)_sign.dat $(SEC_BOOT)
	@cat $(SEC_BOOT) $(SEC_BOOT_IMG)_sign.dat > $(SEC_BOOT_IMG)_sign.img
	@cat $(SEC_BOOT_IMG)_sign.img $(FIRMWAREDIR)/$(TARGET)/$(TARGET)_sign.img > $(FIRMWAREDIR)/$(TARGET)/$(TARGET).fls
	@$(WM_TOOL) -b $(FIRMWAREDIR)/$(TARGET)/$(TARGET)_sign.img -fc 1 -it $(IMG_TYPE) -ih $(IMG_HEADER) -ra $(RUN_ADDRESS) -ua $(UPD_ADDRESS) -nh 0 -un 0 -vs $(shell $(VER_TOOL) $(TOP_DIR)/platform/sys/wm_main.c) -o $(FIRMWAREDIR)/$(TARGET)/$(TARGET)_sign
	@openssl dgst -sign $(CA_PATH)/cakey.pem -sha1 -out $(FIRMWAREDIR)/$(TARGET)/$(TARGET)_sign_gz.dat $(FIRMWAREDIR)/$(TARGET)/$(TARGET)_sign_gz.img
	@cat $(FIRMWAREDIR)/$(TARGET)/$(TARGET)_sign_gz.img $(FIRMWAREDIR)/$(TARGET)/$(TARGET)_sign_gz.dat > $(FIRMWAREDIR)/$(TARGET)/$(TARGET)_sign_ota.img
else
	@cat $(SEC_BOOT) $(FIRMWAREDIR)/$(TARGET)/$(TARGET).img > $(FIRMWAREDIR)/$(TARGET)/$(TARGET).fls
	@$(WM_TOOL) -b $(FIRMWAREDIR)/$(TARGET)/$(TARGET).img -fc 1 -it $(IMG_TYPE) -ih $(IMG_HEADER) -ra $(RUN_ADDRESS) -ua $(UPD_ADDRESS) -nh 0 -un 0 -vs $(shell $(VER_TOOL) $(TOP_DIR)/platform/sys/wm_main.c) -o $(FIRMWAREDIR)/$(TARGET)/$(TARGET)
	@mv $(FIRMWAREDIR)/$(TARGET)/$(TARGET)_gz.img $(FIRMWAREDIR)/$(TARGET)/$(TARGET)_ota.img
endif
	@echo "build finished!"

all: .subdirs $(OBJS) $(OLIBS) $(OIMAGES) $(OBINS)

help:
	@echo  'Cleaning targets:'
	@echo  '  clean      - Remove most generated files'
	@echo  '  distclean  - Remove all generated files'
	@echo  ''
	@echo  'Configuration targets:'
	@echo  '  menuconfig - Update current config utilising a menu based program'
	@echo  ''
	@echo  'Compiling targets:'
	@echo  '  all	     - Build firmware'
	@echo  '  lib	     - Build library'
	@echo  ''
	@echo  'Programming targets:'
	@echo  '  image	     - Flash img firmware to device'
	@echo  '  flash      - Flash fls firmware to the device'
	@echo  '  erase      - Erase device flash'
	@echo  '  down       - Flash fls firmware to the device (without compile)'
	@echo  ''
	@echo  'Other targets:'
	@echo  '  list       - List locally available serial ports'
	@echo  '  run        - Flash the firmware to the device after compilation'
	@echo  '               and capture the log output by the device'

lib: .subdirs $(OBJS) $(OLIBS)
ifeq ($(USE_NIMBLE), 1)
	@cp $(LIBODIR)/libapp$(LIB_EXT) $(TOP_DIR)/lib/$(CONFIG_ARCH_TYPE)
	@cp $(LIBODIR)/libblehost$(LIB_EXT) $(TOP_DIR)/lib/$(CONFIG_ARCH_TYPE)
else
	@cp $(LIBODIR)/libapp_br_edr$(LIB_EXT) $(TOP_DIR)/lib/$(CONFIG_ARCH_TYPE)
	@cp $(LIBODIR)/libbthost_br_edr$(LIB_EXT) $(TOP_DIR)/lib/$(CONFIG_ARCH_TYPE)
endif	
	@cp $(LIBODIR)/libwmarch$(LIB_EXT) $(TOP_DIR)/lib/$(CONFIG_ARCH_TYPE)
	@cp $(LIBODIR)/libwmcommon$(LIB_EXT) $(TOP_DIR)/lib/$(CONFIG_ARCH_TYPE)
	@cp $(LIBODIR)/libdrivers$(LIB_EXT) $(TOP_DIR)/lib/$(CONFIG_ARCH_TYPE)
	@cp $(LIBODIR)/libnetwork$(LIB_EXT) $(TOP_DIR)/lib/$(CONFIG_ARCH_TYPE)
	@cp $(LIBODIR)/libos$(LIB_EXT) $(TOP_DIR)/lib/$(CONFIG_ARCH_TYPE)
	@cp $(LIBODIR)/libwmsys$(LIB_EXT) $(TOP_DIR)/lib/$(CONFIG_ARCH_TYPE)
	@echo "libs has been updated."

menuconfig:
	@$(SDK_TOOLS)/mconfig.sh

clean:
#	$(foreach d, $(SUBDIRS), $(MAKE) -C $(d) clean;)
	$(RM) -r $(ODIR)

distclean:clean
	$(RM) -r $(FIRMWAREDIR)/$(CONFIG_ARCH_TYPE)
	$(RM) -r $(SDK_TOOLS)/.config.old

run:all
	@$(WM_TOOL) -c $(DL_PORT) -rs at -ds $(DL_BAUD) -dl $(FIRMWAREDIR)/$(TARGET)/$(TARGET).fls -sl str -ws 115200

list:
	@$(WM_TOOL) -l

down:
	@$(WM_TOOL) -c $(DL_PORT) -rs at -ds $(DL_BAUD) -dl $(FIRMWAREDIR)/$(TARGET)/$(TARGET).fls

image:all
	@$(WM_TOOL) -c $(DL_PORT) -rs at -ds $(DL_BAUD) -dl $(FIRMWAREDIR)/$(TARGET)/$(TARGET).img

flash:all
	@$(WM_TOOL) -c $(DL_PORT) -rs at -ds $(DL_BAUD) -dl $(FIRMWAREDIR)/$(TARGET)/$(TARGET).fls

erase:
	@$(WM_TOOL) -c $(DL_PORT) -rs at -eo all

.subdirs:
	@set -e; $(foreach d, $(SUBDIRS), $(MAKE) -C $(d);)

sinclude $(OBJS-DEPS)
sinclude $(OBJS-CPPDEPS)

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(MAKECMDGOALS),clobber)
ifdef DEPS
sinclude $(DEPS)
endif
endif
endif

$(OBJODIR)/$(subdir_path)/%.o: %.c
	@mkdir -p $(OBJODIR)/$(subdir_path)
	$(CC) $(if $(findstring $<,$(DSRCS)),$(DFLAGS),$(CFLAGS)) $(COPTS_$(*F)) $(INCLUDES) $(CMACRO) -c "$<" -o "$@" -MMD -MD -MF "$(@:$(OBJODIR)/$(subdir_path)/%.o=$(OBJODIR)/$(subdir_path)/%.o.d)" -MT "$(@)"

$(OBJODIR)/$(notdir $(shell pwd))/%.o: %.cpp
	@mkdir -p $(OBJODIR)/$(subdir_path)
	$(CPP) $(if $(findstring $<,$(DSRCS)),$(DFLAGS),$(CFLAGS)) $(COPTS_$(*F)) $(INCLUDES) $(CMACRO) -c "$<" -o "$@" -MMD -MD -MF "$(@:$(OBJODIR)/$(subdir_path)/%.o=$(OBJODIR)/$(subdir_path)/%.o.d)" -MT "$(@)"
$(OBJODIR)/$(subdir_path)/%.o: %.S
	@mkdir -p $(OBJODIR)/$(subdir_path)
	$(ASM) $(ASMFLAGS) $(INCLUDES) $(CMACRO) -c "$<" -o "$@"

$(foreach lib,$(GEN_LIBS),$(eval $(call ShortcutRule,$(lib),$(LIBODIR))))

$(foreach image,$(GEN_IMAGES),$(eval $(call ShortcutRule,$(image),$(IMAGEODIR))))

$(foreach bin,$(GEN_BINS),$(eval $(call ShortcutRule,$(bin),$(BINODIR))))

$(foreach lib,$(GEN_LIBS),$(eval $(call MakeLibrary,$(basename $(lib)))))

$(foreach image,$(GEN_IMAGES),$(eval $(call MakeImage,$(basename $(image)))))
