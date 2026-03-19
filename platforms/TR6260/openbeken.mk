VPATH  += $(OBK_PATH)
VPATH  += $(OBK_PATH)/../platforms/TR6260
VPATH  += $(OBK_PATH)/../sdk/OpenTR6260/platform/lwip/lwip-2.1.0/src/apps/mdns
DEFINE += -DPLATFORM_TR6260
DEFINE += -DENABLE_DRIVER_MDNS=1 -DLWIP_MDNS_RESPONDER=1 -DLWIP_NUM_NETIF_CLIENT_DATA=1
DEFINE += -DTCP_MSL=1000

CSRCS  += main.c
CSRCS  += mdns.c
CSRCS  += hal/tr6260/hal_adc_tr6260.c
CSRCS  += hal/tr6260/hal_flashConfig_tr6260.c
CSRCS  += hal/tr6260/hal_flashVars_tr6260.c
CSRCS  += hal/tr6260/hal_generic_tr6260.c
CSRCS  += hal/tr6260/hal_hwtimer_tr6260.c
CSRCS  += hal/tr6260/hal_main_tr6260.c
CSRCS  += hal/tr6260/hal_pins_tr6260.c
CSRCS  += hal/tr6260/hal_wifi_tr6260.c
CSRCS  += hal/tr6260/hal_ota_tr6260.c

OBK_SRCS = 
include $(OBK_PATH)/../platforms/obk_main.mk
CSRCS += $(OBKM_SRC)
DEFINE += $(OBK_CFLAGS)

INCLUDE += -I$(OBK_PATH)/../include
VPATH += $(OBK_PATH)/../

BERRY_MODULEPATH = berry/modules
BERRY_SRCPATH = libraries/berry/src
INCLUDE += -I$(OBK_PATH)/../$(BERRY_SRCPATH)

include $(OBK_PATH)/../libraries/berry.mk

CSRCS += $(BERRY_SRC_C)
