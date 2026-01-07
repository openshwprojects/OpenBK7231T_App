# HACK - if COMPILE_PREX defined then we are being called running from original build_app.sh script in standard SDK
# Required to not break old build_app.sh script lines 74-77
MBEDTLS=output/mbedtls-2.28.5
ifdef COMPILE_PREX
all:
	@echo Calling original build_app.sh script
	mkdir -p output
	if [ ! -d "$(MBEDTLS)" ]; then wget -q "https://github.com/Mbed-TLS/mbedtls/archive/refs/tags/v2.28.5.tar.gz"; tar -xf v2.28.5.tar.gz -C output; rm -f v2.28.5.tar.gz; mv $(MBEDTLS)/library/base64.c $(MBEDTLS)/library/base64_mbedtls.c; fi
	cd $(PWD)/../../platforms/$(TARGET_PLATFORM)/toolchain/$(TUYA_APPS_BUILD_PATH) && sh $(TUYA_APPS_BUILD_CMD) $(APP_NAME) $(APP_VERSION) $(TARGET_PLATFORM) "$(USER_CMD)" $(BUILD_MODE)
else

######## Continue with custom simplied Makefile ########

# Use current folder name as app name
APP_NAME ?= $(shell basename $(CURDIR))
# Use current timestamp as version number
TIMESTAMP := $(shell date +%Y%m%d_%H%M%S)
APP_VERSION ?= dev_$(TIMESTAMP)

ifeq ($(VARIANT),berry)
OBK_VARIANT = 1
else ifeq ($(VARIANT),tuyaMCU)
OBK_VARIANT = 2
else ifeq ($(VARIANT),powerMetering)
OBK_VARIANT = 3
else ifeq ($(VARIANT),irRemoteESP)
OBK_VARIANT = 4
else ifeq ($(VARIANT),sensors)
OBK_VARIANT = 5
else ifeq ($(VARIANT),hlw8112)
OBK_VARIANT = 6
else ifeq ($(VARIANT),2M)
OBK_VARIANT = 1
ESP_FSIZE = 2MB
else ifeq ($(VARIANT),4M)
OBK_VARIANT = 2
ESP_FSIZE = 4MB
else ifeq ($(VARIANT),2M_berry)
OBK_VARIANT = 3
ESP_FSIZE = 2MB
else
OBK_VARIANT = 0
endif
$(info VARIANT is $(VARIANT), OBK_VARIANT is $(OBK_VARIANT))

#TARGET_PLATFORM ?= bk7231t
#APPS_BUILD_PATH ?= ../bk7231t_os
APPS_BUILD_CMD ?= build.sh

# Default target is to run OpenBK7231T build
all: OpenBK7231T

# Full target will clean then build all
.PHONY: full
full: clean all

# Update/init git submodules
.PHONY: submodules
submodules:
ifdef GITHUB_ACTIONS
	@echo Submodules already checked out during setup
else
	git submodule update --init --recursive
endif

update-submodules: submodules
ifdef GITHUB_ACTIONS
	git config user.name github-actions
	git config user.email github-actions@github.com
endif

.PHONY: berry_init berry
berry_init:
	git submodule update --init --recursive libraries/berry
	@mkdir -p libraries/berry/generate
	@mkdir -p libraries/berry/temp
	@./libraries/berry/tools/coc/coc -o libraries/berry/temp libraries/berry/src src/berry/modules -c include/berry_conf.h
ifneq ("$(wildcard libraries/berry/generate)","")
	@echo "[Prebuild berry] resources already generated, checking for differences"
endif

berry: berry_init
	@[ "$(shell diff -rq libraries/berry/generate libraries/berry/temp)" ] && (echo "[Prebuild berry] regenerate resources" && cp -r libraries/berry/temp/* libraries/berry/generate) || echo "[Prebuild berry] resources are not different"

# Create symlink for App into SDK folder structure
sdk/OpenBK7231T/apps/$(APP_NAME):
	@echo Create symlink for $(APP_NAME) into sdk folder
	ln -s "$(shell pwd)/" "sdk/OpenBK7231T/apps/$(APP_NAME)"

sdk/OpenBK7231N/apps/$(APP_NAME):
	@echo Create symlink for $(APP_NAME) into sdk folder
	ln -s "$(shell pwd)/" "sdk/OpenBK7231N/apps/$(APP_NAME)"

sdk/OpenXR809/project/oxr_sharedApp/shared:
	@echo Create symlink for $(APP_NAME) into sdk folder
	ln -s "$(shell pwd)/" "sdk/OpenXR809/project/oxr_sharedApp/shared"

sdk/OpenXR806/project/demo/sharedApp/shared:
	@echo Create symlink for $(APP_NAME) into sdk folder
	ln -s "$(shell pwd)/" "sdk/OpenXR806/project/demo/sharedApp/shared"
	
sdk/OpenXR872/project/demo/hello_demo/shared:
	@echo Create symlink for $(APP_NAME) into sdk folder
	ln -s "$(shell pwd)/" "sdk/OpenXR872/project/demo/hello_demo/shared"
	
sdk/OpenBL602/customer_app/bl602_sharedApp/bl602_sharedApp/shared:
	#cannot symlink shared directly, because sdk is looking for stuff recursively and crashes
	#so only linking source and copying required file
	@echo mkdir shared
	mkdir sdk/OpenBL602/customer_app/bl602_sharedApp/bl602_sharedApp/shared
	cp ./platforms/BL602/bouffalo.mk sdk/OpenBL602/customer_app/bl602_sharedApp/bl602_sharedApp/shared
	@echo Create symlink for $(APP_NAME) into sdk folder
	ln -s "$(shell pwd)/src/" "sdk/OpenBL602/customer_app/bl602_sharedApp/bl602_sharedApp/shared/src"
	ln -s "$(shell pwd)/libraries/" "sdk/OpenBL602/customer_app/bl602_sharedApp/bl602_sharedApp/shared/libraries"
	ln -s "$(shell pwd)/include/" "sdk/OpenBL602/customer_app/bl602_sharedApp/bl602_sharedApp/shared/include"

sdk/OpenW800/sharedAppContainer/sharedApp:
	@echo Create symlink for $(APP_NAME) into sdk folder
	@mkdir "sdk/OpenW800/sharedAppContainer"
	ln -s "$(shell pwd)/" "sdk/OpenW800/sharedAppContainer/sharedApp"

sdk/OpenW600/sharedAppContainer/sharedApp:
	@echo Create symlink for $(APP_NAME) into sdk folder
	@mkdir -p "sdk/OpenW600/sharedAppContainer"
	ln -s "$(shell pwd)/" "sdk/OpenW600/sharedAppContainer/sharedApp"

sdk/OpenLN882H/project/OpenBeken/app:
	@echo Create symlink for $(APP_NAME) into sdk folder
	@mkdir -p "sdk/OpenLN882H/project/OpenBeken"
	ln -s "$(shell pwd)/" "sdk/OpenLN882H/project/OpenBeken/app"

.PHONY: prebuild_OpenBK7231N prebuild_OpenBK7231T prebuild_OpenBL602 prebuild_OpenLN882H 
.PHONY: prebuild_OpenW600 prebuild_OpenW800 prebuild_OpenXR809 prebuild_OpenXR806 prebuild_OpenXR872 prebuild_ESPIDF prebuild_OpenTR6260
.PHONY: prebuild_OpenRTL87X0C prebuild_OpenBK7238 prebuild_OpenBK7231N_ALT prebuild_OpenBK7231U
.PHONY: prebuild_OpenBK7231N_ALT prebuild_OpenBK7231T_ALT prebuild_OpenBK7252

prebuild_OpenBK7231N: berry
	git submodule update --init --recursive --depth=1 sdk/OpenBK7231N
	@if [ -e platforms/BK7231N/pre_build.sh ]; then \
		echo "prebuild found for OpenBK7231N"; \
		sh platforms/BK7231N/pre_build.sh; \
	else echo "prebuild for OpenBK7231N not found ... "; \
	fi

prebuild_OpenBK7231T: berry
	git submodule update --init --recursive --depth=1 sdk/OpenBK7231T
	@if [ -e platforms/BK7231T/pre_build.sh ]; then \
		echo "prebuild found for OpenBK7231T"; \
		sh platforms/BK7231T/pre_build.sh; \
	else echo "prebuild for OpenBK7231T not found ... "; \
	fi

prebuild_OpenBL602: berry
ifdef GITHUB_ACTIONS
	pip3 install fdt toml configobj pycryptodomex
endif
	git submodule update --init --recursive --depth=1 sdk/OpenBL602
	@if [ -e platforms/BL602/pre_build.sh ]; then \
		echo "prebuild found for OpenBL602"; \
		sh platforms/BL602/pre_build.sh; \
	else echo "prebuild for OpenBL602 not found ... "; \
	fi

prebuild_OpenLN882H: berry
	git submodule update --init --recursive --depth=1 sdk/OpenLN882H
	@if [ -e platforms/LN882H/pre_build.sh ]; then \
		echo "prebuild found for OpenLN882H"; \
		sh platforms/LN882H/pre_build.sh; \
	else echo "prebuild for OpenLN882H not found ... "; \
	fi

prebuild_OpenW600: berry
	git submodule update --init --recursive --depth=1 sdk/OpenW600
	@if [ -e platforms/W600/pre_build.sh ]; then \
		echo "prebuild found for OpenW600"; \
		sh platforms/W600/pre_build.sh; \
	else echo "prebuild for OpenW600 not found ... "; \
	fi

prebuild_OpenW800: berry
	git submodule update --init --recursive --depth=1 sdk/OpenW800
	@if [ -e platforms/W800/pre_build.sh ]; then \
		echo "prebuild found for OpenW800"; \
		sh platforms/W800/pre_build.sh; \
	else echo "prebuild for OpenW800 not found ... "; \
	fi

prebuild_OpenXR809: berry
	git submodule update --init --recursive --depth=1 sdk/OpenXR809
	@if [ -e platforms/XR809/pre_build.sh ]; then \
		echo "prebuild found for OpenXR809"; \
		sh platforms/XR809/pre_build.sh; \
	else echo "prebuild for OpenXR809 not found ... "; \
	fi

prebuild_OpenXR806: berry
	git submodule update --init --recursive --depth=1 sdk/OpenXR806
	@if [ -e platforms/XR806/pre_build.sh ]; then \
		echo "prebuild found for OpenXR806"; \
		sh platforms/XR806/pre_build.sh; \
	else echo "prebuild for OpenXR806 not found ... "; \
	fi
	
prebuild_OpenXR872: berry
	git submodule update --init --recursive --depth=1 sdk/OpenXR872
	@if [ -e platforms/XR872/pre_build.sh ]; then \
		echo "prebuild found for OpenXR872"; \
		sh platforms/XR872/pre_build.sh; \
	else echo "prebuild for OpenXR872 not found ... "; \
	fi
	
prebuild_ESPIDF: berry
	#git submodule update --init --recursive --depth=1 sdk/esp-idf
	-rm platforms/ESP-IDF/sdkconfig
	-rm platforms/ESP-IDF/partitions.csv
ifeq ($(OBK_VARIANT), 1)
	cp platforms/ESP-IDF/partitions-2mb.csv platforms/ESP-IDF/partitions.csv
else ifeq ($(OBK_VARIANT), 2)
	cp platforms/ESP-IDF/partitions-4mb.csv platforms/ESP-IDF/partitions.csv
else ifeq ($(OBK_VARIANT), 3)
	cp platforms/ESP-IDF/partitions-2mb.csv platforms/ESP-IDF/partitions.csv
endif
	@if [ -e platforms/ESP-IDF/pre_build.sh ]; then \
		echo "prebuild found for ESP-IDF"; \
		sh platforms/ESP-IDF/pre_build.sh; \
	else echo "prebuild for ESP-IDF not found ... "; \
	fi
	
prebuild_ESP8266: berry
	#git submodule update --init --recursive --depth=1 sdk/OpenESP8266
	-rm platforms/ESP8266/sdkconfig
	-rm platforms/ESP8266/partitions.csv
	cp platforms/ESP8266/partitions-2mb.csv platforms/ESP8266/partitions.csv
	@if [ -e platforms/ESP8266/pre_build.sh ]; then \
		echo "prebuild found for ESP8266"; \
		sh platforms/ESP8266/pre_build.sh; \
	else echo "prebuild for ESP8266 not found ... "; \
	fi

prebuild_OpenTR6260: berry
	git submodule update --init --recursive --depth=1 sdk/OpenTR6260
	@if [ -e platforms/TR6260/pre_build.sh ]; then \
		echo "prebuild found for OpenTR6260"; \
		sh platforms/TR6260/pre_build.sh; \
	else echo "prebuild for OpenTR6260 not found ... "; \
	fi

prebuild_OpenRTL87X0C: berry
	git submodule update --init --recursive --depth=1 sdk/OpenRTL87X0C
	@if [ -e platforms/RTL87X0C/pre_build.sh ]; then \
		echo "prebuild found for OpenRTL87X0C"; \
		sh platforms/RTL87X0C/pre_build.sh; \
	else echo "prebuild for OpenRTL87X0C not found ... "; \
	fi

prebuild_OpenRTL8710B: berry
	git submodule update --init --recursive --depth=1 sdk/OpenRTL8710A_B
	@if [ -e platforms/RTL8710B/pre_build.sh ]; then \
		echo "prebuild found for OpenRTL8710B"; \
		sh platforms/RTL8710B/pre_build.sh; \
	else echo "prebuild for OpenRTL8710B not found ... "; \
	fi
	@if [ -e platforms/RTL8710B/tools/amebaz_ota_combine ]; then \
		echo "amebaz_ota_combine is already compiled"; \
	else g++ -o platforms/RTL8710B/tools/amebaz_ota_combine platforms/RTL8710B/tools/amebaz_ota_combine.cpp --std=c++17 -lstdc++fs; \
	fi
	@if [ -e platforms/RTL8710B/tools/amebaz_ug_from_ota ]; then \
		echo "amebaz_ug_from_ota is already compiled"; \
	else g++ -o platforms/RTL8710B/tools/amebaz_ug_from_ota platforms/RTL8710B/tools/amebaz_ug_from_ota.cpp --std=c++17; \
	fi

prebuild_OpenRTL8710A: berry
	git submodule update --init --recursive --depth=1 sdk/OpenRTL8710A_B
	@if [ -e platforms/RTL8710A/pre_build.sh ]; then \
		echo "prebuild found for OpenRTL8710A"; \
		sh platforms/RTL8710A/pre_build.sh; \
	else echo "prebuild for OpenRTL8710A not found ... "; \
	fi

prebuild_OpenRTL8720D: berry
	git submodule update --init --recursive --depth=1 sdk/OpenRTL8720D
	@if [ -e platforms/RTL8720D/pre_build.sh ]; then \
		echo "prebuild found for OpenRTL8720D"; \
		sh platforms/RTL8720D/pre_build.sh; \
	else echo "prebuild for OpenRTL8720D not found ... "; \
	fi

prebuild_OpenBK7238: berry
	git submodule update --init --recursive --depth=1 sdk/beken_freertos_sdk
	@if [ -e platforms/BK723x/pre_build_7238.sh ]; then \
		echo "prebuild found for OpenBK7238"; \
		sh platforms/BK723x/pre_build_7238.sh; \
	else echo "prebuild for OpenBK7238 not found ... "; \
	fi

prebuild_OpenBK7231N_ALT: berry
	git submodule update --init --recursive --depth=1 sdk/beken_freertos_sdk
	@if [ -e platforms/BK723x/pre_build_7231n.sh ]; then \
		echo "prebuild found for OpenBK7231N"; \
		sh platforms/BK723x/pre_build_7231n.sh; \
	else echo "prebuild for OpenBK7231N not found ... "; \
	fi

prebuild_OpenBK7231U: berry
	git submodule update --init --recursive --depth=1 sdk/beken_freertos_sdk
	@if [ -e platforms/BK723x/pre_build_7231u.sh ]; then \
		echo "prebuild found for OpenBK7231U"; \
		sh platforms/BK723x/pre_build_7231u.sh; \
	else echo "prebuild for OpenBK7231U not found ... "; \
	fi

prebuild_OpenBK7231T_ALT: berry
	git submodule update --init --recursive --depth=1 sdk/beken_freertos_sdk
	@if [ -e platforms/BK723x/pre_build_7231t.sh ]; then \
		echo "prebuild found for OpenBK7231T"; \
		sh platforms/BK723x/pre_build_7231t.sh; \
	else echo "prebuild for OpenBK7231T not found ... "; \
	fi

prebuild_OpenBK7252: berry
	git submodule update --init --recursive --depth=1 sdk/beken_freertos_sdk
	@if [ -e platforms/BK723x/pre_build_7252.sh ]; then \
		echo "prebuild found for OpenBK7252"; \
		sh platforms/BK723x/pre_build_7252.sh; \
	else echo "prebuild for OpenBK7252 not found ... "; \
	fi

prebuild_OpenBK7252N: berry
	git submodule update --init --recursive --depth=1 sdk/beken_freertos_sdk
	@if [ -e platforms/BK723x/pre_build_7252n.sh ]; then \
		echo "prebuild found for OpenBK7252N"; \
		sh platforms/BK723x/pre_build_7252n.sh; \
	else echo "prebuild for OpenBK7252N not found ... "; \
	fi

prebuild_OpenECR6600: berry
	git submodule update --init --recursive --depth=1 sdk/OpenECR6600
	@if [ -e platforms/ECR6600/pre_build.sh ]; then \
		echo "prebuild found for OpenECR6600"; \
		sh platforms/ECR6600/pre_build.sh; \
	else echo "prebuild for OpenECR6600 not found ... "; \
	fi

prebuild_OpenRTL8721DA: berry
	git submodule update --init --recursive --depth=1 sdk/ameba-rtos
ifdef GITHUB_ACTIONS
	bash platforms/RTL8721DA/gh_prebuild.sh
endif
	if [ ! -e sdk/ameba-rtos/amebadplus_gcc_project/menuconfig/.config ]; then cd sdk/ameba-rtos/amebadplus_gcc_project && ./menuconfig.py -f ../../../platforms/RTL8721DA/default.conf; fi
	@if [ -e platforms/RTL8721DA/pre_build.sh ]; then \
		echo "prebuild found for OpenRTL8721DA"; \
		sh platforms/RTL8721DA/pre_build.sh; \
	else echo "prebuild for OpenRTL8721DA not found ... "; \
	fi

prebuild_OpenRTL8720E: berry
	git submodule update --init --recursive --depth=1 sdk/ameba-rtos
ifdef GITHUB_ACTIONS
	bash platforms/RTL8720E/gh_prebuild.sh
endif
	if [ ! -e sdk/ameba-rtos/amebalite_gcc_project/menuconfig/.config ]; then cd sdk/ameba-rtos/amebalite_gcc_project && ./menuconfig.py -f ../../../platforms/RTL8720E/default.conf; fi
	@if [ -e platforms/RTL8720E/pre_build.sh ]; then \
		echo "prebuild found for OpenRTL8720E"; \
		sh platforms/RTL8720E/pre_build.sh; \
	else echo "prebuild for OpenRTL8720E not found ... "; \
	fi

prebuild_OpenTXW81X: berry
	git submodule update --init --recursive --depth=1 sdk/OpenTXW81X
	if [ ! -e sdk/OpenTXW81X/tools/gcc/csky-elfabiv2 ]; then cd sdk/OpenTXW81X/tools/gcc && tar -xf *.tar.gz; fi
	@if [ -e platforms/TXW81X/pre_build.sh ]; then \
		echo "prebuild found for OpenTXW81X"; \
		sh platforms/TXW81X/pre_build.sh; \
	else echo "prebuild for OpenTXW81X not found ... "; \
	fi

prebuild_OpenRDA5981: berry
ifdef GITHUB_ACTIONS
	# just so that there would be no cache error
	pip3 install fdt toml configobj pycryptodomex
endif
	git submodule update --init --recursive --depth=1 sdk/OpenRDA5981
	@if [ -e platforms/RDA5981/pre_build.sh ]; then \
		echo "prebuild found for OpenRDA5981"; \
		sh platforms/RDA5981/pre_build.sh; \
	else echo "prebuild for OpenRDA5981 not found ... "; \
	fi

# Build main binaries
OpenBK7231T: prebuild_OpenBK7231T
	mkdir -p output
	if [ ! -d "$(MBEDTLS)" ]; then wget -q "https://github.com/Mbed-TLS/mbedtls/archive/refs/tags/v2.28.5.tar.gz"; tar -xf v2.28.5.tar.gz -C output; rm -f v2.28.5.tar.gz; mv $(MBEDTLS)/library/base64.c $(MBEDTLS)/library/base64_mbedtls.c; fi 
	$(MAKE) APP_NAME=OpenBK7231T TARGET_PLATFORM=bk7231t SDK_PATH=sdk/OpenBK7231T APPS_BUILD_PATH=../bk7231t_os OBK_VARIANT=$(OBK_VARIANT) build-BK7231

OpenBK7231N: prebuild_OpenBK7231N
	mkdir -p output
	if [ ! -d "$(MBEDTLS)" ]; then wget -q "https://github.com/Mbed-TLS/mbedtls/archive/refs/tags/v2.28.5.tar.gz"; tar -xf v2.28.5.tar.gz -C output; rm -f v2.28.5.tar.gz; mv $(MBEDTLS)/library/base64.c $(MBEDTLS)/library/base64_mbedtls.c; fi
	$(MAKE) APP_NAME=OpenBK7231N TARGET_PLATFORM=bk7231n SDK_PATH=sdk/OpenBK7231N APPS_BUILD_PATH=../bk7231n_os OBK_VARIANT=$(OBK_VARIANT) build-BK7231

.PHONY: OpenXR872
OpenXR872: prebuild_OpenXR872 sdk/OpenXR872/project/demo/hello_demo/shared
	$(MAKE) -C sdk/OpenXR872/src CC_DIR=$(ARM_NONE_EABI_GCC_PATH) APP_VERSION=$(APP_VERSION) OBK_VARIANT=$(OBK_VARIANT) -j $(shell nproc) --no-print-directory
	$(MAKE) -C sdk/OpenXR872/src install CC_DIR=$(ARM_NONE_EABI_GCC_PATH) APP_VERSION=$(APP_VERSION) OBK_VARIANT=$(OBK_VARIANT) -j $(shell nproc) --no-print-directory
	$(MAKE) -C sdk/OpenXR872/project/demo/hello_demo/gcc CC_DIR=$(ARM_NONE_EABI_GCC_PATH) APP_VERSION=$(APP_VERSION) OBK_VARIANT=$(OBK_VARIANT) -j $(shell nproc) --no-print-directory
	$(MAKE) -C sdk/OpenXR872/project/demo/hello_demo/gcc image CC_DIR=$(ARM_NONE_EABI_GCC_PATH) APP_VERSION=$(APP_VERSION) OBK_VARIANT=$(OBK_VARIANT) -j $(shell nproc) --no-print-directory
	$(MAKE) -C sdk/OpenXR872/project/demo/hello_demo/gcc image_xz CC_DIR=$(ARM_NONE_EABI_GCC_PATH) APP_VERSION=$(APP_VERSION) OBK_VARIANT=$(OBK_VARIANT) -j $(shell nproc) --no-print-directory
	mkdir -p output/$(APP_VERSION)
	cp sdk/OpenXR872/project/demo/hello_demo/image/xr872/xr_system.img output/$(APP_VERSION)/OpenXR872_$(APP_VERSION).img
	cp sdk/OpenXR872/project/demo/hello_demo/image/xr872/xr_system_img_xz.img output/$(APP_VERSION)/OpenXR872_$(APP_VERSION)_ota.img

.PHONY: OpenXR806
OpenXR806: prebuild_OpenXR806 sdk/OpenXR806/project/demo/sharedApp/shared
	$(MAKE) -C sdk/OpenXR806/src CC_DIR=$(ARM_NONE_EABI_GCC_PATH) APP_VERSION=$(APP_VERSION) OBK_VARIANT=$(OBK_VARIANT) -j $(shell nproc) --no-print-directory
	$(MAKE) -C sdk/OpenXR806/src install CC_DIR=$(ARM_NONE_EABI_GCC_PATH) APP_VERSION=$(APP_VERSION) OBK_VARIANT=$(OBK_VARIANT) -j $(shell nproc) --no-print-directory
	$(MAKE) -C sdk/OpenXR806/project/demo/sharedApp/gcc CC_DIR=$(ARM_NONE_EABI_GCC_PATH) APP_VERSION=$(APP_VERSION) OBK_VARIANT=$(OBK_VARIANT) -j $(shell nproc) --no-print-directory
	$(MAKE) -C sdk/OpenXR806/project/demo/sharedApp/gcc image CC_DIR=$(ARM_NONE_EABI_GCC_PATH) APP_VERSION=$(APP_VERSION) OBK_VARIANT=$(OBK_VARIANT) -j $(shell nproc) --no-print-directory
	$(MAKE) -C sdk/OpenXR806/project/demo/sharedApp/gcc image_xz CC_DIR=$(ARM_NONE_EABI_GCC_PATH) APP_VERSION=$(APP_VERSION) OBK_VARIANT=$(OBK_VARIANT) -j $(shell nproc) --no-print-directory
	mkdir -p output/$(APP_VERSION)
	cp sdk/OpenXR806/project/demo/sharedApp/image/xr806/xr_system.img output/$(APP_VERSION)/OpenXR806_$(APP_VERSION).img
	cp sdk/OpenXR806/project/demo/sharedApp/image/xr806/xr_system_img_xz.img output/$(APP_VERSION)/OpenXR806_$(APP_VERSION)_ota.img
	
.PHONY: OpenXR809
OpenXR809: prebuild_OpenXR809 sdk/OpenXR809/project/oxr_sharedApp/shared
	$(MAKE) -C sdk/OpenXR809/src CC_DIR=$(ARM_NONE_EABI_GCC_PATH) APP_VERSION=$(APP_VERSION) OBK_VARIANT=$(OBK_VARIANT) --no-print-directory  -j $(shell nproc)
	$(MAKE) -C sdk/OpenXR809/src install CC_DIR=$(ARM_NONE_EABI_GCC_PATH) APP_VERSION=$(APP_VERSION) OBK_VARIANT=$(OBK_VARIANT) --no-print-directory -j $(shell nproc)
	$(MAKE) -C sdk/OpenXR809/project/oxr_sharedApp/gcc CC_DIR=$(ARM_NONE_EABI_GCC_PATH) APP_VERSION=$(APP_VERSION) OBK_VARIANT=$(OBK_VARIANT) --no-print-directory -j $(shell nproc)
	$(MAKE) -C sdk/OpenXR809/project/oxr_sharedApp/gcc image CC_DIR=$(ARM_NONE_EABI_GCC_PATH) APP_VERSION=$(APP_VERSION) OBK_VARIANT=$(OBK_VARIANT) -j $(shell nproc) --no-print-directory
	mkdir -p output/$(APP_VERSION)
	cp sdk/OpenXR809/project/oxr_sharedApp/image/xr809/xr_system.img output/$(APP_VERSION)/OpenXR809_$(APP_VERSION).img

.PHONY: build-BK7231
build-BK7231: $(SDK_PATH)/apps/$(APP_NAME)
	cd $(SDK_PATH)/platforms/$(TARGET_PLATFORM)/toolchain/$(APPS_BUILD_PATH) && bash $(APPS_BUILD_CMD) $(APP_NAME) $(APP_VERSION) $(TARGET_PLATFORM)
	rm $(SDK_PATH)/platforms/$(TARGET_PLATFORM)/toolchain/$(APPS_BUILD_PATH)/tools/generate/$(APP_NAME)_*.rbl || /bin/true
	rm $(SDK_PATH)/platforms/$(TARGET_PLATFORM)/toolchain/$(APPS_BUILD_PATH)/tools/generate/$(APP_NAME)_*.bin || /bin/true

OpenBL602: prebuild_OpenBL602 sdk/OpenBL602/customer_app/bl602_sharedApp/bl602_sharedApp/shared
	$(MAKE) -C sdk/OpenBL602/customer_app/bl602_sharedApp USER_SW_VER=$(APP_VERSION) OBK_VARIANT=$(OBK_VARIANT) CONFIG_CHIP_NAME=BL602 CONFIG_LINK_ROM=1 -j $(shell nproc)
	$(MAKE) -C sdk/OpenBL602/customer_app/bl602_sharedApp USER_SW_VER=$(APP_VERSION) OBK_VARIANT=$(OBK_VARIANT) CONFIG_CHIP_NAME=BL602 bins
	mkdir -p output/$(APP_VERSION)
	cp sdk/OpenBL602/customer_app/bl602_sharedApp/build_out/bl602_sharedApp.bin output/$(APP_VERSION)/OpenBL602_$(APP_VERSION).bin
	cp sdk/OpenBL602/customer_app/bl602_sharedApp/build_out/ota/dts40M_pt2M_boot2release_ef4015/FW_OTA.bin output/$(APP_VERSION)/OpenBL602_$(APP_VERSION)_OTA.bin
	cp sdk/OpenBL602/customer_app/bl602_sharedApp/build_out/ota/dts40M_pt2M_boot2release_ef4015/FW_OTA.bin.xz output/$(APP_VERSION)/OpenBL602_$(APP_VERSION)_OTA.bin.xz
	cp sdk/OpenBL602/customer_app/bl602_sharedApp/build_out/ota/dts40M_pt2M_boot2release_ef4015/FW_OTA.bin.xz.ota output/$(APP_VERSION)/OpenBL602_$(APP_VERSION)_OTA.bin.xz.ota
	
sdk/OpenW800/tools/w800/csky/bin:
	mkdir -p sdk/OpenW800/tools/w800/csky
	# cd sdk/OpenW800/tools/w800/csky && wget -q "https://occ-oss-prod.oss-cn-hangzhou.aliyuncs.com/resource/1356021/1619529419771/csky-elf-noneabiv2-tools-x86_64-newlib-20210423.tar.gz" && tar -xf *.tar.gz && rm -f *.tar.gz
	if [ ! -e sdk/OpenW800/tools/w800/csky/got_csky-elf-noneabiv2-tools-x86_64-newlib-20250328 ]; then cd sdk/OpenW800/tools/w800/csky && tar -xf *.tar.gz && touch got_csky-elf-noneabiv2-tools-x86_64-newlib-20250328; fi

sdk/OpenW600/tools/gcc-arm-none-eabi-4_9-2015q1/bin:
	git submodule update --init --depth=1 sdk/OpenBK7231T

.PHONY: OpenW800
OpenW800: prebuild_OpenW800 sdk/OpenW800/tools/w800/csky/bin sdk/OpenW800/sharedAppContainer/sharedApp
	# if building new version, make sure "new_http.o" is deleted (it contains build time and version, so build time is set to actual time)
	rm -rf sdk/OpenW800/bin/build/w800/obj/sharedAppContainer/sharedApp/src/httpserver/new_http.o
	# define APP_Version so it's not "W800_Test" every time
	$(MAKE) -C sdk/OpenW800 OBK_VARIANT=$(OBK_VARIANT) CONFIG_W800_USE_LIB=n CONFIG_W800_TOOLCHAIN_PATH="$(shell realpath sdk/OpenW800/tools/w800/csky/bin)/"
	mkdir -p output/$(APP_VERSION)
	cp sdk/OpenW800/bin/w800/w800.fls output/$(APP_VERSION)/OpenW800_$(APP_VERSION).fls
	cp sdk/OpenW800/bin/w800/w800_ota.img output/$(APP_VERSION)/OpenW800_$(APP_VERSION)_ota.img

.PHONY: OpenW600
OpenW600: prebuild_OpenW600 sdk/OpenW600/tools/gcc-arm-none-eabi-4_9-2015q1/bin sdk/OpenW600/sharedAppContainer/sharedApp
	$(MAKE) -C sdk/OpenW600 TOOL_CHAIN_PATH="$(shell realpath sdk/OpenBK7231T/platforms/bk7231t/toolchain/gcc-arm-none-eabi-4_9-2015q1/bin)/" APP_VERSION=$(APP_VERSION) OBK_VARIANT=$(OBK_VARIANT)
	mkdir -p output/$(APP_VERSION)
	cp sdk/OpenW600/bin/w600/w600.fls output/$(APP_VERSION)/OpenW600_$(APP_VERSION).fls
	cp sdk/OpenW600/bin/w600/w600_gz.img output/$(APP_VERSION)/OpenW600_$(APP_VERSION)_gz.img

.PHONY: OpenLN882H
OpenLN882H: prebuild_OpenLN882H sdk/OpenLN882H/project/OpenBeken/app
	CROSS_TOOLCHAIN_ROOT=$(ARM_NONE_EABI_GCC_PATH)/../ APP_VERSION=$(APP_VERSION) OBK_VARIANT=$(OBK_VARIANT) cmake sdk/OpenLN882H -B sdk/OpenLN882H/build
	CROSS_TOOLCHAIN_ROOT=$(ARM_NONE_EABI_GCC_PATH)/../ APP_VERSION=$(APP_VERSION) OBK_VARIANT=$(OBK_VARIANT) cmake --build ./sdk/OpenLN882H/build -j $(shell nproc)
	mkdir -p output/$(APP_VERSION)
	cp sdk/OpenLN882H/build/bin/flashimage.bin output/$(APP_VERSION)/OpenLN882H_$(APP_VERSION).bin
	cp sdk/OpenLN882H/build/bin/flashimage-ota-xz-v0.1.bin output/$(APP_VERSION)/OpenLN882H_$(APP_VERSION)_OTA.bin

.PHONY: OpenESP32
OpenESP32: prebuild_ESPIDF
	IDF_TARGET="esp32" APP_VERSION=$(APP_VERSION) OBK_VARIANT=$(OBK_VARIANT) cmake platforms/ESP-IDF -B platforms/ESP-IDF/build-32 
	IDF_TARGET="esp32" APP_VERSION=$(APP_VERSION) OBK_VARIANT=$(OBK_VARIANT) cmake --build ./platforms/ESP-IDF/build-32 -j $(shell nproc)
	mkdir -p output/$(APP_VERSION)
	python3 -m esptool -c esp32 merge_bin -o output/$(APP_VERSION)/OpenESP32_$(APP_VERSION).factory.bin --flash_mode dio --flash_size $(ESP_FSIZE) 0x1000 ./platforms/ESP-IDF/build-32/bootloader/bootloader.bin 0x8000 ./platforms/ESP-IDF/build-32/partition_table/partition-table.bin 0x10000 ./platforms/ESP-IDF/build-32/OpenBeken.bin
	cp ./platforms/ESP-IDF/build-32/OpenBeken.bin output/$(APP_VERSION)/OpenESP32_$(APP_VERSION).img

.PHONY: OpenESP32C3
OpenESP32C3: prebuild_ESPIDF
	IDF_TARGET="esp32c3" APP_VERSION=$(APP_VERSION) OBK_VARIANT=$(OBK_VARIANT) cmake platforms/ESP-IDF -B platforms/ESP-IDF/build-c3 
	IDF_TARGET="esp32c3" APP_VERSION=$(APP_VERSION) OBK_VARIANT=$(OBK_VARIANT) cmake --build ./platforms/ESP-IDF/build-c3 -j $(shell nproc)
	mkdir -p output/$(APP_VERSION)
	python3 -m esptool -c esp32c3 merge_bin -o output/$(APP_VERSION)/OpenESP32C3_$(APP_VERSION).factory.bin --flash_mode dio --flash_size $(ESP_FSIZE) 0x0 ./platforms/ESP-IDF/build-c3/bootloader/bootloader.bin 0x8000 ./platforms/ESP-IDF/build-c3/partition_table/partition-table.bin 0x10000 ./platforms/ESP-IDF/build-c3/OpenBeken.bin
	cp ./platforms/ESP-IDF/build-c3/OpenBeken.bin output/$(APP_VERSION)/OpenESP32C3_$(APP_VERSION).img

.PHONY: OpenESP32C2
OpenESP32C2: prebuild_ESPIDF
	IDF_TARGET="esp32c2" APP_VERSION=$(APP_VERSION) OBK_VARIANT=$(OBK_VARIANT) cmake platforms/ESP-IDF -B platforms/ESP-IDF/build-c2 
	IDF_TARGET="esp32c2" APP_VERSION=$(APP_VERSION) OBK_VARIANT=$(OBK_VARIANT) cmake --build ./platforms/ESP-IDF/build-c2 -j $(shell nproc)
	mkdir -p output/$(APP_VERSION)
	python3 -m esptool -c esp32c2 merge_bin -o output/$(APP_VERSION)/OpenESP32C2_$(APP_VERSION).factory.bin --flash_mode dio --flash_size $(ESP_FSIZE) 0x0 ./platforms/ESP-IDF/build-c2/bootloader/bootloader.bin 0x8000 ./platforms/ESP-IDF/build-c2/partition_table/partition-table.bin 0x10000 ./platforms/ESP-IDF/build-c2/OpenBeken.bin
	cp ./platforms/ESP-IDF/build-c2/OpenBeken.bin output/$(APP_VERSION)/OpenESP32C2_$(APP_VERSION).img

.PHONY: OpenESP32C5
OpenESP32C5: prebuild_ESPIDF
	IDF_TARGET="esp32c5" APP_VERSION=$(APP_VERSION) OBK_VARIANT=$(OBK_VARIANT) cmake platforms/ESP-IDF -B platforms/ESP-IDF/build-c5 
	IDF_TARGET="esp32c5" APP_VERSION=$(APP_VERSION) OBK_VARIANT=$(OBK_VARIANT) cmake --build ./platforms/ESP-IDF/build-c5 -j $(shell nproc)
	mkdir -p output/$(APP_VERSION)
	python3 -m esptool -c esp32c5 merge_bin -o output/$(APP_VERSION)/OpenESP32C5_$(APP_VERSION).factory.bin --flash_mode dio --flash_size $(ESP_FSIZE) 0x2000 ./platforms/ESP-IDF/build-c5/bootloader/bootloader.bin 0x8000 ./platforms/ESP-IDF/build-c5/partition_table/partition-table.bin 0x10000 ./platforms/ESP-IDF/build-c5/OpenBeken.bin
	cp ./platforms/ESP-IDF/build-c5/OpenBeken.bin output/$(APP_VERSION)/OpenESP32C5_$(APP_VERSION).img

.PHONY: OpenESP32C61
OpenESP32C61: prebuild_ESPIDF
	IDF_TARGET="esp32c61" APP_VERSION=$(APP_VERSION) OBK_VARIANT=$(OBK_VARIANT) cmake platforms/ESP-IDF -B platforms/ESP-IDF/build-c61 
	IDF_TARGET="esp32c61" APP_VERSION=$(APP_VERSION) OBK_VARIANT=$(OBK_VARIANT) cmake --build ./platforms/ESP-IDF/build-c61 -j $(shell nproc)
	mkdir -p output/$(APP_VERSION)
	python3 -m esptool -c esp32c61 merge_bin -o output/$(APP_VERSION)/OpenESP32C61_$(APP_VERSION).factory.bin --flash_mode dio --flash_size $(ESP_FSIZE) 0x0 ./platforms/ESP-IDF/build-c61/bootloader/bootloader.bin 0x8000 ./platforms/ESP-IDF/build-c61/partition_table/partition-table.bin 0x10000 ./platforms/ESP-IDF/build-c61/OpenBeken.bin
	cp ./platforms/ESP-IDF/build-c61/OpenBeken.bin output/$(APP_VERSION)/OpenESP32C61_$(APP_VERSION).img

.PHONY: OpenESP32C6
OpenESP32C6: prebuild_ESPIDF
	IDF_TARGET="esp32c6" APP_VERSION=$(APP_VERSION) OBK_VARIANT=$(OBK_VARIANT) cmake platforms/ESP-IDF -B platforms/ESP-IDF/build-c6 
	IDF_TARGET="esp32c6" APP_VERSION=$(APP_VERSION) OBK_VARIANT=$(OBK_VARIANT) cmake --build ./platforms/ESP-IDF/build-c6 -j $(shell nproc)
	mkdir -p output/$(APP_VERSION)
	python3 -m esptool -c esp32c6 merge_bin -o output/$(APP_VERSION)/OpenESP32C6_$(APP_VERSION).factory.bin --flash_mode dio --flash_size $(ESP_FSIZE) 0x0 ./platforms/ESP-IDF/build-c6/bootloader/bootloader.bin 0x8000 ./platforms/ESP-IDF/build-c6/partition_table/partition-table.bin 0x10000 ./platforms/ESP-IDF/build-c6/OpenBeken.bin
	cp ./platforms/ESP-IDF/build-c6/OpenBeken.bin output/$(APP_VERSION)/OpenESP32C6_$(APP_VERSION).img

.PHONY: OpenESP32S2
OpenESP32S2: prebuild_ESPIDF
	IDF_TARGET="esp32s2" APP_VERSION=$(APP_VERSION) OBK_VARIANT=$(OBK_VARIANT) cmake platforms/ESP-IDF -B platforms/ESP-IDF/build-s2 
	IDF_TARGET="esp32s2" APP_VERSION=$(APP_VERSION) OBK_VARIANT=$(OBK_VARIANT) cmake --build ./platforms/ESP-IDF/build-s2 -j $(shell nproc)
	mkdir -p output/$(APP_VERSION)
	python3 -m esptool -c esp32s2 merge_bin -o output/$(APP_VERSION)/OpenESP32S2_$(APP_VERSION).factory.bin --flash_mode dio --flash_size $(ESP_FSIZE) 0x1000 ./platforms/ESP-IDF/build-s2/bootloader/bootloader.bin 0x8000 ./platforms/ESP-IDF/build-s2/partition_table/partition-table.bin 0x10000 ./platforms/ESP-IDF/build-s2/OpenBeken.bin
	cp ./platforms/ESP-IDF/build-s2/OpenBeken.bin output/$(APP_VERSION)/OpenESP32S2_$(APP_VERSION).img

.PHONY: OpenESP32S3
OpenESP32S3: prebuild_ESPIDF
	IDF_TARGET="esp32s3" APP_VERSION=$(APP_VERSION) OBK_VARIANT=$(OBK_VARIANT) cmake platforms/ESP-IDF -B platforms/ESP-IDF/build-s3 
	IDF_TARGET="esp32s3" APP_VERSION=$(APP_VERSION) OBK_VARIANT=$(OBK_VARIANT) cmake --build ./platforms/ESP-IDF/build-s3 -j $(shell nproc)
	mkdir -p output/$(APP_VERSION)
	python3 -m esptool -c esp32s3 merge_bin -o output/$(APP_VERSION)/OpenESP32S3_$(APP_VERSION).factory.bin --flash_mode dio --flash_size $(ESP_FSIZE) 0x0 ./platforms/ESP-IDF/build-s3/bootloader/bootloader.bin 0x8000 ./platforms/ESP-IDF/build-s3/partition_table/partition-table.bin 0x10000 ./platforms/ESP-IDF/build-s3/OpenBeken.bin
	cp ./platforms/ESP-IDF/build-s3/OpenBeken.bin output/$(APP_VERSION)/OpenESP32S3_$(APP_VERSION).img

.PHONY: OpenESP8266
OpenESP8266: prebuild_ESP8266
	APP_VERSION=$(APP_VERSION) OBK_VARIANT=$(OBK_VARIANT) cmake platforms/ESP8266 -B platforms/ESP8266/build
	APP_VERSION=$(APP_VERSION) OBK_VARIANT=$(OBK_VARIANT) cmake --build ./platforms/ESP8266/build -j $(shell nproc)
	mkdir -p output/$(APP_VERSION)
	python3 -m esptool -c esp8266 merge_bin -o output/$(APP_VERSION)/OpenESP8266_2MB_$(APP_VERSION).factory.bin --flash_mode dout --flash_size 2MB 0x0 ./platforms/ESP8266/build/bootloader/bootloader.bin 0x8000 ./platforms/ESP8266/build/partition_table/partition-table.bin 0x10000 ./platforms/ESP8266/build/OpenBeken.bin
	cp ./platforms/ESP8266/build/OpenBeken.bin output/$(APP_VERSION)/OpenESP8266_$(APP_VERSION).img
	-rm platforms/ESP-IDF/partitions.csv
	cp platforms/ESP8266/partitions-1mb.csv platforms/ESP8266/partitions.csv
	cd platforms/ESP8266/ && idf.py partition_table
	python3 -m esptool -c esp8266 merge-bin -o output/$(APP_VERSION)/OpenESP8266_1MB_$(APP_VERSION).factory.bin --flash-mode dout --flash-size 1MB 0x0 ./platforms/ESP8266/build/bootloader/bootloader.bin 0x8000 ./platforms/ESP8266/build/partition_table/partition-table.bin 0x10000 ./platforms/ESP8266/build/OpenBeken.bin
	
.PHONY: OpenTR6260
OpenTR6260: prebuild_OpenTR6260
	if [ ! -e sdk/OpenTR6260/toolchain/nds32le-elf-mculib-v3 ]; then cd sdk/OpenTR6260/toolchain && xz -d < nds32le-elf-mculib-v3.txz | tar xvf - > /dev/null; fi
	cd sdk/OpenTR6260/scripts && APP_VERSION=$(APP_VERSION) OBK_VARIANT=$(OBK_VARIANT) bash build_tr6260s1.sh
	mkdir -p output/$(APP_VERSION)
	cp sdk/OpenTR6260/out/tr6260s1/standalone/tr6260s1_0x007000.bin output/$(APP_VERSION)/OpenTR6260_$(APP_VERSION).bin
	
.PHONY: OpenRTL87X0C
OpenRTL87X0C: prebuild_OpenRTL87X0C
	$(MAKE) -C sdk/OpenRTL87X0C/project/OpenBeken/GCC-RELEASE APP_VERSION=$(APP_VERSION) OBK_VARIANT=$(OBK_VARIANT) -j $(shell nproc)
	mkdir -p output/$(APP_VERSION)
	cp sdk/OpenRTL87X0C/project/OpenBeken/GCC-RELEASE/application_is/Debug/bin/flash_is.bin output/$(APP_VERSION)/OpenRTL87X0C_$(APP_VERSION).bin
	cp sdk/OpenRTL87X0C/project/OpenBeken/GCC-RELEASE/application_is/Debug/bin/firmware_is.bin output/$(APP_VERSION)/OpenRTL87X0C_$(APP_VERSION)_ota.img
	
.PHONY: OpenRTL8710B
OpenRTL8710B: prebuild_OpenRTL8710B
	$(MAKE) -C sdk/OpenRTL8710A_B/project/obk_amebaz/GCC-RELEASE APP_VERSION=$(APP_VERSION) OBK_VARIANT=$(OBK_VARIANT) -j $(shell nproc)
	$(MAKE) -C sdk/OpenRTL8710A_B/project/obk_amebaz/GCC-RELEASE APP_VERSION=$(APP_VERSION) OBK_VARIANT=$(OBK_VARIANT) ota_idx=2
	mkdir -p output/$(APP_VERSION)
	dd conv=notrunc bs=1K if=sdk/OpenRTL8710A_B/project/obk_amebaz/GCC-RELEASE/application/Debug/bin/boot_all.bin of=output/$(APP_VERSION)/OpenRTL8710B_$(APP_VERSION).bin seek=0
	dd conv=notrunc bs=1K if=sdk/OpenRTL8710A_B/project/obk_amebaz/GCC-RELEASE/application/Debug/bin/image2_all_ota1.bin of=output/$(APP_VERSION)/OpenRTL8710B_$(APP_VERSION).bin seek=44
	./platforms/RTL8710B/tools/amebaz_ota_combine sdk/OpenRTL8710A_B/project/obk_amebaz/GCC-RELEASE/application/Debug/bin/image2_all_ota1.bin sdk/OpenRTL8710A_B/project/obk_amebaz/GCC-RELEASE/application/Debug/bin/image2_all_ota2.bin output/$(APP_VERSION)/OpenRTL8710B_$(APP_VERSION)_ota.img
	./platforms/RTL8710B/tools/amebaz_ug_from_ota output/$(APP_VERSION)/OpenRTL8710B_$(APP_VERSION)_ota.img output/$(APP_VERSION)/OpenRTL8710B_UG_$(APP_VERSION).img

.PHONY: OpenRTL8710A
OpenRTL8710A: prebuild_OpenRTL8710A
	$(MAKE) -C sdk/OpenRTL8710A_B/project/obk_ameba1/GCC-RELEASE APP_VERSION=$(APP_VERSION) OBK_VARIANT=$(OBK_VARIANT) -j $(shell nproc)
	mkdir -p output/$(APP_VERSION)
	cp sdk/OpenRTL8710A_B/project/obk_ameba1/GCC-RELEASE/application/Debug/bin/ram_all.bin output/$(APP_VERSION)/OpenRTL8710A_$(APP_VERSION).bin
	cp sdk/OpenRTL8710A_B/project/obk_ameba1/GCC-RELEASE/application/Debug/bin/ota.bin output/$(APP_VERSION)/OpenRTL8710A_$(APP_VERSION)_ota.img
	
.PHONY: OpenRTL8720D
OpenRTL8720D: prebuild_OpenRTL8720D
	$(MAKE) -C sdk/OpenRTL8720D/project/OpenBeken/GCC-RELEASE/project_hp --no-print-directory APP_VERSION=$(APP_VERSION) OBK_VARIANT=$(OBK_VARIANT)
	$(MAKE) -C sdk/OpenRTL8720D/project/OpenBeken/GCC-RELEASE/project_lp --no-print-directory APP_VERSION=$(APP_VERSION) OBK_VARIANT=$(OBK_VARIANT)
	mkdir -p output/$(APP_VERSION)
	touch output/$(APP_VERSION)/OpenRTL8720D_$(APP_VERSION).bin
	dd conv=notrunc bs=1K if=sdk/OpenRTL8720D/project/OpenBeken/GCC-RELEASE/project_lp/asdk/image/km0_boot_all.bin of=output/$(APP_VERSION)/OpenRTL8720D_$(APP_VERSION).bin seek=0
	dd conv=notrunc bs=1K if=sdk/OpenRTL8720D/project/OpenBeken/GCC-RELEASE/project_hp/asdk/image/km4_boot_all.bin of=output/$(APP_VERSION)/OpenRTL8720D_$(APP_VERSION).bin seek=16
	dd conv=notrunc bs=1K if=sdk/OpenRTL8720D/project/OpenBeken/GCC-RELEASE/project_hp/asdk/image/km0_km4_image2.bin of=output/$(APP_VERSION)/OpenRTL8720D_$(APP_VERSION).bin seek=24
	cp sdk/OpenRTL8720D/project/OpenBeken/GCC-RELEASE/project_lp/asdk/image/OTA_All.bin output/$(APP_VERSION)/OpenRTL8720D_$(APP_VERSION)_ota.img

.PHONY: OpenRTL8721DA
OpenRTL8721DA: prebuild_OpenRTL8721DA
	cd sdk/ameba-rtos/amebadplus_gcc_project && APP_VERSION=$(APP_VERSION) OBK_VARIANT=$(OBK_VARIANT) ./build.py -a ../../../platforms/RTL8721DA
	mkdir -p output/$(APP_VERSION)
	cp sdk/ameba-rtos/amebadplus_gcc_project/km4_boot_all.bin /tmp/OpenRTL8721DA_$(APP_VERSION).bin
	dd conv=notrunc bs=1K if=sdk/ameba-rtos/amebadplus_gcc_project/km0_km4_app.bin of=/tmp/OpenRTL8721DA_$(APP_VERSION).bin seek=80
	mv /tmp/OpenRTL8721DA_$(APP_VERSION).bin output/$(APP_VERSION)/
	cp sdk/ameba-rtos/amebadplus_gcc_project/ota_all.bin output/$(APP_VERSION)/OpenRTL8721DA_$(APP_VERSION)_ota.img

.PHONY: OpenRTL8720E
OpenRTL8720E: prebuild_OpenRTL8720E
	cd sdk/ameba-rtos/amebalite_gcc_project && APP_VERSION=$(APP_VERSION) OBK_VARIANT=$(OBK_VARIANT) ./build.py -a ../../../platforms/RTL8720E
	mkdir -p output/$(APP_VERSION)
	cp sdk/ameba-rtos/amebalite_gcc_project/km4_boot_all.bin /tmp/OpenRTL8720E_$(APP_VERSION).bin
	dd conv=notrunc bs=1K if=sdk/ameba-rtos/amebalite_gcc_project/kr4_km4_app.bin of=/tmp/OpenRTL8720E_$(APP_VERSION).bin seek=80
	mv /tmp/OpenRTL8720E_$(APP_VERSION).bin output/$(APP_VERSION)/
	cp sdk/ameba-rtos/amebalite_gcc_project/ota_all.bin output/$(APP_VERSION)/OpenRTL8720E_$(APP_VERSION)_ota.img

.PHONY: OpenBK7238
OpenBK7238: prebuild_OpenBK7238
	cd sdk/beken_freertos_sdk && OBK_VARIANT=$(OBK_VARIANT) sh build.sh bk7238 $(APP_VERSION)
	mkdir -p output/$(APP_VERSION)
	cp sdk/beken_freertos_sdk/out/bk7238.bin output/$(APP_VERSION)/OpenBK7238_${APP_VERSION}.bin
	cp sdk/beken_freertos_sdk/out/bk7238_QIO.bin output/$(APP_VERSION)/OpenBK7238_QIO_${APP_VERSION}.bin
	cp sdk/beken_freertos_sdk/out/app.rbl output/$(APP_VERSION)/OpenBK7238_${APP_VERSION}.rbl
	cp sdk/beken_freertos_sdk/out/bk7238_UA.bin output/$(APP_VERSION)/OpenBK7238_UA_${APP_VERSION}.bin

.PHONY: OpenBK7231U
OpenBK7231U: prebuild_OpenBK7231U
	cd sdk/beken_freertos_sdk && sh build.sh bk7231u $(APP_VERSION)
	mkdir -p output/$(APP_VERSION)
	cp sdk/beken_freertos_sdk/out/bk7231u_QIO.bin output/$(APP_VERSION)/OpenBK7231U_QIO_${APP_VERSION}.bin
	cp sdk/beken_freertos_sdk/out/bk7231u.bin output/$(APP_VERSION)/OpenBK7231U_${APP_VERSION}.bin
	cp sdk/beken_freertos_sdk/out/app.rbl output/$(APP_VERSION)/OpenBK7231U_${APP_VERSION}.rbl
	cp sdk/beken_freertos_sdk/out/bk7231u_UA.bin output/$(APP_VERSION)/OpenBK7231U_UA_${APP_VERSION}.bin

.PHONY: OpenBK7252
OpenBK7252: prebuild_OpenBK7252
	cd sdk/beken_freertos_sdk && sh build.sh bk7251 $(APP_VERSION)
	mkdir -p output/$(APP_VERSION)
	cp sdk/beken_freertos_sdk/out/bk7251.bin output/$(APP_VERSION)/OpenBK7252_${APP_VERSION}.bin
	cp sdk/beken_freertos_sdk/out/bk7251_QIO.bin output/$(APP_VERSION)/OpenBK7252_QIO_${APP_VERSION}.bin
	cp sdk/beken_freertos_sdk/out/app.rbl output/$(APP_VERSION)/OpenBK7252_${APP_VERSION}.rbl
	cp sdk/beken_freertos_sdk/out/bk7251_UA.bin output/$(APP_VERSION)/OpenBK7252_UA_${APP_VERSION}.bin
	cp sdk/beken_freertos_sdk/out/bk7251_Tuya_QIO.bin output/$(APP_VERSION)/OpenBK7252_Tuya_QIO_${APP_VERSION}.bin
	cp sdk/beken_freertos_sdk/out/bk7251_Tuya_UA.bin output/$(APP_VERSION)/OpenBK7252_Tuya_UA_${APP_VERSION}.bin

.PHONY: OpenBK7252N
OpenBK7252N: prebuild_OpenBK7252N
	cd sdk/beken_freertos_sdk && sh build.sh bk7252n $(APP_VERSION)
	mkdir -p output/$(APP_VERSION)
	cp sdk/beken_freertos_sdk/out/bk7252n.bin output/$(APP_VERSION)/OpenBK7252N_${APP_VERSION}.bin
	cp sdk/beken_freertos_sdk/out/bk7252n_QIO.bin output/$(APP_VERSION)/OpenBK7252N_QIO_${APP_VERSION}.bin
	cp sdk/beken_freertos_sdk/out/app.rbl output/$(APP_VERSION)/OpenBK7252N_${APP_VERSION}.rbl
	cp sdk/beken_freertos_sdk/out/bk7252n_UA.bin output/$(APP_VERSION)/OpenBK7252N_UA_${APP_VERSION}.bin
	#cp sdk/beken_freertos_sdk/out/bk7252n_Tuya_QIO.bin output/$(APP_VERSION)/OpenBK7252N_Tuya_QIO_${APP_VERSION}.bin
	#cp sdk/beken_freertos_sdk/out/bk7252n_Tuya_UA.bin output/$(APP_VERSION)/OpenBK7252N_Tuya_UA_${APP_VERSION}.bin

.PHONY: OpenBK7231N_ALT
OpenBK7231N_ALT: prebuild_OpenBK7231N_ALT
	cd sdk/beken_freertos_sdk && OBK_VARIANT=$(OBK_VARIANT) sh build.sh bk7231n $(APP_VERSION)_ALT
	mkdir -p output/$(APP_VERSION)
	cp sdk/beken_freertos_sdk/out/bk7231n_QIO.bin output/$(APP_VERSION)/OpenBK7231N_ALT_QIO_${APP_VERSION}.bin
	cp sdk/beken_freertos_sdk/out/bk7231n.bin output/$(APP_VERSION)/OpenBK7231N_ALT_${APP_VERSION}.bin
	cp sdk/beken_freertos_sdk/out/app.rbl output/$(APP_VERSION)/OpenBK7231N_ALT_${APP_VERSION}.rbl
	cp sdk/beken_freertos_sdk/out/bk7231n_UA.bin output/$(APP_VERSION)/OpenBK7231N_ALT_UA_${APP_VERSION}.bin
	cp sdk/beken_freertos_sdk/out/BK7231M_QIO.bin output/$(APP_VERSION)/OpenBK7231M_ALT_QIO_${APP_VERSION}.bin

.PHONY: OpenBK7231T_ALT
OpenBK7231T_ALT: prebuild_OpenBK7231T_ALT
	cd sdk/beken_freertos_sdk && sh build.sh bk7231 $(APP_VERSION)_ALT
	mkdir -p output/$(APP_VERSION)
	cp sdk/beken_freertos_sdk/out/bk7231t_QIO.bin output/$(APP_VERSION)/OpenBK7231T_ALT_QIO_${APP_VERSION}.bin
	cp sdk/beken_freertos_sdk/out/bk7231u.bin output/$(APP_VERSION)/OpenBK7231T_ALT_${APP_VERSION}.bin
	cp sdk/beken_freertos_sdk/out/app.rbl output/$(APP_VERSION)/OpenBK7231T_ALT_${APP_VERSION}.rbl
	cp sdk/beken_freertos_sdk/out/bk7231t_UA.bin output/$(APP_VERSION)/OpenBK7231T_ALT_UA_${APP_VERSION}.bin
	
.PHONY: OpenECR6600
ECRDIR := $(PWD)/sdk/OpenECR6600
OpenECR6600: prebuild_OpenECR6600
	if [ ! -e sdk/OpenECR6600/tool/nds32le-elf-mculib-v3s ]; then cd sdk/OpenECR6600/tool && xz -d < nds32le-elf-mculib-v3s.txz | tar xvf - > /dev/null; fi
	cd sdk/OpenECR6600 && make BOARD_DIR=$(ECRDIR)/Boards/ecr6600/standalone APP_NAME=OpenBeken TOPDIR=$(ECRDIR) GCC_PATH=$(ECRDIR)/tool/nds32le-elf-mculib-v3s/bin/ APP_VERSION=$(APP_VERSION) OBK_VARIANT=$(OBK_VARIANT) defconfig
	cd sdk/OpenECR6600 && make BOARD_DIR=$(ECRDIR)/Boards/ecr6600/standalone APP_NAME=OpenBeken TOPDIR=$(ECRDIR) GCC_PATH=$(ECRDIR)/tool/nds32le-elf-mculib-v3s/bin/ APP_VERSION=$(APP_VERSION) OBK_VARIANT=$(OBK_VARIANT) REL_V=OpenECR6600_$(APP_VERSION) all
	mkdir -p output/$(APP_VERSION)
	cp $(ECRDIR)/build/OpenBeken/ECR6600F_standalone_OpenBeken_allinone.bin output/$(APP_VERSION)/OpenECR6600_$(APP_VERSION).bin
	cp $(ECRDIR)/build/OpenBeken/ECR6600F_OpenBeken_Compress_ota_packeg.bin output/$(APP_VERSION)/OpenECR6600_$(APP_VERSION)_ota.img
	
.PHONY: OpenTXW81X
OpenTXW81X: prebuild_OpenTXW81X
	cd sdk/OpenTXW81X/project && make APP_VERSION=$(APP_VERSION) OBK_VARIANT=$(OBK_VARIANT) -j $(shell nproc) && \
	./BinScript.exe BinScript.BinScript > /dev/null && ./makecode.exe > /dev/null
	mkdir -p output/$(APP_VERSION)
	cp sdk/OpenTXW81X/project/APP.bin output/$(APP_VERSION)/OpenTXW81X_$(APP_VERSION).bin
	cp sdk/OpenTXW81X/project/APP_compress.bin output/$(APP_VERSION)/OpenTXW81X_$(APP_VERSION)_ota.img
	
.PHONY: OpenRDA5981
OpenRDA5981: prebuild_OpenRDA5981
	$(MAKE) -C sdk/OpenRDA5981 APP_VERSION=$(APP_VERSION) OBK_VARIANT=$(OBK_VARIANT) -j $(shell nproc)
	mkdir -p output/$(APP_VERSION)
	#cp sdk/OpenRDA5981/.build/OpenBeken.bin output/$(APP_VERSION)/OpenRDA5981_$(APP_VERSION).img
	python3 sdk/OpenRDA5981/ota_lzma/image_pack_firmware.py sdk/OpenRDA5981/.build/OpenBeken.bin $(APP_VERSION) 0
	cp sdk/OpenRDA5981/ota_lzma/bootloader.bin output/$(APP_VERSION)/OpenRDA5981_$(APP_VERSION).bin
	dd conv=notrunc bs=1K if=sdk/OpenRDA5981/.build/OpenBeken_fwpacked.bin of=output/$(APP_VERSION)/OpenRDA5981_$(APP_VERSION).bin seek=8
	#./sdk/OpenRDA5981/ota_lzma/imgpkt e sdk/OpenRDA5981/.build/OpenBeken.bin sdk/OpenRDA5981/.build/OpenBeken.bin.lzma
	./sdk/OpenRDA5981/ota_lzma/xz --format=lzma -A -z -k -v -c sdk/OpenRDA5981/.build/OpenBeken.bin > sdk/OpenRDA5981/.build/OpenBeken.bin.lzma
	python3 sdk/OpenRDA5981/ota_lzma/ota_pack_image_lzma.py sdk/OpenRDA5981/.build/OpenBeken.bin sdk/OpenRDA5981/.build/OpenBeken.bin.lzma output/$(APP_VERSION)/OpenRDA5981_$(APP_VERSION)_ota.img $(APP_VERSION)

# Add custom Makefile if required
-include custom.mk

# clean .o files and output directory
.PHONY: clean
clean: 
	-test -d ./sdk/OpenXR809 && $(MAKE) -C sdk/OpenXR809/src clean
	-test -d ./sdk/OpenXR809 && $(MAKE) -C sdk/OpenXR809/project/oxr_sharedApp/gcc clean
	-test -d ./sdk/OpenXR806 && $(MAKE) -C sdk/OpenXR806/src clean
	-test -d ./sdk/OpenXR806 && $(MAKE) -C sdk/OpenXR806/project/demo/sharedApp/gcc clean
	-test -d ./sdk/OpenXR872 && $(MAKE) -C sdk/OpenXR872/src clean
	-test -d ./sdk/OpenXR872 && $(MAKE) -C sdk/OpenXR872/project/demo/hello_demo/gcc clean
	-test -d ./sdk/OpenW800 && $(MAKE) -C sdk/OpenW800 clean
	-test -d ./sdk/OpenW600 && $(MAKE) -C sdk/OpenW600 clean
	-test -d ./sdk/OpenTR6260 && $(MAKE) -C sdk/OpenTR6260/scripts tr6260s1_clean
	-test -d ./sdk/OpenRTL87X0C && $(MAKE) -C sdk/OpenRTL87X0C/project/OpenBeken/GCC-RELEASE clean
	-test -d ./sdk/OpenRTL8710A_B && $(MAKE) -C sdk/OpenRTL8710A_B/project/obk_amebaz/GCC-RELEASE clean
	-test -d ./sdk/OpenRTL8710A_B && $(MAKE) -C sdk/OpenRTL8710A_B/project/obk_ameba1/GCC-RELEASE clean
	-test -d ./sdk/OpenRTL8720D && $(MAKE) -C sdk/OpenRTL8720D/project/OpenBeken/GCC-RELEASE/project_hp clean
	-test -d ./sdk/OpenRTL8720D && $(MAKE) -C sdk/OpenRTL8720D/project/OpenBeken/GCC-RELEASE/project_lp clean
	-test -d ./sdk/ameba-rtos && cd sdk/ameba-rtos/amebadplus_gcc_project && ./build.py -a ../../../platforms/RTL8721DA -c
	-test -d ./sdk/ameba-rtos && cd sdk/ameba-rtos/amebalite_gcc_project && ./build.py -a ../../../platforms/RTL8720E -c
	-test -d ./sdk/beken_freertos_sdk && $(MAKE) -C sdk/beken_freertos_sdk clean
	-test -d ./sdk/OpenTXW81X && $(MAKE) -C sdk/OpenTXW81X/project clean
	-test -d ./sdk/OpenRDA5981 && $(MAKE) -C sdk/OpenRDA5981 clean
	-test -d ./sdk/OpenLN882H/build && cmake --build ./sdk/OpenLN882H/build --target clean
	-test -d ./platforms/ESP-IDF/build-32 && cmake --build ./platforms/ESP-IDF/build-32 --target clean
	-test -d ./platforms/ESP-IDF/build-c3 && cmake --build ./platforms/ESP-IDF/build-c3 --target clean
	-test -d ./platforms/ESP-IDF/build-c2 && cmake --build ./platforms/ESP-IDF/build-c2 --target clean
	-test -d ./platforms/ESP-IDF/build-c6 && cmake --build ./platforms/ESP-IDF/build-c6 --target clean
	-test -d ./platforms/ESP-IDF/build-s2 && cmake --build ./platforms/ESP-IDF/build-s2 --target clean
	-test -d ./platforms/ESP-IDF/build-s3 && cmake --build ./platforms/ESP-IDF/build-s3 --target clean
	-test -d ./platforms/ESP-IDF/build-c5 && cmake --build ./platforms/ESP-IDF/build-c5 --target clean
	-test -d ./platforms/ESP-IDF/build-c61 && cmake --build ./platforms/ESP-IDF/build-c61 --target clean
	-test -d ./platforms/ESP8266/build && cmake --build ./platforms/ESP8266/build --target clean
	-test -d ./sdk/OpenECR6600 && cd sdk/OpenECR6600 && make BOARD_DIR=$(ECRDIR)/Boards/ecr6600/standalone APP_NAME=OpenBeken TOPDIR=$(ECRDIR) GCC_PATH=$(ECRDIR)/tool/nds32le-elf-mculib-v3s/bin/ clean
	-test -d ./sdk/OpenBK7231T && $(MAKE) -C sdk/OpenBK7231T/platforms/bk7231t/bk7231t_os APP_BIN_NAME=$(APP_NAME) USER_SW_VER=$(APP_VERSION) clean
	-test -d ./sdk/OpenBK7231N && $(MAKE) -C sdk/OpenBK7231N/platforms/bk7231n/bk7231n_os APP_BIN_NAME=$(APP_NAME) USER_SW_VER=$(APP_VERSION) clean
	-$(RM) -r $(BUILD_DIR)

# Example upload command - import the following snippet into Node-RED and update the IPs in the variables below
# This will stage the rbl binary on the Node-RED server and trigger a OTA request to for the BK chip to pull it automatically
## [{"id":"3cd109953d170e66","type":"tab","label":"Firmware","disabled":false,"info":"","env":[]},{"id":"0f0c991791d6922a","type":"group","z":"3cd109953d170e66","name":"Return Firmware to BK7231T","style":{"label":true},"nodes":["d274dcdb0a33093b","5cb323971ddeea52","06889aec9ed22a48","f814bcf109e0ea18","630e454cbfb63211"],"x":14,"y":239,"w":852,"h":142},{"id":"2e572f2886d560c7","type":"group","z":"3cd109953d170e66","name":"OTA command - store binary and trigger device OTA request","style":{"label":true},"nodes":["748c6dd1fc3bc2e8","fb5148de5da10c42","6fd4e23a5b96d39c","6eff9c90b19282fa","4e71b18352ff8641","99f44f74dba836a4","0fdb40d65b6911da","344c7f6a5f46c69b","f564aafdaa4a9ce2"],"x":14,"y":19,"w":1212,"h":202},{"id":"7f1a0772422bfdcc","type":"group","z":"3cd109953d170e66","name":"Clear stored firmware (if required)","style":{"label":true},"nodes":["f6f80ca0dd720e30","6e619a81ddcc630f"],"x":14,"y":399,"w":452,"h":82},{"id":"d274dcdb0a33093b","type":"http in","z":"3cd109953d170e66","g":"0f0c991791d6922a","name":"","url":"/firmware","method":"get","upload":false,"swaggerDoc":"","x":140,"y":280,"wires":[["f814bcf109e0ea18","630e454cbfb63211"]]},{"id":"5cb323971ddeea52","type":"http response","z":"3cd109953d170e66","g":"0f0c991791d6922a","name":"Return Firmware Binary","statusCode":"200","headers":{},"x":730,"y":280,"wires":[]},{"id":"748c6dd1fc3bc2e8","type":"http request","z":"3cd109953d170e66","g":"2e572f2886d560c7","name":"Send OTA request to BK7231T","method":"GET","ret":"bin","paytoqs":"ignore","url":"","tls":"","persist":false,"proxy":"","authType":"","senderr":false,"x":910,"y":60,"wires":[["99f44f74dba836a4"]]},{"id":"06889aec9ed22a48","type":"debug","z":"3cd109953d170e66","g":"0f0c991791d6922a","name":"","active":false,"tosidebar":true,"console":false,"tostatus":false,"complete":"true","targetType":"full","statusVal":"","statusType":"auto","x":670,"y":340,"wires":[]},{"id":"f814bcf109e0ea18","type":"debug","z":"3cd109953d170e66","g":"0f0c991791d6922a","name":"","active":false,"tosidebar":true,"console":false,"tostatus":false,"complete":"true","targetType":"full","statusVal":"","statusType":"auto","x":330,"y":340,"wires":[]},{"id":"fb5148de5da10c42","type":"http in","z":"3cd109953d170e66","g":"2e572f2886d560c7","name":"","url":"/ota","method":"post","upload":true,"swaggerDoc":"","x":130,"y":60,"wires":[["6fd4e23a5b96d39c","4e71b18352ff8641","344c7f6a5f46c69b"]]},{"id":"6fd4e23a5b96d39c","type":"debug","z":"3cd109953d170e66","g":"2e572f2886d560c7","name":"","active":false,"tosidebar":true,"console":false,"tostatus":false,"complete":"true","targetType":"full","statusVal":"","statusType":"auto","x":330,"y":180,"wires":[]},{"id":"6eff9c90b19282fa","type":"http response","z":"3cd109953d170e66","g":"2e572f2886d560c7","name":"","statusCode":"200","headers":{},"x":620,"y":120,"wires":[]},{"id":"4e71b18352ff8641","type":"change","z":"3cd109953d170e66","g":"2e572f2886d560c7","name":"Save data and prepare OTA command","rules":[{"t":"set","p":"ota","pt":"flow","to":"req.files[0].buffer","tot":"msg","dc":true},{"t":"set","p":"filename","pt":"flow","to":"req.files[0].originalname","tot":"msg"},{"t":"set","p":"ip","pt":"flow","to":"payload.ip","tot":"msg"},{"t":"set","p":"endpoint","pt":"flow","to":"payload.endpoint","tot":"msg"},{"t":"set","p":"url","pt":"msg","to":"\"http://\"&$flowContext(\"ip\")&\"/ota_exec?host=\"&$flowContext(\"endpoint\")&\"/firmware\"","tot":"jsonata"},{"t":"delete","p":"req","pt":"msg"},{"t":"delete","p":"res","pt":"msg"}],"action":"","property":"","from":"","to":"","reg":false,"x":430,"y":60,"wires":[["0fdb40d65b6911da"]]},{"id":"630e454cbfb63211","type":"change","z":"3cd109953d170e66","g":"0f0c991791d6922a","name":"Grab firmware from flow variable","rules":[{"t":"set","p":"payload","pt":"msg","to":"ota","tot":"flow"},{"t":"set","p":"headers","pt":"msg","to":"{\t   \"Content-Type\":\"application/octet-stream\",\t   \"Content-Disposition\":\"attachment; filename=\"&$flowContext(\"filename\"),\t   \"Connection\":\"close\"\t}","tot":"jsonata"}],"action":"","property":"","from":"","to":"","reg":false,"x":420,"y":280,"wires":[["5cb323971ddeea52","06889aec9ed22a48"]]},{"id":"99f44f74dba836a4","type":"debug","z":"3cd109953d170e66","g":"2e572f2886d560c7","name":"","active":false,"tosidebar":true,"console":false,"tostatus":false,"complete":"true","targetType":"full","statusVal":"","statusType":"auto","x":1130,"y":60,"wires":[]},{"id":"f6f80ca0dd720e30","type":"inject","z":"3cd109953d170e66","g":"7f1a0772422bfdcc","name":"","props":[{"p":"payload"},{"p":"topic","vt":"str"}],"repeat":"","crontab":"","once":false,"onceDelay":0.1,"topic":"","payload":"","payloadType":"date","x":120,"y":440,"wires":[["6e619a81ddcc630f"]]},{"id":"6e619a81ddcc630f","type":"change","z":"3cd109953d170e66","g":"7f1a0772422bfdcc","name":"","rules":[{"t":"delete","p":"ota","pt":"flow"}],"action":"","property":"","from":"","to":"","reg":false,"x":360,"y":440,"wires":[[]]},{"id":"0fdb40d65b6911da","type":"delay","z":"3cd109953d170e66","g":"2e572f2886d560c7","name":"","pauseType":"delay","timeout":"2","timeoutUnits":"seconds","rate":"1","nbRateUnits":"1","rateUnits":"second","randomFirst":"1","randomLast":"5","randomUnits":"seconds","drop":false,"allowrate":false,"outputs":1,"x":680,"y":60,"wires":[["748c6dd1fc3bc2e8","f564aafdaa4a9ce2"]]},{"id":"344c7f6a5f46c69b","type":"change","z":"3cd109953d170e66","g":"2e572f2886d560c7","name":"Prepare response message","rules":[{"t":"set","p":"payload","pt":"msg","to":"\"Uploading \"&req.files[0].originalname&\" to \"&payload.ip&\"\\n\"","tot":"jsonata","dc":true}],"action":"","property":"","from":"","to":"","reg":false,"x":400,"y":120,"wires":[["6eff9c90b19282fa"]]},{"id":"f564aafdaa4a9ce2","type":"debug","z":"3cd109953d170e66","g":"2e572f2886d560c7","name":"","active":false,"tosidebar":true,"console":false,"tostatus":false,"complete":"true","targetType":"full","statusVal":"","statusType":"auto","x":830,"y":120,"wires":[]}]
NODE_RED_ENDPOINT ?= http://192.168.x.x:1880/endpoint
BK7231T_IP ?= 192.168.x.y
.PHONY: upload
upload:
	curl -F "ota=@output/$(APP_VERSION)/OpenBK7231T_$(APP_VERSION).rbl" -F "ip=$(BK7231T_IP)" -F "endpoint=$(NODE_RED_ENDPOINT)" $(NODE_RED_ENDPOINT)/ota

endif
