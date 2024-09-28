TOP_DIR = ..

$(info    TOP_DIR is $(TOP_DIR)) 
sinclude $(TOP_DIR)/tools/w800/conf.mk

ifndef PDIR
GEN_LIBS = libuser$(LIB_EXT)
endif

#DEFINES +=

_SHARED_APP = $(TOP_DIR)/sharedAppContainer/sharedApp/src

$(info    _SHARED_APP is $(_SHARED_APP)) 


CSRCS += $(_SHARED_APP)/cmnds/cmd_channels.c
CSRCS += $(_SHARED_APP)/cmnds/cmd_eventHandlers.c
CSRCS += $(_SHARED_APP)/cmnds/cmd_main.c
CSRCS += $(_SHARED_APP)/cmnds/cmd_newLEDDriver.c
CSRCS += $(_SHARED_APP)/cmnds/cmd_newLEDDriver_colors.c
CSRCS += $(_SHARED_APP)/cmnds/cmd_repeatingEvents.c
CSRCS += $(_SHARED_APP)/cmnds/cmd_tasmota.c
CSRCS += $(_SHARED_APP)/cmnds/cmd_test.c
CSRCS += $(_SHARED_APP)/cmnds/cmd_tcp.c
CSRCS += $(_SHARED_APP)/cmnds/cmd_tokenizer.c
CSRCS += $(_SHARED_APP)/cmnds/cmd_if.c
CSRCS += $(_SHARED_APP)/cJSON/cJSON.c
CSRCS += $(_SHARED_APP)/base64/base64.c
CSRCS += $(_SHARED_APP)/hal/w800/hal_adc_w800.c
CSRCS += $(_SHARED_APP)/hal/w800/hal_flashConfig_w800.c
CSRCS += $(_SHARED_APP)/hal/w800/hal_flashVars_w800.c
CSRCS += $(_SHARED_APP)/hal/w800/hal_generic_w800.c
CSRCS += $(_SHARED_APP)/hal/w800/hal_main_w800.c
CSRCS += $(_SHARED_APP)/hal/w800/hal_pins_w800.c
CSRCS += $(_SHARED_APP)/hal/w800/hal_wifi_w800.c
CSRCS += $(_SHARED_APP)/httpserver/hass.c
CSRCS += $(_SHARED_APP)/httpserver/http_fns.c
CSRCS += $(_SHARED_APP)/httpserver/http_tcp_server.c
CSRCS += $(_SHARED_APP)/httpserver/http_basic_auth.c
CSRCS += $(_SHARED_APP)/httpserver/new_http.c
CSRCS += $(_SHARED_APP)/httpserver/rest_interface.c
CSRCS += $(_SHARED_APP)/httpserver/json_interface.c
CSRCS += $(_SHARED_APP)/logging/logging.c
CSRCS += $(_SHARED_APP)/jsmn/jsmn.c
CSRCS += $(_SHARED_APP)/mqtt/new_mqtt.c
CSRCS += $(_SHARED_APP)/mqtt/new_mqtt_deduper.c
CSRCS += $(_SHARED_APP)/new_cfg.c
CSRCS += $(_SHARED_APP)/new_common.c
CSRCS += $(_SHARED_APP)/new_ping.c
CSRCS += $(_SHARED_APP)/new_pins.c
CSRCS += $(_SHARED_APP)/rgb2hsv.c
CSRCS += $(_SHARED_APP)/tiny_crc8.c
CSRCS += $(_SHARED_APP)/driver/drv_main.c
CSRCS += $(_SHARED_APP)/driver/drv_ds1820_simple.c
CSRCS += $(_SHARED_APP)/driver/drv_charts.c
CSRCS += $(_SHARED_APP)/user_main.c
CSRCS += main.c

CSRCS += $(TOP_DIR)/src/network/lwip2.0.3/apps/mqtt/mqtt.c

sinclude $(TOP_DIR)/tools/w800/rules.mk

INCLUDES := $(INCLUDES) -I $(PDIR)include

PDIR := ../$(PDIR)
sinclude $(PDIR)Makefile
