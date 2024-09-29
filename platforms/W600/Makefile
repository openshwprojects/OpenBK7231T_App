
#############################################################
# Required variables for each makefile
# Discard this section from all parent makefiles
# Expected variables (with automatic defaults):
#   CSRCS (all "C" files in the dir)
#   SUBDIRS (all subdirs with a Makefile)
#   GEN_LIBS - list of libs to be generated ()
#   GEN_IMAGES - list of images to be generated ()
#   COMPONENTS_xxx - a list of libs/objs in the form
#     subdir/lib to be extracted and rolled up into
#     a generated lib/image xxx.a ()
#

TOP_DIR = ..
sinclude $(TOP_DIR)/tools/tool_chain.def

ifndef PDIR
GEN_LIBS = libuser$(LIB_EXT)
endif

_SHARED_APP = $(TOP_DIR)/sharedAppContainer/sharedApp/src

CSRCS += $(_SHARED_APP)/bitmessage/bitmessage_read.c
CSRCS += $(_SHARED_APP)/bitmessage/bitmessage_write.c
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
CSRCS += $(_SHARED_APP)/devicegroups/deviceGroups_read.c
CSRCS += $(_SHARED_APP)/devicegroups/deviceGroups_write.c
CSRCS += $(_SHARED_APP)/devicegroups/deviceGroups_util.c
CSRCS += $(_SHARED_APP)/driver/drv_bl0937.c
CSRCS += $(_SHARED_APP)/driver/drv_bl_shared.c
CSRCS += $(_SHARED_APP)/driver/drv_pwrCal.c
CSRCS += $(_SHARED_APP)/driver/drv_dht.c
CSRCS += $(_SHARED_APP)/driver/drv_dht_internal.c
CSRCS += $(_SHARED_APP)/driver/drv_httpButtons.c
CSRCS += $(_SHARED_APP)/driver/drv_main.c
CSRCS += $(_SHARED_APP)/driver/drv_ntp.c
CSRCS += $(_SHARED_APP)/driver/drv_tasmotaDeviceGroups.c
CSRCS += $(_SHARED_APP)/driver/drv_test_drivers.c
CSRCS += $(_SHARED_APP)/driver/drv_bridge_driver.c
CSRCS += $(_SHARED_APP)/driver/drv_charts.c
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
CSRCS += $(_SHARED_APP)/ota/ota.c
CSRCS += $(_SHARED_APP)/rgb2hsv.c
CSRCS += $(_SHARED_APP)/tiny_crc8.c
CSRCS += $(_SHARED_APP)/user_main.c
CSRCS += main.c

CSRCS += $(TOP_DIR)/src/network/lwip2.0.3/apps/mqtt/mqtt.c


#############################################################
# Configuration i.e. compile options etc.
# Target specific stuff (defines etc.) goes in here!
# Generally values applying to a tree are captured in the
#   makefile at its root level - these are then overridden
#   for a subtree within the makefile rooted therein
#
#DEFINES +=

#############################################################
# Recursion Magic - Don't touch this!!
#
# Each subtree potentially has an include directory
#   corresponding to the common APIs applicable to modules
#   rooted at that subtree. Accordingly, the INCLUDE PATH
#   of a module can only contain the include directories up
#   its parent path, and not its siblings
#
# Required for each makefile to inherit from the parent
#

sinclude $(TOP_DIR)/tools/rules.mk

INCLUDES := $(INCLUDES) -I $(PDIR)include

PDIR := ../$(PDIR)
sinclude $(PDIR)Makefile


