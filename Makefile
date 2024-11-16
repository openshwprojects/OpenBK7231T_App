# HACK - if COMPILE_PREX defined then we are being called running from original build_app.sh script in standard SDK
# Required to not break old build_app.sh script lines 74-77
ifdef COMPILE_PREX
all:
	@echo Calling original build_app.sh script
	cd $(PWD)/../../platforms/$(TARGET_PLATFORM)/toolchain/$(TUYA_APPS_BUILD_PATH) && sh $(TUYA_APPS_BUILD_CMD) $(APP_NAME) $(APP_VERSION) $(TARGET_PLATFORM) "$(USER_CMD)" $(BUILD_MODE)
else

######## Continue with custom simplied Makefile ########

# Use current folder name as app name
APP_NAME ?= $(shell basename $(CURDIR))
# Use current timestamp as version number
TIMESTAMP := $(shell date +%Y%m%d_%H%M%S)
APP_VERSION ?= dev_$(TIMESTAMP)

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
	git submodule update --init --recursive --remote
endif

update-submodules: submodules
	git add sdk/OpenBK7231T sdk/OpenBK7231N sdk/OpenXR809 sdk/OpenBL602 sdk/OpenW800 sdk/OpenW600 sdk/OpenLN882H sdk/esp-idf
ifdef GITHUB_ACTIONS
	git config user.name github-actions
	git config user.email github-actions@github.com
endif
	git commit -m "feat: update SDKs" && git push || echo "No changes to commit"

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

sdk/OpenBL602/customer_app/bl602_sharedApp/bl602_sharedApp/shared:
	@echo Create symlink for $(APP_NAME) into sdk folder
	ln -s "$(shell pwd)/" "sdk/OpenBL602/customer_app/bl602_sharedApp/bl602_sharedApp/shared"

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

.PHONY: prebuild_OpenBK7231N prebuild_OpenBK7231T prebuild_OpenBL602 prebuild_OpenLN882H prebuild_OpenW600 prebuild_OpenW800 prebuild_OpenXR809 prebuild_ESPIDF

prebuild_OpenBK7231N:
	@if [ -e platforms/BK7231N/pre_build.sh ]; then \
		echo "prebuild found for OpenBK7231N"; \
		sh platforms/BK7231N/pre_build.sh; \
	else echo "prebuild for OpenBK7231N not found ... "; \
	fi

prebuild_OpenBK7231T:
	@if [ -e platforms/BK7231T/pre_build.sh ]; then \
		echo "prebuild found for OpenBK7231T"; \
		sh platforms/BK7231T/pre_build.sh; \
	else echo "prebuild for OpenBK7231T not found ... "; \
	fi

prebuild_OpenBL602:
	@if [ -e platforms/BL602/pre_build.sh ]; then \
		echo "prebuild found for OpenBL602"; \
		sh platforms/BL602/pre_build.sh; \
	else echo "prebuild for OpenBL602 not found ... "; \
	fi

prebuild_OpenLN882H:
	@if [ -e platforms/LN882H/pre_build.sh ]; then \
		echo "prebuild found for OpenLN882H"; \
		sh platforms/LN882H/pre_build.sh; \
	else echo "prebuild for OpenLN882H not found ... "; \
	fi

prebuild_OpenW600:
	@if [ -e platforms/W600/pre_build.sh ]; then \
		echo "prebuild found for OpenW600"; \
		sh platforms/W600/pre_build.sh; \
	else echo "prebuild for OpenW600 not found ... "; \
	fi

prebuild_OpenW800:
	@if [ -e platforms/W800/pre_build.sh ]; then \
		echo "prebuild found for OpenW800"; \
		sh platforms/W800/pre_build.sh; \
	else echo "prebuild for OpenW800 not found ... "; \
	fi

prebuild_OpenXR809:
	@if [ -e platforms/XR809/pre_build.sh ]; then \
		echo "prebuild found for OpenXR809"; \
		sh platforms/XR809/pre_build.sh; \
	else echo "prebuild for OpenXR809 not found ... "; \
	fi

prebuild_ESPIDF:
	@if [ -e platforms/ESP-IDF/pre_build.sh ]; then \
		echo "prebuild found for ESP-IDF"; \
		sh platforms/ESP-IDF/pre_build.sh; \
	else echo "prebuild for ESP-IDF not found ... "; \
	fi

# Build main binaries
OpenBK7231T: prebuild_OpenBK7231T
	$(MAKE) APP_NAME=OpenBK7231T TARGET_PLATFORM=bk7231t SDK_PATH=sdk/OpenBK7231T APPS_BUILD_PATH=../bk7231t_os build-BK7231

OpenBK7231N: prebuild_OpenBK7231N
	$(MAKE) APP_NAME=OpenBK7231N TARGET_PLATFORM=bk7231n SDK_PATH=sdk/OpenBK7231N APPS_BUILD_PATH=../bk7231n_os build-BK7231

sdk/OpenXR809/tools/gcc-arm-none-eabi-4_9-2015q2:
	cd sdk/OpenXR809/tools && wget -q "https://launchpad.net/gcc-arm-embedded/4.9/4.9-2015-q2-update/+download/gcc-arm-none-eabi-4_9-2015q2-20150609-linux.tar.bz2" && tar -xf *.tar.bz2 && rm -f *.tar.bz2

	
.PHONY: OpenXR809 build-XR809
# Retry OpenXR809 a few times to account for calibration file issues
RETRY = 3
OpenXR809: prebuild_OpenXR809
	@for i in `seq 1 ${RETRY}`; do ($(MAKE) -k build-XR809; echo Prebuild attempt $$i/${RETRY}); done
	@echo Running build final time to check output
	$(MAKE) build-XR809;

build-XR809: submodules sdk/OpenXR809/project/oxr_sharedApp/shared sdk/OpenXR809/tools/gcc-arm-none-eabi-4_9-2015q2
	$(MAKE) -C sdk/OpenXR809/src CC_DIR=$(PWD)/sdk/OpenXR809/tools/gcc-arm-none-eabi-4_9-2015q2/bin
	$(MAKE) -C sdk/OpenXR809/src install CC_DIR=$(PWD)/sdk/OpenXR809/tools/gcc-arm-none-eabi-4_9-2015q2/bin
	$(MAKE) -C sdk/OpenXR809/project/oxr_sharedApp/gcc CC_DIR=$(PWD)/sdk/OpenXR809/tools/gcc-arm-none-eabi-4_9-2015q2/bin
	$(MAKE) -C sdk/OpenXR809/project/oxr_sharedApp/gcc image CC_DIR=$(PWD)/sdk/OpenXR809/tools/gcc-arm-none-eabi-4_9-2015q2/bin
	mkdir -p output/$(APP_VERSION)
	cp sdk/OpenXR809/project/oxr_sharedApp/image/xr809/xr_system.img output/$(APP_VERSION)/OpenXR809_$(APP_VERSION).img

.PHONY: build-BK7231
build-BK7231: submodules $(SDK_PATH)/apps/$(APP_NAME)
	cd $(SDK_PATH)/platforms/$(TARGET_PLATFORM)/toolchain/$(APPS_BUILD_PATH) && sh $(APPS_BUILD_CMD) $(APP_NAME) $(APP_VERSION) $(TARGET_PLATFORM)
	rm $(SDK_PATH)/platforms/$(TARGET_PLATFORM)/toolchain/$(APPS_BUILD_PATH)/tools/generate/$(APP_NAME)_*.rbl || /bin/true
	rm $(SDK_PATH)/platforms/$(TARGET_PLATFORM)/toolchain/$(APPS_BUILD_PATH)/tools/generate/$(APP_NAME)_*.bin || /bin/true

OpenBL602: submodules sdk/OpenBL602/customer_app/bl602_sharedApp/bl602_sharedApp/shared prebuild_OpenBL602
	$(MAKE) -C sdk/OpenBL602/customer_app/bl602_sharedApp USER_SW_VER=$(APP_VERSION) CONFIG_CHIP_NAME=BL602 CONFIG_LINK_ROM=1 -j
	$(MAKE) -C sdk/OpenBL602/customer_app/bl602_sharedApp USER_SW_VER=$(APP_VERSION) CONFIG_CHIP_NAME=BL602 bins
	mkdir -p output/$(APP_VERSION)
	cp sdk/OpenBL602/customer_app/bl602_sharedApp/build_out/bl602_sharedApp.bin output/$(APP_VERSION)/OpenBL602_$(APP_VERSION).bin
	cp sdk/OpenBL602/customer_app/bl602_sharedApp/build_out/ota/dts40M_pt2M_boot2release_ef4015/FW_OTA.bin output/$(APP_VERSION)/OpenBL602_$(APP_VERSION)_OTA.bin
	cp sdk/OpenBL602/customer_app/bl602_sharedApp/build_out/ota/dts40M_pt2M_boot2release_ef4015/FW_OTA.bin.xz output/$(APP_VERSION)/OpenBL602_$(APP_VERSION)_OTA.bin.xz
	cp sdk/OpenBL602/customer_app/bl602_sharedApp/build_out/ota/dts40M_pt2M_boot2release_ef4015/FW_OTA.bin.xz.ota output/$(APP_VERSION)/OpenBL602_$(APP_VERSION)_OTA.bin.xz.ota
	
sdk/OpenW800/tools/w800/csky/bin: submodules
	mkdir -p sdk/OpenW800/tools/w800/csky
	# cd sdk/OpenW800/tools/w800/csky && wget -q "https://occ-oss-prod.oss-cn-hangzhou.aliyuncs.com/resource/1356021/1619529111421/csky-elfabiv2-tools-x86_64-minilibc-20210423.tar.gz" && tar -xf *.tar.gz && rm -f *.tar.gz
	if [ ! -e sdk/OpenW800/tools/w800/csky/got_csky-elfabiv2-tools-x86_64-minilibc-20210423 ]; then cd sdk/OpenW800/tools/w800/csky && wget -q "https://occ-oss-prod.oss-cn-hangzhou.aliyuncs.com/resource/1356021/1619529111421/csky-elfabiv2-tools-x86_64-minilibc-20210423.tar.gz" && tar -xf *.tar.gz && rm -f *.tar.gz && touch got_csky-elfabiv2-tools-x86_64-minilibc-20210423 ; fi

sdk/OpenW600/tools/gcc-arm-none-eabi-4_9-2014q4/bin: submodules
	mkdir -p sdk/OpenW600/tools
	cd sdk/OpenW600/tools && tar -xf ../support/*.tar.bz2

.PHONY: OpenW800
OpenW800: sdk/OpenW800/tools/w800/csky/bin sdk/OpenW800/sharedAppContainer/sharedApp prebuild_OpenW800
	$(MAKE) -C sdk/OpenW800 EXTRA_CCFLAGS=-DPLATFORM_W800 CONFIG_W800_USE_LIB=n CONFIG_W800_TOOLCHAIN_PATH="$(shell realpath sdk/OpenW800/tools/w800/csky/bin)/"
	mkdir -p output/$(APP_VERSION)
	cp sdk/OpenW800/bin/w800/w800.fls output/$(APP_VERSION)/OpenW800_$(APP_VERSION).fls
	cp sdk/OpenW800/bin/w800/w800_ota.img output/$(APP_VERSION)/OpenW800_$(APP_VERSION)_ota.img

.PHONY: OpenW600
OpenW600: sdk/OpenW600/tools/gcc-arm-none-eabi-4_9-2014q4/bin sdk/OpenW600/sharedAppContainer/sharedApp prebuild_OpenW600
	$(MAKE) -C sdk/OpenW600 TOOL_CHAIN_PATH="$(shell realpath sdk/OpenW600/tools/gcc-arm-none-eabi-4_9-2014q4/bin)/" APP_VERSION=$(APP_VERSION)
	mkdir -p output/$(APP_VERSION)
	cp sdk/OpenW600/bin/w600/w600.fls output/$(APP_VERSION)/OpenW600_$(APP_VERSION).fls
	cp sdk/OpenW600/bin/w600/w600_gz.img output/$(APP_VERSION)/OpenW600_$(APP_VERSION)_gz.img

.PHONY: OpenLN882H
OpenLN882H: submodules sdk/OpenLN882H/project/OpenBeken/app prebuild_OpenLN882H
	CROSS_TOOLCHAIN_ROOT="/usr/" cmake sdk/OpenLN882H -B sdk/OpenLN882H/build
	CROSS_TOOLCHAIN_ROOT="/usr/" cmake --build ./sdk/OpenLN882H/build
	mkdir -p output/$(APP_VERSION)
	cp sdk/OpenLN882H/build/bin/flashimage.bin output/$(APP_VERSION)/OpenLN882H_$(APP_VERSION).bin
	cp sdk/OpenLN882H/build/bin/flashimage-ota-xz-v0.1.bin output/$(APP_VERSION)/OpenLN882H_$(APP_VERSION)_OTA.bin

.PHONY: OpenESP32
OpenESP32: prebuild_ESPIDF
	-rm platforms/ESP-IDF/sdkconfig
	IDF_TARGET="esp32" USER_SW_VER=$(APP_VERSION) cmake platforms/ESP-IDF -B platforms/ESP-IDF/build-32 
	IDF_TARGET="esp32" USER_SW_VER=$(APP_VERSION) cmake --build ./platforms/ESP-IDF/build-32 -j $(shell nproc)
	mkdir -p output/$(APP_VERSION)
	esptool.py -c esp32 merge_bin -o output/$(APP_VERSION)/OpenESP32_$(APP_VERSION).factory.bin --flash_mode dio --flash_size 4MB 0x1000 ./platforms/ESP-IDF/build-32/bootloader/bootloader.bin 0x8000 ./platforms/ESP-IDF/build-32/partition_table/partition-table.bin 0x10000 ./platforms/ESP-IDF/build-32/OpenBeken.bin
	cp ./platforms/ESP-IDF/build-32/OpenBeken.bin output/$(APP_VERSION)/OpenESP32_$(APP_VERSION).img

.PHONY: OpenESP32C3
OpenESP32C3: prebuild_ESPIDF
	-rm platforms/ESP-IDF/sdkconfig
	IDF_TARGET="esp32c3" USER_SW_VER=$(APP_VERSION) cmake platforms/ESP-IDF -B platforms/ESP-IDF/build-c3 
	IDF_TARGET="esp32c3" USER_SW_VER=$(APP_VERSION) cmake --build ./platforms/ESP-IDF/build-c3 -j $(shell nproc)
	mkdir -p output/$(APP_VERSION)
	esptool.py -c esp32c3 merge_bin -o output/$(APP_VERSION)/OpenESP32C3_$(APP_VERSION).factory.bin --flash_mode dio --flash_size 2MB 0x0 ./platforms/ESP-IDF/build-c3/bootloader/bootloader.bin 0x8000 ./platforms/ESP-IDF/build-c3/partition_table/partition-table.bin 0x10000 ./platforms/ESP-IDF/build-c3/OpenBeken.bin
	cp ./platforms/ESP-IDF/build-c3/OpenBeken.bin output/$(APP_VERSION)/OpenESP32C3_$(APP_VERSION).img

.PHONY: OpenESP32C2
OpenESP32C2: prebuild_ESPIDF
	-rm platforms/ESP-IDF/sdkconfig
	IDF_TARGET="esp32c2" USER_SW_VER=$(APP_VERSION) cmake platforms/ESP-IDF -B platforms/ESP-IDF/build-c2 
	IDF_TARGET="esp32c2" USER_SW_VER=$(APP_VERSION) cmake --build ./platforms/ESP-IDF/build-c2 -j $(shell nproc)
	mkdir -p output/$(APP_VERSION)
	esptool.py -c esp32c2 merge_bin -o output/$(APP_VERSION)/OpenESP32C2_$(APP_VERSION).factory.bin --flash_mode dio --flash_size 2MB 0x0 ./platforms/ESP-IDF/build-c2/bootloader/bootloader.bin 0x8000 ./platforms/ESP-IDF/build-c2/partition_table/partition-table.bin 0x10000 ./platforms/ESP-IDF/build-c2/OpenBeken.bin
	cp ./platforms/ESP-IDF/build-c2/OpenBeken.bin output/$(APP_VERSION)/OpenESP32C2_$(APP_VERSION).img

.PHONY: OpenESP32C6
OpenESP32C6: prebuild_ESPIDF
	-rm platforms/ESP-IDF/sdkconfig
	IDF_TARGET="esp32c6" USER_SW_VER=$(APP_VERSION) cmake platforms/ESP-IDF -B platforms/ESP-IDF/build-c6 
	IDF_TARGET="esp32c6" USER_SW_VER=$(APP_VERSION) cmake --build ./platforms/ESP-IDF/build-c6 -j $(shell nproc)
	mkdir -p output/$(APP_VERSION)
	esptool.py -c esp32c6 merge_bin -o output/$(APP_VERSION)/OpenESP32C6_$(APP_VERSION).factory.bin --flash_mode dio --flash_size 4MB 0x0 ./platforms/ESP-IDF/build-c6/bootloader/bootloader.bin 0x8000 ./platforms/ESP-IDF/build-c6/partition_table/partition-table.bin 0x10000 ./platforms/ESP-IDF/build-c6/OpenBeken.bin
	cp ./platforms/ESP-IDF/build-c6/OpenBeken.bin output/$(APP_VERSION)/OpenESP32C6_$(APP_VERSION).img

.PHONY: OpenESP32S2
OpenESP32S2: prebuild_ESPIDF
	-rm platforms/ESP-IDF/sdkconfig
	IDF_TARGET="esp32s2" USER_SW_VER=$(APP_VERSION) cmake platforms/ESP-IDF -B platforms/ESP-IDF/build-s2 
	IDF_TARGET="esp32s2" USER_SW_VER=$(APP_VERSION) cmake --build ./platforms/ESP-IDF/build-s2 -j $(shell nproc)
	mkdir -p output/$(APP_VERSION)
	esptool.py -c esp32s2 merge_bin -o output/$(APP_VERSION)/OpenESP32S2_$(APP_VERSION).factory.bin --flash_mode dio --flash_size 4MB 0x1000 ./platforms/ESP-IDF/build-s2/bootloader/bootloader.bin 0x8000 ./platforms/ESP-IDF/build-s2/partition_table/partition-table.bin 0x10000 ./platforms/ESP-IDF/build-s2/OpenBeken.bin
	cp ./platforms/ESP-IDF/build-s2/OpenBeken.bin output/$(APP_VERSION)/OpenESP32S2_$(APP_VERSION).img

.PHONY: OpenESP32S3
OpenESP32S3: prebuild_ESPIDF
	-rm platforms/ESP-IDF/sdkconfig
	IDF_TARGET="esp32s3" USER_SW_VER=$(APP_VERSION) cmake platforms/ESP-IDF -B platforms/ESP-IDF/build-s3 
	IDF_TARGET="esp32s3" USER_SW_VER=$(APP_VERSION) cmake --build ./platforms/ESP-IDF/build-s3 -j $(shell nproc)
	mkdir -p output/$(APP_VERSION)
	esptool.py -c esp32s3 merge_bin -o output/$(APP_VERSION)/OpenESP32S3_$(APP_VERSION).factory.bin --flash_mode dio --flash_size 4MB 0x0 ./platforms/ESP-IDF/build-s3/bootloader/bootloader.bin 0x8000 ./platforms/ESP-IDF/build-s3/partition_table/partition-table.bin 0x10000 ./platforms/ESP-IDF/build-s3/OpenBeken.bin
	cp ./platforms/ESP-IDF/build-s3/OpenBeken.bin output/$(APP_VERSION)/OpenESP32S3_$(APP_VERSION).img

# clean .o files and output directory
.PHONY: clean
clean: 
	$(MAKE) -C sdk/OpenBK7231T/platforms/bk7231t/bk7231t_os APP_BIN_NAME=$(APP_NAME) USER_SW_VER=$(APP_VERSION) clean
	$(MAKE) -C sdk/OpenBK7231N/platforms/bk7231n/bk7231n_os APP_BIN_NAME=$(APP_NAME) USER_SW_VER=$(APP_VERSION) clean
	$(MAKE) -C sdk/OpenXR809/src clean
	$(MAKE) -C sdk/OpenXR809/project/oxr_sharedApp/gcc clean
	$(MAKE) -C sdk/OpenW800 clean
	$(MAKE) -C sdk/OpenW600 clean
	test -d ./sdk/OpenLN882H/build && cmake --build ./sdk/OpenLN882H/build --target clean
	test -d ./platforms/ESP-IDF/build-32 && cmake --build ./platforms/ESP-IDF/build-32 --target clean
	test -d ./platforms/ESP-IDF/build-c3 && cmake --build ./platforms/ESP-IDF/build-c3 --target clean
	test -d ./platforms/ESP-IDF/build-c2 && cmake --build ./platforms/ESP-IDF/build-c2 --target clean
	test -d ./platforms/ESP-IDF/build-c6 && cmake --build ./platforms/ESP-IDF/build-c6 --target clean
	test -d ./platforms/ESP-IDF/build-s2 && cmake --build ./platforms/ESP-IDF/build-s2 --target clean
	test -d ./platforms/ESP-IDF/build-s3 && cmake --build ./platforms/ESP-IDF/build-s3 --target clean

# Add custom Makefile if required
-include custom.mk

# Example upload command - import the following snippet into Node-RED and update the IPs in the variables below
# This will stage the rbl binary on the Node-RED server and trigger a OTA request to for the BK chip to pull it automatically
## [{"id":"3cd109953d170e66","type":"tab","label":"Firmware","disabled":false,"info":"","env":[]},{"id":"0f0c991791d6922a","type":"group","z":"3cd109953d170e66","name":"Return Firmware to BK7231T","style":{"label":true},"nodes":["d274dcdb0a33093b","5cb323971ddeea52","06889aec9ed22a48","f814bcf109e0ea18","630e454cbfb63211"],"x":14,"y":239,"w":852,"h":142},{"id":"2e572f2886d560c7","type":"group","z":"3cd109953d170e66","name":"OTA command - store binary and trigger device OTA request","style":{"label":true},"nodes":["748c6dd1fc3bc2e8","fb5148de5da10c42","6fd4e23a5b96d39c","6eff9c90b19282fa","4e71b18352ff8641","99f44f74dba836a4","0fdb40d65b6911da","344c7f6a5f46c69b","f564aafdaa4a9ce2"],"x":14,"y":19,"w":1212,"h":202},{"id":"7f1a0772422bfdcc","type":"group","z":"3cd109953d170e66","name":"Clear stored firmware (if required)","style":{"label":true},"nodes":["f6f80ca0dd720e30","6e619a81ddcc630f"],"x":14,"y":399,"w":452,"h":82},{"id":"d274dcdb0a33093b","type":"http in","z":"3cd109953d170e66","g":"0f0c991791d6922a","name":"","url":"/firmware","method":"get","upload":false,"swaggerDoc":"","x":140,"y":280,"wires":[["f814bcf109e0ea18","630e454cbfb63211"]]},{"id":"5cb323971ddeea52","type":"http response","z":"3cd109953d170e66","g":"0f0c991791d6922a","name":"Return Firmware Binary","statusCode":"200","headers":{},"x":730,"y":280,"wires":[]},{"id":"748c6dd1fc3bc2e8","type":"http request","z":"3cd109953d170e66","g":"2e572f2886d560c7","name":"Send OTA request to BK7231T","method":"GET","ret":"bin","paytoqs":"ignore","url":"","tls":"","persist":false,"proxy":"","authType":"","senderr":false,"x":910,"y":60,"wires":[["99f44f74dba836a4"]]},{"id":"06889aec9ed22a48","type":"debug","z":"3cd109953d170e66","g":"0f0c991791d6922a","name":"","active":false,"tosidebar":true,"console":false,"tostatus":false,"complete":"true","targetType":"full","statusVal":"","statusType":"auto","x":670,"y":340,"wires":[]},{"id":"f814bcf109e0ea18","type":"debug","z":"3cd109953d170e66","g":"0f0c991791d6922a","name":"","active":false,"tosidebar":true,"console":false,"tostatus":false,"complete":"true","targetType":"full","statusVal":"","statusType":"auto","x":330,"y":340,"wires":[]},{"id":"fb5148de5da10c42","type":"http in","z":"3cd109953d170e66","g":"2e572f2886d560c7","name":"","url":"/ota","method":"post","upload":true,"swaggerDoc":"","x":130,"y":60,"wires":[["6fd4e23a5b96d39c","4e71b18352ff8641","344c7f6a5f46c69b"]]},{"id":"6fd4e23a5b96d39c","type":"debug","z":"3cd109953d170e66","g":"2e572f2886d560c7","name":"","active":false,"tosidebar":true,"console":false,"tostatus":false,"complete":"true","targetType":"full","statusVal":"","statusType":"auto","x":330,"y":180,"wires":[]},{"id":"6eff9c90b19282fa","type":"http response","z":"3cd109953d170e66","g":"2e572f2886d560c7","name":"","statusCode":"200","headers":{},"x":620,"y":120,"wires":[]},{"id":"4e71b18352ff8641","type":"change","z":"3cd109953d170e66","g":"2e572f2886d560c7","name":"Save data and prepare OTA command","rules":[{"t":"set","p":"ota","pt":"flow","to":"req.files[0].buffer","tot":"msg","dc":true},{"t":"set","p":"filename","pt":"flow","to":"req.files[0].originalname","tot":"msg"},{"t":"set","p":"ip","pt":"flow","to":"payload.ip","tot":"msg"},{"t":"set","p":"endpoint","pt":"flow","to":"payload.endpoint","tot":"msg"},{"t":"set","p":"url","pt":"msg","to":"\"http://\"&$flowContext(\"ip\")&\"/ota_exec?host=\"&$flowContext(\"endpoint\")&\"/firmware\"","tot":"jsonata"},{"t":"delete","p":"req","pt":"msg"},{"t":"delete","p":"res","pt":"msg"}],"action":"","property":"","from":"","to":"","reg":false,"x":430,"y":60,"wires":[["0fdb40d65b6911da"]]},{"id":"630e454cbfb63211","type":"change","z":"3cd109953d170e66","g":"0f0c991791d6922a","name":"Grab firmware from flow variable","rules":[{"t":"set","p":"payload","pt":"msg","to":"ota","tot":"flow"},{"t":"set","p":"headers","pt":"msg","to":"{\t   \"Content-Type\":\"application/octet-stream\",\t   \"Content-Disposition\":\"attachment; filename=\"&$flowContext(\"filename\"),\t   \"Connection\":\"close\"\t}","tot":"jsonata"}],"action":"","property":"","from":"","to":"","reg":false,"x":420,"y":280,"wires":[["5cb323971ddeea52","06889aec9ed22a48"]]},{"id":"99f44f74dba836a4","type":"debug","z":"3cd109953d170e66","g":"2e572f2886d560c7","name":"","active":false,"tosidebar":true,"console":false,"tostatus":false,"complete":"true","targetType":"full","statusVal":"","statusType":"auto","x":1130,"y":60,"wires":[]},{"id":"f6f80ca0dd720e30","type":"inject","z":"3cd109953d170e66","g":"7f1a0772422bfdcc","name":"","props":[{"p":"payload"},{"p":"topic","vt":"str"}],"repeat":"","crontab":"","once":false,"onceDelay":0.1,"topic":"","payload":"","payloadType":"date","x":120,"y":440,"wires":[["6e619a81ddcc630f"]]},{"id":"6e619a81ddcc630f","type":"change","z":"3cd109953d170e66","g":"7f1a0772422bfdcc","name":"","rules":[{"t":"delete","p":"ota","pt":"flow"}],"action":"","property":"","from":"","to":"","reg":false,"x":360,"y":440,"wires":[[]]},{"id":"0fdb40d65b6911da","type":"delay","z":"3cd109953d170e66","g":"2e572f2886d560c7","name":"","pauseType":"delay","timeout":"2","timeoutUnits":"seconds","rate":"1","nbRateUnits":"1","rateUnits":"second","randomFirst":"1","randomLast":"5","randomUnits":"seconds","drop":false,"allowrate":false,"outputs":1,"x":680,"y":60,"wires":[["748c6dd1fc3bc2e8","f564aafdaa4a9ce2"]]},{"id":"344c7f6a5f46c69b","type":"change","z":"3cd109953d170e66","g":"2e572f2886d560c7","name":"Prepare response message","rules":[{"t":"set","p":"payload","pt":"msg","to":"\"Uploading \"&req.files[0].originalname&\" to \"&payload.ip&\"\\n\"","tot":"jsonata","dc":true}],"action":"","property":"","from":"","to":"","reg":false,"x":400,"y":120,"wires":[["6eff9c90b19282fa"]]},{"id":"f564aafdaa4a9ce2","type":"debug","z":"3cd109953d170e66","g":"2e572f2886d560c7","name":"","active":false,"tosidebar":true,"console":false,"tostatus":false,"complete":"true","targetType":"full","statusVal":"","statusType":"auto","x":830,"y":120,"wires":[]}]
NODE_RED_ENDPOINT ?= http://192.168.x.x:1880/endpoint
BK7231T_IP ?= 192.168.x.y
.PHONY: upload
upload:
	curl -F "ota=@output/$(APP_VERSION)/OpenBK7231T_$(APP_VERSION).rbl" -F "ip=$(BK7231T_IP)" -F "endpoint=$(NODE_RED_ENDPOINT)" $(NODE_RED_ENDPOINT)/ota

endif
