#include "../obk_config.h"

#include "../new_common.h"
#include "../logging/logging.h"
#include "../httpserver/new_http.h"
//#include "str_pub.h"
#include "../new_pins.h"
#include "../jsmn/jsmn_h.h"
#include "../printnetinfo/printnetinfo.h"

static int http_rest_get(http_request_t *request);
static int http_rest_post(http_request_t *request);
static int http_rest_app(http_request_t *request);

static int http_rest_post_pins(http_request_t *request);
static int http_rest_get_pins(http_request_t *request);

static int http_rest_post_logconfig(http_request_t *request);
static int http_rest_get_logconfig(http_request_t *request);

void init_rest(){
    HTTP_RegisterCallback( "/api/", HTTP_GET, http_rest_get);
    HTTP_RegisterCallback( "/api/", HTTP_POST, http_rest_post);
    HTTP_RegisterCallback( "/app", HTTP_GET, http_rest_app);
}

const char *apppage1 = 
"<!DOCTYPE html>"
"<html>"
"    <head>"
"        <script>"
"            var root = '";
const char * apppage2 = "';"
"            var device = 'http://";
const char * apppage3 = "';"
"        </script>"
"        <script src=\"";
const char * apppage4 = "startup.js\"></script>"
"    </head>"
"<body>"
"</body>"
"</html>";


static int http_rest_app(http_request_t *request){
    //char *webhost = "http://raspberrypi:1880";//CFG_GetWebRoot();
    char *webhost = "https://openbekeniot.github.io/webapp/";
    char *ourip = getMyIp(); //CFG_GetOurIP();
    http_setup(request, httpMimeTypeHTML);
    if (webhost && ourip){
        poststr(request, apppage1);
        poststr(request, webhost);
        poststr(request, apppage2);
        poststr(request, ourip);
        poststr(request, apppage3);
        poststr(request, webhost);
        poststr(request, apppage4);
    } else {
        poststr(request,htmlHeader);
        poststr(request,htmlReturnToMenu);
        poststr(request,"no APP available<br/>");
    }
    poststr(request,NULL);
    return 0;
}


#ifdef BK_LITTLEFS
static int http_rest_get_lfs(http_request_t *request){
    init_lfs();
    http_setup(request, httpMimeTypeHTML);
    poststr(request, "GET of ");
    poststr(request, request->url);
    poststr(request, htmlEnd);
    poststr(request,NULL);
  return 0;
}
#endif



static int http_rest_get(http_request_t *request){
    ADDLOG_DEBUG(LOG_FEATURE_API, "GET of %s", request->url);
    if (!strcmp(request->url, "api/pins")){
        return http_rest_get_pins(request);
    }
    if (!strcmp(request->url, "api/logconfig")){
        return http_rest_get_logconfig(request);
    }
    #ifdef BK_LITTLEFS
    if (!strcmp(request->url, "api/initlfs")){
        return http_rest_get_lfs(request);
    }
    #endif

    http_setup(request, httpMimeTypeHTML);
    poststr(request, "GET of ");
    poststr(request, request->url);
    poststr(request, htmlEnd);
    poststr(request,NULL);
    return 0;
}



static int http_rest_get_pins(http_request_t *request){
    int i;
    /*typedef struct pinsState_s {
    	byte roles[32];
	    byte channels[32];
    } pinsState_t;

    extern pinsState_t g_pins;
    */
    http_setup(request, httpMimeTypeJson);
    poststr(request, "{\"rolenames\":[");
    for (i = 0; i < IOR_Total_Options; i++){
        if (i){
            hprintf128(request, ",\"%s\"", htmlPinRoleNames[i]);
        } else {
            hprintf128(request, "\"%s\"", htmlPinRoleNames[i]);
        }
    }
    poststr(request, "],\"roles\":[");

    for (i = 0; i < 32; i++){
        if (i){
            hprintf128(request, ",%d", g_pins.roles[i]);
        } else {
            hprintf128(request, "%d", g_pins.roles[i]);
        }
    }
    poststr(request, "],\"channels\":[");
    for (i = 0; i < 32; i++){
        if (i){
            hprintf128(request, ",%d", g_pins.channels[i]);
        } else {
            hprintf128(request, "%d", g_pins.channels[i]);
        }
    }
    poststr(request, "]}");
    poststr(request, NULL);
    return 0;
}



////////////////////////////
// log config
static int http_rest_get_logconfig(http_request_t *request){
    int i;
    http_setup(request, httpMimeTypeJson);
    hprintf128(request, "{\"level\":%d,", loglevel);
    hprintf128(request, "\"features\":%d,", logfeatures);
    poststr(request, "\"levelnames\":[");
    for (i = 0; i < LOG_MAX; i++){
        if (i){
            hprintf128(request, ",\"%s\"", loglevelnames[i]);
        } else {
            hprintf128(request, "\"%s\"", loglevelnames[i]);
        }
    }
    poststr(request, "],\"featurenames\":[");
    for (i = 0; i < LOG_FEATURE_MAX; i++){
        if (i){
            hprintf128(request, ",\"%s\"", logfeaturenames[i]);
        } else {
            hprintf128(request, "\"%s\"", logfeaturenames[i]);
        }
    }
    poststr(request, "]}");
    poststr(request, NULL);
    return 0;
}

static int http_rest_post_logconfig(http_request_t *request){
    int i;
    int r;
    char tmp[64];

    //https://github.com/zserge/jsmn/blob/master/example/simple.c
    //jsmn_parser p;
    jsmn_parser *p = malloc(sizeof(jsmn_parser));
    //jsmntok_t t[128]; /* We expect no more than 128 tokens */
#define TOKEN_COUNT 128
    jsmntok_t *t = malloc(sizeof(jsmntok_t)*TOKEN_COUNT);
    char *json_str = request->bodystart;
    int json_len = strlen(json_str);

	http_setup(request, httpMimeTypeText);
	memset(p, 0, sizeof(jsmn_parser));
	memset(t, 0, sizeof(jsmntok_t)*128);

    jsmn_init(p);
    r = jsmn_parse(p, json_str, json_len, t, TOKEN_COUNT);
    if (r < 0) {
        ADDLOG_ERROR(LOG_FEATURE_API, "Failed to parse JSON: %d", r);
        poststr(request, NULL);
        free(p);
        free(t);
        return 0;
    }

    /* Assume the top-level element is an object */
    if (r < 1 || t[0].type != JSMN_OBJECT) {
        ADDLOG_ERROR(LOG_FEATURE_API, "Object expected", r);
        poststr(request, NULL);
        free(p);
        free(t);
        return 0;
    }

    //sprintf(tmp,"parsed JSON: %s\n", json_str);
    //poststr(request, tmp);
    //poststr(request, NULL);

        /* Loop over all keys of the root object */
    for (i = 1; i < r; i++) {
        if (jsoneq(json_str, &t[i], "level") == 0) {
            if (t[i + 1].type != JSMN_PRIMITIVE) {
                continue; /* We expect groups to be an array of strings */
            }
            loglevel = atoi(json_str + t[i + 1].start);
            i += t[i + 1].size + 1;
        } else if (jsoneq(json_str, &t[i], "features") == 0) {
            if (t[i + 1].type != JSMN_PRIMITIVE) {
                continue; /* We expect groups to be an array of strings */
            }
            logfeatures = atoi(json_str + t[i + 1].start);;
            i += t[i + 1].size + 1;
        } else {
            ADDLOG_ERROR(LOG_FEATURE_API, "Unexpected key: %.*s", t[i].end - t[i].start,
                json_str + t[i].start);
            sprintf(tmp,"Unexpected key: %.*s\n", t[i].end - t[i].start,
                json_str + t[i].start);
            poststr(request, tmp);
        }
    }

    poststr(request, NULL);
    free(p);
    free(t);
    return 0;
}

/////////////////////////////////////////////////


static int http_rest_post(http_request_t *request){
    char tmp[20];
    ADDLOG_DEBUG(LOG_FEATURE_API, "POST to %s", request->url);
    if (!strcmp(request->url, "api/pins")){
        return http_rest_post_pins(request);
    }
    if (!strcmp(request->url, "api/logconfig")){
        return http_rest_post_logconfig(request);
    }

    http_setup(request, httpMimeTypeHTML);
    poststr(request, "POST to ");
    poststr(request, request->url);
    poststr(request, "<br/>Content Length:");
    sprintf(tmp, "%d", request->contentLength);
    poststr(request, tmp);
    poststr(request, "<br/>Content:[");
    poststr(request, request->bodystart);
    poststr(request, "]<br/>");
    poststr(request, htmlEnd);
    poststr(request,NULL);
    return 0;
}

// currently crashes the MCU - maybe stack overflow?
static int http_rest_post_pins(http_request_t *request){
    int i;
    int r;
    char tmp[64];
    int iChanged = 0;

    //https://github.com/zserge/jsmn/blob/master/example/simple.c
    //jsmn_parser p;
    jsmn_parser *p = malloc(sizeof(jsmn_parser));
    //jsmntok_t t[128]; /* We expect no more than 128 tokens */
#define TOKEN_COUNT 128
    jsmntok_t *t = malloc(sizeof(jsmntok_t)*TOKEN_COUNT);
    char *json_str = request->bodystart;
    int json_len = strlen(json_str);

	http_setup(request, httpMimeTypeText);
	memset(p, 0, sizeof(jsmn_parser));
	memset(t, 0, sizeof(jsmntok_t)*128);

    jsmn_init(p);
    r = jsmn_parse(p, json_str, json_len, t, TOKEN_COUNT);
    if (r < 0) {
        ADDLOG_ERROR(LOG_FEATURE_API, "Failed to parse JSON: %d", r);
        sprintf(tmp,"Failed to parse JSON: %d\n", r);
        poststr(request, tmp);
        poststr(request, NULL);
        free(p);
        free(t);
        return 0;
    }

    /* Assume the top-level element is an object */
    if (r < 1 || t[0].type != JSMN_OBJECT) {
        ADDLOG_ERROR(LOG_FEATURE_API, "Object expected", r);
        sprintf(tmp,"Object expected\n");
        poststr(request, tmp);
        poststr(request, NULL);
        free(p);
        free(t);
        return 0;
    }

    /* Loop over all keys of the root object */
    for (i = 1; i < r; i++) {
        if (jsoneq(json_str, &t[i], "roles") == 0) {
            int j;
            if (t[i + 1].type != JSMN_ARRAY) {
                continue; /* We expect groups to be an array of strings */
            }
            for (j = 0; j < t[i + 1].size; j++) {
                int roleval, pr;
                jsmntok_t *g = &t[i + j + 2];
                roleval = atoi(json_str + g->start);
				pr = PIN_GetPinRoleForPinIndex(i);
				if(pr != roleval) {
					PIN_SetPinRoleForPinIndex(i,roleval);
					iChanged++;
				}
            }
            i += t[i + 1].size + 1;
        } else if (jsoneq(json_str, &t[i], "channels") == 0) {
            int j;
            if (t[i + 1].type != JSMN_ARRAY) {
                continue; /* We expect groups to be an array of strings */
            }
            for (j = 0; j < t[i + 1].size; j++) {
                int chanval, pr;
                jsmntok_t *g = &t[i + j + 2];
                chanval = atoi(json_str + g->start);
				pr = PIN_GetPinChannelForPinIndex(j);
				if(pr != chanval) {
					PIN_SetPinChannelForPinIndex(j,chanval);
					iChanged++;
				}
            }
            i += t[i + 1].size + 1;
        } else {
            ADDLOG_ERROR(LOG_FEATURE_API, "Unexpected key: %.*s", t[i].end - t[i].start,
                json_str + t[i].start);
        }
    }
    if (iChanged){
	    PIN_SaveToFlash();
    }

    poststr(request, NULL);
    free(p);
    free(t);
    return 0;
}
