MODULE_IFLAGS +=  -DPLATFORM_RTL8720D -DPLATFORM_REALTEK -Wno-strict-prototypes -Wno-unused-parameter

MODULE_IFLAGS += -I$(EFDIR)/easyflash/inc

CSRC  += $(PLDIR)/main.c

#CSRC  += hal/realtek/rtl8720d/hal_adc_rtl8710b.c
#CSRC  += hal/realtek/rtl8720d/hal_main_rtl8710b.c
CSRC  += hal/realtek/rtl8720d/hal_uart_rtl8720d.c
CSRC  += hal/realtek/rtl8720d/hal_pins_rtl8720d.c
CSRC  += hal/realtek/hal_flashConfig_realtek.c
CSRC  += hal/realtek/hal_flashVars_realtek.c
CSRC  += hal/realtek/hal_generic_realtek.c
CSRC  += hal/realtek/hal_pins_realtek.c
CSRC  += hal/realtek/hal_wifi_realtek.c
CSRC  += hal/realtek/hal_ota_realtek.c

OBK_SRCS = 
include $(EFDIR)/../platforms/obk_main.mk
CSRC += $(OBKM_SRC)
CPPSRC += $(OBKM_SRC_CXX)
MODULE_IFLAGS += $(OBK_CFLAGS)

CSRC += libraries/easyflash/ports/ef_port.c
CSRC += libraries/easyflash/easyflash.c
#CSRC += libraries/easyflash/ef_cmd.c
CSRC += libraries/easyflash/ef_env.c
CSRC += libraries/easyflash/ef_env_legacy.c
CSRC += libraries/easyflash/ef_env_legacy_wl.c
CSRC += libraries/easyflash/ef_iap.c
CSRC += libraries/easyflash/ef_log.c
CSRC += libraries/easyflash/ef_utils.c

MODULE_IFLAGS += -I$(EFDIR)/../include
BERRY_MODULEPATH = berry/modules
BERRY_SRCPATH = libraries/berry/src
MODULE_IFLAGS += -I$(EFDIR)/berry/src

include $(EFDIR)/berry.mk

CSRC += $(BERRY_SRC_C)
