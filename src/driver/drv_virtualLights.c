// src/driver/drv_virtualLights.c
#include "drv_virtualLights.h"
#include "../cmnds/cmd_public.h"
#include "../logging/logging.h"
#include "drv_local.h"
#include "../new_pins.h"

extern const char *CFG_GetDeviceName(void);
extern const char *CFG_GetMQTTClientId(void);
extern const char *HAL_GetMyIPString(void);
#include "../mqtt/new_mqtt.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

extern const char *g_build_str;

static int g_virtualLightsEnabled = 0;
static int g_whiteEnabled = 1;
static int g_rgbEnabled = 0;

static int g_whiteDimmer = 100;
static int g_whiteTemperature = 4201;
static int g_rgbDimmer = 100;

static byte g_rgbR = 255;
static byte g_rgbG = 0;
static byte g_rgbB = 0;

static int vl_parse_onoff(const char *args, int defval) {
	int v;
	if(args == 0 || *args == 0) {
		return defval;
	}
	v = atoi(args);
	return v ? 1 : 0;
}

static int vl_parse_percent(const char *args, int defval) {
	int v;
	if(args == 0 || *args == 0) {
		return defval;
	}
	v = atoi(args);
	if(v < 0) v = 0;
	if(v > 100) v = 100;
	return v;
}

static int hex_val(char c) {
	if(c >= '0' && c <= '9') return c - '0';
	if(c >= 'a' && c <= 'f') return 10 + (c - 'a');
	if(c >= 'A' && c <= 'F') return 10 + (c - 'A');
	return -1;
}

static int parse_hex_byte(const char *s) {
	int a = hex_val(s[0]);
	int b = hex_val(s[1]);
	if(a < 0 || b < 0) {
		return -1;
	}
	return (a << 4) | b;
}

static int vl_parse_rgb_hex(const char *args, byte *r, byte *g, byte *b) {
	const char *p;
	int rv, gv, bv;

	if(args == 0) {
		return 0;
	}

	while(*args == ' ') {
		args++;
	}

	p = args;
	if(*p == '#') {
		p++;
	}

	if(strlen(p) < 6) {
		return 0;
	}

	rv = parse_hex_byte(p + 0);
	gv = parse_hex_byte(p + 2);
	bv = parse_hex_byte(p + 4);

	if(rv < 0 || gv < 0 || bv < 0) {
		return 0;
	}

	*r = (byte)rv;
	*g = (byte)gv;
	*b = (byte)bv;
	return 1;
}


static int vl_parse_rgb_any(const char *args, byte *r, byte *g, byte *b) {
        int rv, gv, bv;

        if(vl_parse_rgb_hex(args, r, g, b)) {
                return 1;
        }

        if(args == 0) {
                return 0;
        }

        while(*args == ' ') {
                args++;
        }

        if(sscanf(args, "%d,%d,%d", &rv, &gv, &bv) == 3) {
                if(rv < 0) rv = 0;
                if(rv > 255) rv = 255;
                if(gv < 0) gv = 0;
                if(gv > 255) gv = 255;
                if(bv < 0) bv = 0;
                if(bv > 255) bv = 255;

                *r = (byte)rv;
                *g = (byte)gv;
                *b = (byte)bv;
                return 1;
        }

        return 0;
}

static byte vl_scale_byte(byte v, int dimmer) {
	int out = ((int)v * dimmer) / 100;
	if(out < 0) out = 0;
	if(out > 255) out = 255;
	return (byte)out;
}


static void VL_MQTT_PublishWhiteState(void) {
        char buf[64];

    if (!MQTT_IsReady()) {
        return;
    }

        int mired;

        snprintf(buf, sizeof(buf), "%d", g_whiteEnabled ? 1 : 0);
        MQTT_PublishMain_StringString("vl_white", buf, 0);

        snprintf(buf, sizeof(buf), "%d", g_whiteDimmer);
        MQTT_PublishMain_StringString("vl_white_bri", buf, 0);

        mired = 1000000 / g_whiteTemperature;
        snprintf(buf, sizeof(buf), "%d", mired);
        MQTT_PublishMain_StringString("vl_white_mired", buf, 0);
}

static void VL_MQTT_PublishRGBState(void) {
        char buf[64];

    if (!MQTT_IsReady()) {
        return;
    }


        snprintf(buf, sizeof(buf), "%d", g_rgbEnabled ? 1 : 0);
        MQTT_PublishMain_StringString("vl_rgb", buf, 0);

        snprintf(buf, sizeof(buf), "%d", g_rgbDimmer);
        MQTT_PublishMain_StringString("vl_rgb_bri", buf, 0);

        snprintf(buf, sizeof(buf), "%d,%d,%d",
                (int)g_rgbR, (int)g_rgbG, (int)g_rgbB);
        MQTT_PublishMain_StringString("vl_rgb_color", buf, 0);
}


static void VirtualLights_ApplyRGBStrip() {
	byte r, g, b;

	if(!g_virtualLightsEnabled) {
		return;
	}

	if(!Strip_IsActive()) {
		addLogAdv(LOG_INFO, LOG_FEATURE_CMD, "VirtualLights: RGB strip driver not active");
		return;
	}

	if(pixel_count == 0) {
		addLogAdv(LOG_INFO, LOG_FEATURE_CMD, "VirtualLights: pixel_count is 0, run SM16703P_Init <count> <order>");
		return;
	}

	if(!g_rgbEnabled || g_rgbDimmer <= 0) {
		Strip_setAllPixels(0, 0, 0, 0, 0);
		Strip_Apply();
		addLogAdv(LOG_INFO, LOG_FEATURE_CMD, "VirtualLights: RGB strip OFF");
		return;
	}

	r = vl_scale_byte(g_rgbR, g_rgbDimmer);
	g = vl_scale_byte(g_rgbG, g_rgbDimmer);
	b = vl_scale_byte(g_rgbB, g_rgbDimmer);

	Strip_setAllPixels(r, g, b, 0, 0);
	Strip_Apply();

	addLogAdv(LOG_INFO, LOG_FEATURE_CMD,
		"VirtualLights: RGB strip applied count=%u color=#%02X%02X%02X dimmer=%d scaled=#%02X%02X%02X",
		(unsigned int)pixel_count,
		(unsigned int)g_rgbR,
		(unsigned int)g_rgbG,
		(unsigned int)g_rgbB,
		g_rgbDimmer,
		(unsigned int)r,
		(unsigned int)g,
		(unsigned int)b);
}

static void VL_ApplyWhiteOnly() {
        int temp;
        int cold;
        int warm;

        if(!g_virtualLightsEnabled) {
                return;
        }

        temp = g_whiteTemperature;
        if(temp < 2000) temp = 2000;
        if(temp > 6500) temp = 6500;

        if(!g_whiteEnabled || g_whiteDimmer <= 0) {
                CHANNEL_Set(0, 0, 0);
                CHANNEL_Set(1, 0, 0);
                addLogAdv(LOG_INFO, LOG_FEATURE_CMD, "VirtualLights: White OFF");
                return;
        }

        cold = ((temp - 2000) * g_whiteDimmer) / 4500;
        if(cold < 0) cold = 0;
        if(cold > 100) cold = 100;

        warm = g_whiteDimmer - cold;
        if(warm < 0) warm = 0;
        if(warm > 100) warm = 100;

        CHANNEL_Set(0, cold, 0);
        CHANNEL_Set(1, warm, 0);

        addLogAdv(LOG_INFO, LOG_FEATURE_CMD,
                "VirtualLights: White applied temp=%d dimmer=%d cw=%d ww=%d",
                temp, g_whiteDimmer, cold, warm);
}

static void VL_ApplyAll() {
	if(!g_virtualLightsEnabled) {
		return;
	}
	VL_ApplyWhiteOnly();
	VirtualLights_ApplyRGBStrip();
}

static commandResult_t CMD_VirtualLights_Enable(const void *context, const char *cmd, const char *args, int flags) {
	(void)context; (void)cmd; (void)flags;

	g_virtualLightsEnabled = vl_parse_onoff(args, 1);

	if(g_virtualLightsEnabled) {
		VL_ApplyAll();
	}
	else {
                CHANNEL_Set(0, 0, 0);
                CHANNEL_Set(1, 0, 0);

                if(Strip_IsActive()) {
                        Strip_setAllPixels(0, 0, 0, 0, 0);
                        Strip_Apply();
                }
        }

	addLogAdv(LOG_INFO, LOG_FEATURE_CMD, "VirtualLights_Enable=%d", g_virtualLightsEnabled);
	VL_MQTT_PublishWhiteState();
    VL_MQTT_PublishRGBState();
    return CMD_RES_OK;
}

static commandResult_t CMD_VirtualLights_White_Enable(const void *context, const char *cmd, const char *args, int flags) {
        (void)context; (void)cmd; (void)flags;

        g_whiteEnabled = vl_parse_onoff(args, 1);
        VL_ApplyWhiteOnly();

        addLogAdv(LOG_INFO, LOG_FEATURE_CMD, "VirtualLights_White_Enable=%d", g_whiteEnabled);
        VL_MQTT_PublishWhiteState();
        return CMD_RES_OK;
}

static commandResult_t CMD_VirtualLights_White_Dimmer(const void *context, const char *cmd, const char *args, int flags) {
        (void)context; (void)cmd; (void)flags;

        g_whiteDimmer = vl_parse_percent(args, g_whiteDimmer);
        VL_ApplyWhiteOnly();

        addLogAdv(LOG_INFO, LOG_FEATURE_CMD, "VirtualLights_White_Dimmer=%d", g_whiteDimmer);
        VL_MQTT_PublishWhiteState();
        return CMD_RES_OK;
}

static commandResult_t CMD_VirtualLights_RGB_Enable(const void *context, const char *cmd, const char *args, int flags) {
	(void)context; (void)cmd; (void)flags;

	g_rgbEnabled = vl_parse_onoff(args, 1);
	VirtualLights_ApplyRGBStrip();

	addLogAdv(LOG_INFO, LOG_FEATURE_CMD, "VirtualLights_RGB_Enable=%d", g_rgbEnabled);
	VL_MQTT_PublishRGBState();
    return CMD_RES_OK;
}

static commandResult_t CMD_VirtualLights_RGB_Dimmer(const void *context, const char *cmd, const char *args, int flags) {
	(void)context; (void)cmd; (void)flags;

	g_rgbDimmer = vl_parse_percent(args, g_rgbDimmer);
	VirtualLights_ApplyRGBStrip();

	addLogAdv(LOG_INFO, LOG_FEATURE_CMD, "VirtualLights_RGB_Dimmer=%d", g_rgbDimmer);
	VL_MQTT_PublishRGBState();
    return CMD_RES_OK;
}

static commandResult_t CMD_VirtualLights_RGB_Color(const void *context, const char *cmd, const char *args, int flags) {
	byte r, g, b;
	(void)context; (void)cmd; (void)flags;

	if(!vl_parse_rgb_any(args, &r, &g, &b)) {
		addLogAdv(LOG_INFO, LOG_FEATURE_CMD, "VirtualLights_RGB_Color: bad arg, expected #RRGGBB");
		return CMD_RES_BAD_ARGUMENT;
	}

	g_rgbR = r;
	g_rgbG = g;
	g_rgbB = b;

	VirtualLights_ApplyRGBStrip();

	addLogAdv(LOG_INFO, LOG_FEATURE_CMD,
		"VirtualLights_RGB_Color=#%02X%02X%02X",
		(unsigned int)g_rgbR,
		(unsigned int)g_rgbG,
		(unsigned int)g_rgbB);

	
    VL_MQTT_PublishRGBState();
    return CMD_RES_OK;
}
static commandResult_t CMD_VirtualLights_White_Temperature(const void *context, const char *cmd, const char *args, int flags) {
        int v;
        (void)context; (void)cmd; (void)flags;

        if(args == 0 || *args == 0) {
                return CMD_RES_BAD_ARGUMENT;
        }

        v = atoi(args);
        if(v < 2000) v = 2000;
        if(v > 6500) v = 6500;

        g_whiteTemperature = v;
        VL_ApplyWhiteOnly();

        addLogAdv(LOG_INFO, LOG_FEATURE_CMD, "VirtualLights_White_Temperature=%d", g_whiteTemperature);
        VL_MQTT_PublishWhiteState();
        return CMD_RES_OK;
}

static commandResult_t CMD_VirtualLights_White_Mired(const void *context, const char *cmd, const char *args, int flags) {
        int mired;
        int kelvin;
        (void)context; (void)cmd; (void)flags;

        if(args == 0 || *args == 0) {
                return CMD_RES_BAD_ARGUMENT;
        }

        mired = atoi(args);
        if(mired < 154) mired = 154;
        if(mired > 500) mired = 500;

        kelvin = 1000000 / mired;
        if(kelvin < 2000) kelvin = 2000;
        if(kelvin > 6500) kelvin = 6500;

        g_whiteTemperature = kelvin;
        VL_ApplyWhiteOnly();

        addLogAdv(LOG_INFO, LOG_FEATURE_CMD,
                "VirtualLights_White_Mired=%d -> Kelvin=%d",
                mired, kelvin);
        VL_MQTT_PublishWhiteState();
        return CMD_RES_OK;
}

static commandResult_t CMD_VirtualLights_RGB_Refresh(const void *context, const char *cmd, const char *args, int flags) {
	(void)context; (void)cmd; (void)args; (void)flags;
	VirtualLights_ApplyRGBStrip();
	return CMD_RES_OK;
}



// -------------------------------------------------
// MQTT Refresh Command
// -------------------------------------------------
static commandResult_t CMD_VirtualLights_MQTT_Refresh(const void *context, const char *cmd, const char *args, int flags) {
    (void)context; (void)cmd; (void)args; (void)flags;

    VL_MQTT_PublishWhiteState();
    VL_MQTT_PublishRGBState();

    addLogAdv(LOG_INFO, LOG_FEATURE_CMD, "VirtualLights MQTT state refreshed");

    return CMD_RES_OK;
}

void VirtualLights_Init() {
	CMD_RegisterCommand("VirtualLights_Enable", CMD_VirtualLights_Enable, NULL);
	CMD_RegisterCommand("VirtualLights_White_Enable", CMD_VirtualLights_White_Enable, NULL);
	CMD_RegisterCommand("VirtualLights_White_Dimmer", CMD_VirtualLights_White_Dimmer, NULL);
        CMD_RegisterCommand("VirtualLights_White_Temperature", CMD_VirtualLights_White_Temperature, NULL);
    CMD_RegisterCommand("VirtualLights_White_Mired", CMD_VirtualLights_White_Mired, NULL);
	CMD_RegisterCommand("VirtualLights_RGB_Enable", CMD_VirtualLights_RGB_Enable, NULL);
	CMD_RegisterCommand("VirtualLights_RGB_Dimmer", CMD_VirtualLights_RGB_Dimmer, NULL);
	CMD_RegisterCommand("VirtualLights_RGB_Color", CMD_VirtualLights_RGB_Color, NULL);
    CMD_RegisterCommand("VirtualLights_MQTT_Refresh", CMD_VirtualLights_MQTT_Refresh, NULL);
	CMD_RegisterCommand("VirtualLights_RGB_Refresh", CMD_VirtualLights_RGB_Refresh, NULL);

	addLogAdv(LOG_INFO, LOG_FEATURE_CMD, "VirtualLights driver initialized");
}
int VirtualLights_IsEnabled(void) {
        return g_virtualLightsEnabled;
}


void VirtualLights_Shutdown() {
	addLogAdv(LOG_INFO, LOG_FEATURE_CMD, "VirtualLights driver stopped");
}

void VirtualLights_OnChannelChanged(int ch, int value) {
	(void)ch;
	(void)value;
}

void VirtualLights_OnHassDiscovery(const char *topic) {
        const char *devName;
        const char *clientId;
        const char *ip;
        char config_topic[192];
        char payload[1024];

        (void)topic;

        devName = CFG_GetDeviceName();
        clientId = CFG_GetMQTTClientId();
        ip = HAL_GetMyIPString();

        snprintf(config_topic, sizeof(config_topic), "homeassistant/light/%s_vl_white/config", devName);
        snprintf(payload, sizeof(payload),
                "{"
                "\"name\":\"White\","
                "\"uniq_id\":\"%s_vl_white\","
                "\"~\":\"%s\","
                "\"stat_t\":\"~/vl_white/get\","
                "\"cmd_t\":\"cmnd/%s/VirtualLights_White_Enable\","
                "\"pl_on\":\"1\","
                "\"pl_off\":\"0\","
                "\"bri_stat_t\":\"~/vl_white_bri/get\","
                "\"bri_cmd_t\":\"cmnd/%s/VirtualLights_White_Dimmer\","
                "\"bri_scl\":\"100\","
                "\"clr_temp_stat_t\":\"~/vl_white_mired/get\","
                "\"clr_temp_cmd_t\":\"cmnd/%s/VirtualLights_White_Mired\","
                "\"min_mirs\":\"154\","
                "\"max_mirs\":\"500\","
                "\"dev\":{\"ids\":[\"%s\"],\"name\":\"%s\",\"sw\":\"%s\",\"mf\":\"OpenBeken\",\"mdl\":\"BK7238\",\"cu\":\"http://%s/index\"}"
                "}",
                devName, clientId,
                clientId,
                clientId,
                clientId,
                devName, devName, g_build_str, ip);
        MQTT_PublishMain_StringString(config_topic, payload,
                OBK_PUBLISH_FLAG_RAW_TOPIC_NAME | OBK_PUBLISH_FLAG_RETAIN);

        snprintf(config_topic, sizeof(config_topic), "homeassistant/light/%s_vl_rgb/config", devName);
        snprintf(payload, sizeof(payload),
                "{"
                "\"name\":\"RGB Strip\","
                "\"uniq_id\":\"%s_vl_rgb\","
                "\"~\":\"%s\","
                "\"stat_t\":\"~/vl_rgb/get\","
                "\"cmd_t\":\"cmnd/%s/VirtualLights_RGB_Enable\","
                "\"pl_on\":\"1\","
                "\"pl_off\":\"0\","
                "\"bri_stat_t\":\"~/vl_rgb_bri/get\","
                "\"bri_cmd_t\":\"cmnd/%s/VirtualLights_RGB_Dimmer\","
                "\"bri_scl\":\"100\","
                "\"rgb_stat_t\":\"~/vl_rgb_color/get\","
                "\"rgb_cmd_t\":\"cmnd/%s/VirtualLights_RGB_Color\","
                "\"dev\":{\"ids\":[\"%s\"],\"name\":\"%s\",\"sw\":\"%s\",\"mf\":\"OpenBeken\",\"mdl\":\"BK7238\",\"cu\":\"http://%s/index\"}"
                "}",
                devName, clientId,
                clientId,
                clientId,
                clientId,
                devName, devName, g_build_str, ip);
        MQTT_PublishMain_StringString(config_topic, payload,
                OBK_PUBLISH_FLAG_RAW_TOPIC_NAME | OBK_PUBLISH_FLAG_RETAIN);

        VL_MQTT_PublishWhiteState();
        VL_MQTT_PublishRGBState();
}

void VirtualLights_AppendInformationToHTTPIndexPage(http_request_t *request, int bPreState) {
        char rgbHex[8];

        if(bPreState) {
                return;
        }

        sprintf(rgbHex, "%02X%02X%02X",
                (unsigned int)g_rgbR,
                (unsigned int)g_rgbG,
                (unsigned int)g_rgbB);

        poststr(request, "<h4>VirtualLights</h4>");

        poststr(request, "<p>Split mode: ");
        hprintf255(request, "%d ", g_virtualLightsEnabled);
        poststr(request, "<a href='#' onclick=\"fetch('/cm?cmnd=VirtualLights_Enable%201').then(function(){showState();});return false;\">ON</a> ");
        poststr(request, "<a href='#' onclick=\"fetch('/cm?cmnd=VirtualLights_Enable%200').then(function(){showState();});return false;\">OFF</a></p>");

        if(!g_virtualLightsEnabled) {
                return;
        }

        poststr(request, "<hr>");

        poststr(request, "<p><b>White</b>: ");
        hprintf255(request, "%d ", g_whiteEnabled);
        poststr(request, "<a href='#' onclick=\"fetch('/cm?cmnd=VirtualLights_White_Enable%201').then(function(){showState();});return false;\">ON</a> ");
        poststr(request, "<a href='#' onclick=\"fetch('/cm?cmnd=VirtualLights_White_Enable%200').then(function(){showState();});return false;\">OFF</a></p>");

        hprintf255(request, "<p>White Brightness: %d</p>", g_whiteDimmer);
        hprintf255(request,
                "<input type='range' min='0' max='100' value='%d' onchange=\"fetch('/cm?cmnd='+encodeURIComponent('VirtualLights_White_Dimmer '+this.value)).then(function(){showState();});\">",
                g_whiteDimmer);

        hprintf255(request, "<p>White Temperature (%d K)</p>", g_whiteTemperature);
        hprintf255(request,
                "<input type='range' min='2000' max='6500' step='50' value='%d' onchange=\"fetch('/cm?cmnd='+encodeURIComponent('VirtualLights_White_Temperature '+this.value)).then(function(){showState();});\">",
                g_whiteTemperature);

        poststr(request, "<hr>");

        poststr(request, "<p><b>RGB Strip</b>: ");
        hprintf255(request, "%d ", g_rgbEnabled);
        poststr(request, "<a href='#' onclick=\"fetch('/cm?cmnd=VirtualLights_RGB_Enable%201').then(function(){showState();});return false;\">ON</a> ");
        poststr(request, "<a href='#' onclick=\"fetch('/cm?cmnd=VirtualLights_RGB_Enable%200').then(function(){showState();});return false;\">OFF</a></p>");

        hprintf255(request, "<p>RGB Brightness: %d</p>", g_rgbDimmer);
        hprintf255(request,
                "<input type='range' min='0' max='100' value='%d' onchange=\"fetch('/cm?cmnd='+encodeURIComponent('VirtualLights_RGB_Dimmer '+this.value)).then(function(){showState();});\">",
                g_rgbDimmer);

        poststr(request, "<p>RGB Color</p>");
        poststr(request, "<form onsubmit=\"fetch('/cm?cmnd='+encodeURIComponent(document.getElementById('vl_rgb_color_cmd').value)).then(function(){showState();});return false;\">");
        hprintf255(request,
                "<input type='color' id='vl_rgb_color_picker' value='#%s' onchange=\"document.getElementById('vl_rgb_color_cmd').value='VirtualLights_RGB_Color '+this.value;\">",
                rgbHex);
        hprintf255(request,
                "<input type='hidden' id='vl_rgb_color_cmd' value='VirtualLights_RGB_Color #%s'>",
                rgbHex);
        poststr(request, "<input type='submit' value='Apply RGB Color'>");
        poststr(request, "</form>");

        poststr(request, "<hr>");
        poststr(request, "<p><b>All</b>: ");
        poststr(request, "<a href='#' onclick=\"fetch('/cm?cmnd=VirtualLights_White_Enable%201').then(function(){return fetch('/cm?cmnd=VirtualLights_RGB_Enable%201');}).then(function(){showState();});return false;\">ON</a> ");
        poststr(request, "<a href='#' onclick=\"fetch('/cm?cmnd=VirtualLights_White_Enable%200').then(function(){return fetch('/cm?cmnd=VirtualLights_RGB_Enable%200');}).then(function(){showState();});return false;\">OFF</a></p>");

        if(Strip_IsActive()) {
                hprintf255(request, "<p>RGB driver active, pixel_count=%u</p>", (unsigned int)pixel_count);
        } else {
                poststr(request, "<p>RGB driver inactive. Start it with: <code>startDriver SM16703P</code>, then <code>SM16703P_Init &lt;count&gt; &lt;order&gt;</code>, then <code>SM16703P_Start</code>.</p>");
        }
}
