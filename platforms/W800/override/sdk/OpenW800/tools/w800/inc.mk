INCLUDES := $(INC) $(INCLUDES) -I $(PDIR)include

INCLUDES += -I $(TOP_DIR)/include
INCLUDES += -I $(TOP_DIR)/include/app
INCLUDES += -I $(TOP_DIR)/include/arch/xt804
INCLUDES += -I $(TOP_DIR)/include/arch/xt804/csi_core
INCLUDES += -I $(TOP_DIR)/include/arch/xt804/csi_dsp
INCLUDES += -I $(TOP_DIR)/include/driver
INCLUDES += -I $(TOP_DIR)/include/net
INCLUDES += -I $(TOP_DIR)/include/os
INCLUDES += -I $(TOP_DIR)/include/platform
INCLUDES += -I $(TOP_DIR)/include/wifi
INCLUDES += -I $(TOP_DIR)/include/bt

INCLUDES += -I $(TOP_DIR)/platform/common/params
INCLUDES += -I $(TOP_DIR)/platform/inc
INCLUDES += -I $(TOP_DIR)/platform/sys

INCLUDES += -I $(TOP_DIR)/src/app/wm_atcmd
INCLUDES += -I $(TOP_DIR)/src/app/dhcpserver
INCLUDES += -I $(TOP_DIR)/src/app/dnsserver
INCLUDES += -I $(TOP_DIR)/src/app/web
INCLUDES += -I $(TOP_DIR)/src/app/cloud
INCLUDES += -I $(TOP_DIR)/src/app/cJSON
INCLUDES += -I $(TOP_DIR)/src/app/rmms
INCLUDES += -I $(TOP_DIR)/src/app/ntp
INCLUDES += -I $(TOP_DIR)/src/app/httpclient
INCLUDES += -I $(TOP_DIR)/src/app/oneshotconfig
INCLUDES += -I $(TOP_DIR)/src/app/iperf
INCLUDES += -I $(TOP_DIR)/src/app/mqtt
INCLUDES += -I $(TOP_DIR)/src/app/ping
INCLUDES += -I $(TOP_DIR)/src/app/polarssl/include
INCLUDES += -I $(TOP_DIR)/src/app/mDNS/mDNSPosix
INCLUDES += -I $(TOP_DIR)/src/app/mDNS/mDNSCore
INCLUDES += -I $(TOP_DIR)/src/app/ota
INCLUDES += -I $(TOP_DIR)/src/app/libwebsockets-2.1-stable
INCLUDES += -I $(TOP_DIR)/src/app/fatfs
INCLUDES += -I $(TOP_DIR)/src/app/mbedtls/include
INCLUDES += -I $(TOP_DIR)/src/app/mbedtls/ports
INCLUDES += -I $(TOP_DIR)/src/network/api2.0.3
INCLUDES += -I $(TOP_DIR)/src/network/lwip2.0.3/include
INCLUDES += -I $(TOP_DIR)/src/os/rtos/include

INCLUDES += -I $(TOP_DIR)/src/app/factorycmd
INCLUDES += -I $(TOP_DIR)/src/app/bleapp
INCLUDES += -I $(TOP_DIR)/demo
#nimble host
INCLUDES += -I $(TOP_DIR)/src/bt/blehost/ext/tinycrypt/include
INCLUDES += -I $(TOP_DIR)/src/bt/blehost/nimble/host/include
INCLUDES += -I $(TOP_DIR)/src/bt/blehost/nimble/host/mesh/include
INCLUDES += -I $(TOP_DIR)/src/bt/blehost/nimble/host/porting/w800/include
INCLUDES += -I $(TOP_DIR)/src/bt/blehost/nimble/host/services/gap/include
INCLUDES += -I $(TOP_DIR)/src/bt/blehost/nimble/host/services/gatt/include
INCLUDES += -I $(TOP_DIR)/src/bt/blehost/nimble/host/services/bas/include
INCLUDES += -I $(TOP_DIR)/src/bt/blehost/nimble/host/services/dis/include
INCLUDES += -I $(TOP_DIR)/src/bt/blehost/nimble/host/store/config/include
INCLUDES += -I $(TOP_DIR)/src/bt/blehost/nimble/host/store/ram/include
INCLUDES += -I $(TOP_DIR)/src/bt/blehost/nimble/host/util/include
INCLUDES += -I $(TOP_DIR)/src/bt/blehost/nimble/include
INCLUDES += -I $(TOP_DIR)/src/bt/blehost/nimble/transport/uart/include
INCLUDES += -I $(TOP_DIR)/src/bt/blehost/porting/w800/include
#br_edr bt host
INCLUDES += -I $(TOP_DIR)/src/bt/host
INCLUDES += -I $(TOP_DIR)/src/bt/host/main
INCLUDES += -I $(TOP_DIR)/src/bt/host/include
INCLUDES += -I $(TOP_DIR)/src/bt/host/stack
INCLUDES += -I $(TOP_DIR)/src/bt/host/stack/include
INCLUDES += -I $(TOP_DIR)/src/bt/host/gki/wm
INCLUDES += -I $(TOP_DIR)/src/bt/host/gki/common
INCLUDES += -I $(TOP_DIR)/src/bt/host/vnd/include
INCLUDES += -I $(TOP_DIR)/src/bt/host/bta/include
INCLUDES += -I $(TOP_DIR)/src/bt/host/osi/include
INCLUDES += -I $(TOP_DIR)/src/bt/host/hci/include
INCLUDES += -I $(TOP_DIR)/src/bt/host/btif/include
INCLUDES += -I $(TOP_DIR)/src/bt/host/embdrv/sbc/encoder/include
INCLUDES += -I $(TOP_DIR)/src/bt/host/embdrv/sbc/decoder/include
INCLUDES += -I $(TOP_DIR)/src/bt/host/btcore/include

