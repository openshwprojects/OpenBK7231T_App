# HACK - if COMPILE_PREX defined then we are being called running from original build_app.sh script in standard SDK
# Required to not break old build_app.sh script lines 74-77
ifdef COMPILE_PREX
all:
	@echo Calling original build_app.sh script
	cd $(PWD)/../../platforms/$(TARGET_PLATFORM)/toolchain/$(TUYA_APPS_BUILD_PATH) && sh $(TUYA_APPS_BUILD_CMD) $(APP_NAME) $(APP_VERSION) $(TARGET_PLATFORM)
else

######## Continue with custom simplied Makefile ########

# Use current folder name as app name
APP_NAME ?= $(shell basename $(CURDIR))
# Use current timestamp as version number
TIMESTAMP := $(shell date +%Y%m%d_%H%M%S)
APP_VERSION ?= dev_$(TIMESTAMP)

TARGET_PLATFORM ?= bk7231t
APPS_BUILD_PATH ?= ../bk7231t_os
APPS_BUILD_CMD ?= build.sh

# Default target is to run build
all: build

# Full target will clean then build all
.PHONY: full
full: clean build

# Update/init git submodules
.PHONY: submodules
submodules:
	git submodule update --init --recursive

# Create symlink for App into SDK folder structure
sdk/apps/$(APP_NAME):
	@echo Create symlink for $(APP_NAME) into sdk/apps folder
	ln -s "$(shell pwd)/" "sdk/apps/$(APP_NAME)"

# Build main binary
.PHONY: build
build: submodules sdk/apps/$(APP_NAME)
	cd sdk/platforms/$(TARGET_PLATFORM)/toolchain/$(APPS_BUILD_PATH) && sh $(APPS_BUILD_CMD) $(APP_NAME) $(APP_VERSION) $(TARGET_PLATFORM)

# clean .o files
.PHONY: clean
clean: 
	$(MAKE) -C sdk/platforms/$(TARGET_PLATFORM)/toolchain/$(APPS_BUILD_PATH) APP_BIN_NAME=$(APP_NAME) USER_SW_VER=$(APP_VERSION) clean
	rm -rf output/*

# Add custom Makefile if required
-include custom.mk

endif