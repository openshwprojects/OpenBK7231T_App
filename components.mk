OBK_DIR = $(TOP_DIR)/apps/$(APP_BIN_NAME)/

BERRY_SRCPATH = $(OBK_DIR)/libraries/berry/src/

# different frameworks put object files in different places,
# berry needs to add a rule to autogenerate some files before the object files
# are built, so it needs the translation function from a C source to an object
# file
define obj_from_c
	$(patsubst %.c, %.o, $(1))
endef

include $(OBK_DIR)/libraries/berry.mk

SRC_C += $(BERRY_SRC_C)
