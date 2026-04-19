#include "drv_virtualLights.h"
#include "drv_public.h"
#include "../cmnds/cmd_public.h"
#include "../httpserver/hass.h"
#include "../httpserver/new_http.h"
#include "../logging/logging.h"
#include "../new_cfg.h"
#include "../new_pins.h"
#include <stdlib.h>
#include <string.h>

static int g_virtualLightsEnabled = 0;
static int g_whiteEnabled = 1;
static int g_rgbEnabled = 1;

#define VL_WHITE_TOGGLE_CH  201
#define VL_WHITE_DIMMER_CH  202
#define VL_RGB_TOGGLE_CH    203

static commandResult_t CMD_VirtualLights_Enable(const void *context, const char *cmd, const char *args, int flags) {
    (void)context; (void)cmd; (void)flags;
    g_virtualLightsEnabled = (args && *args) ? (atoi(args) ? 1 : 0) : 1;
    addLogAdv(LOG_INFO, LOG_FEATURE_CMD, "VirtualLights_Enable=%d", g_virtualLightsEnabled);
    return CMD_RES_OK;
}

static commandResult_t CMD_VirtualLights_White_Enable(const void *context, const char *cmd, const char *args, int flags) {
    (void)context; (void)cmd; (void)flags;
    g_whiteEnabled = (args && *args) ? (atoi(args) ? 1 : 0) : 1;

    if (!g_virtualLightsEnabled) {
        addLogAdv(LOG_INFO, LOG_FEATURE_CMD, "VirtualLights disabled, ignoring white enable");
        return CMD_RES_OK;
    }

    CHANNEL_Set(VL_WHITE_TOGGLE_CH, g_whiteEnabled ? 1 : 0, 0);

    if (g_whiteEnabled) {
        LED_SetEnableAll(1);
        LED_SetMode(Light_Temperature, 1);
    } else {
        LED_SetEnableAll(0);
    }

    addLogAdv(LOG_INFO, LOG_FEATURE_CMD, "VirtualLights_White_Enable=%d", g_whiteEnabled);
    return CMD_RES_OK;
}

static commandResult_t CMD_VirtualLights_RGB_Enable(const void *context, const char *cmd, const char *args, int flags) {
    (void)context; (void)cmd; (void)flags;
    g_rgbEnabled = (args && *args) ? (atoi(args) ? 1 : 0) : 1;

    if (!g_virtualLightsEnabled) {
        addLogAdv(LOG_INFO, LOG_FEATURE_CMD, "VirtualLights disabled, ignoring rgb enable");
        return CMD_RES_OK;
    }

    CHANNEL_Set(VL_RGB_TOGGLE_CH, g_rgbEnabled ? 1 : 0, 0);

    if (g_rgbEnabled) {
        LED_SetEnableAll(1);
        LED_SetMode(Light_RGB, 1);
    } else {
        LED_SetEnableAll(0);
    }

    addLogAdv(LOG_INFO, LOG_FEATURE_CMD, "VirtualLights_RGB_Enable=%d", g_rgbEnabled);
    return CMD_RES_OK;
}

static commandResult_t CMD_VirtualLights_White_Dimmer(const void *context, const char *cmd, const char *args, int flags) {
    int v;
    (void)context; (void)cmd; (void)flags;

    if (!(args && *args)) return CMD_RES_BAD_ARGUMENT;
    v = atoi(args);
    if (v < 0) v = 0;
    if (v > 100) v = 100;

    CHANNEL_Set(VL_WHITE_DIMMER_CH, v, 0);

    if (g_virtualLightsEnabled && g_whiteEnabled) {
        LED_SetDimmer(v, 1);
        LED_SetMode(Light_Temperature, 1);
    }

    addLogAdv(LOG_INFO, LOG_FEATURE_CMD, "VirtualLights_White_Dimmer=%d", v);
    return CMD_RES_OK;
}

void VirtualLights_Init() {
    CMD_RegisterCommand("VirtualLights_Enable", CMD_VirtualLights_Enable, NULL);
    CMD_RegisterCommand("VirtualLights_White_Enable", CMD_VirtualLights_White_Enable, NULL);
    CMD_RegisterCommand("VirtualLights_RGB_Enable", CMD_VirtualLights_RGB_Enable, NULL);
    CMD_RegisterCommand("VirtualLights_White_Dimmer", CMD_VirtualLights_White_Dimmer, NULL);

    CHANNEL_Set(VL_WHITE_TOGGLE_CH, g_whiteEnabled, 0);
    CHANNEL_Set(VL_WHITE_DIMMER_CH, LED_GetDimmer(), 0);
    CHANNEL_Set(VL_RGB_TOGGLE_CH, g_rgbEnabled, 0);

    addLogAdv(LOG_INFO, LOG_FEATURE_CMD, "VirtualLights driver initialized");
}

void VirtualLights_Shutdown() {
    addLogAdv(LOG_INFO, LOG_FEATURE_CMD, "VirtualLights driver stopped");
}

void VirtualLights_OnChannelChanged(int ch, int value) {
    (void)value;

    if (!g_virtualLightsEnabled) return;

    if (ch == 3 || ch == 4 || ch == SPECIAL_CHANNEL_TEMPERATURE || ch == SPECIAL_CHANNEL_BRIGHTNESS) {
        CHANNEL_Set(VL_WHITE_DIMMER_CH, LED_GetDimmer(), 0);
    }
}

void VirtualLights_OnHassDiscovery(const char *topic) {
#if ENABLE_HA_DISCOVERY
    HassDeviceInfo *dev_info;

    if (!g_virtualLightsEnabled) return;

    dev_info = hass_init_light_singleColor_onChannels(VL_WHITE_TOGGLE_CH, VL_WHITE_DIMMER_CH, 100);
    if (dev_info) {
        cJSON_ReplaceItemInObject(dev_info->root, "name", cJSON_CreateString("White"));
        MQTT_QueuePublish(topic, dev_info->channel, hass_build_discovery_json(dev_info), OBK_PUBLISH_FLAG_RETAIN);
        hass_free_device_info(dev_info);
    }

    dev_info = hass_init_device_info(LIGHT_RGB, VL_RGB_TOGGLE_CH, "1", "0", 0, "RGB");
    if (dev_info) {
        cJSON_AddStringToObject(dev_info->root, "stat_t", "~/203/get");
        cJSON_AddStringToObject(dev_info->root, "cmd_t", "~/203/set");
        cJSON_AddStringToObject(dev_info->root, "rgb_stat_t", "~/led_basecolor_rgb/get");

        sprintf(g_hassBuffer, "cmnd/%s/led_basecolor_rgb", CFG_GetMQTTClientId());
        cJSON_AddStringToObject(dev_info->root, "rgb_cmd_t", g_hassBuffer);

        cJSON_AddStringToObject(dev_info->root, "rgb_cmd_tpl", "{{'#%02x%02x%02x0000'|format(red,green,blue)}}");
        cJSON_AddStringToObject(dev_info->root, "rgb_val_tpl", "{{ value[0:2]|int(base=16) }},{{ value[2:4]|int(base=16) }},{{ value[4:6]|int(base=16) }}");

        MQTT_QueuePublish(topic, dev_info->channel, hass_build_discovery_json(dev_info), OBK_PUBLISH_FLAG_RETAIN);
        hass_free_device_info(dev_info);
    }
#else
    (void)topic;
#endif
}

void VirtualLights_AppendInformationToHTTPIndexPage(http_request_t *request, int bPreState) {
    int whiteDimmer;

    if (bPreState) return;

    whiteDimmer = CHANNEL_Get(VL_WHITE_DIMMER_CH);

    hprintf255(request, "<h4>VirtualLights</h4>");

    hprintf255(request,
        "<p>Split mode: %d "
        "<a href='/cm?cmnd=VirtualLights_Enable%%201'>ON</a> "
        "<a href='/cm?cmnd=VirtualLights_Enable%%200'>OFF</a></p>",
        g_virtualLightsEnabled);

    if (!g_virtualLightsEnabled) return;

    hprintf255(request,
        "<p>White: %d "
        "<a href='/cm?cmnd=VirtualLights_White_Enable%%201'>ON</a> "
        "<a href='/cm?cmnd=VirtualLights_White_Enable%%200'>OFF</a></p>",
        g_whiteEnabled);

    hprintf255(request,
        "<form action='/cm' method='get'>"
        "<input type='hidden' name='cmnd' id='vl_white_dim_cmd' value='VirtualLights_White_Dimmer %d'>"
        "<input type='range' min='0' max='100' value='%d' "
        "onchange=\"document.getElementById('vl_white_dim_cmd').value='VirtualLights_White_Dimmer '+this.value; this.form.submit();\">"
        "</form>",
        whiteDimmer, whiteDimmer);

    hprintf255(request,
        "<p>RGB: %d "
        "<a href='/cm?cmnd=VirtualLights_RGB_Enable%%201'>ON</a> "
        "<a href='/cm?cmnd=VirtualLights_RGB_Enable%%200'>OFF</a></p>",
        g_rgbEnabled);

    hprintf255(request,
        "<p><a href='/index'>Use main RGB color picker above for RGB color while split mode is enabled.</a></p>");
}
