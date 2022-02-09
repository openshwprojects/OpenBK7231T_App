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

static int http_rest_get(http_request_t *request){
    if (!strcmp(request->url, "api/pins")){
        return http_rest_get_pins(request);
    }

    http_setup(request, httpMimeTypeHTML);
    poststr(request, "GET of ");
    poststr(request, request->url);
    poststr(request, htmlEnd);
    poststr(request,NULL);
    return 0;
}

static int http_rest_get_pins(http_request_t *request){
    int i;
    char tmp[20];
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
            sprintf(tmp, ",\"%s\"", htmlPinRoleNames[i]);
        } else {
            sprintf(tmp, "\"%s\"", htmlPinRoleNames[i]);
        }
        poststr(request, tmp);
    }
    poststr(request, "],\"roles\":[");

    for (i = 0; i < 32; i++){
        if (i){
            sprintf(tmp, ",%d", g_pins.roles[i]);
        } else {
            sprintf(tmp, "%d", g_pins.roles[i]);
        }
        poststr(request, tmp);
    }
    poststr(request, "],\"channels\":[");
    for (i = 0; i < 32; i++){
        if (i){
            sprintf(tmp, ",%d", g_pins.channels[i]);
        } else {
            sprintf(tmp, "%d", g_pins.channels[i]);
        }
        poststr(request, tmp);
    }
    poststr(request, "]}");
    poststr(request, NULL);
    return 0;
}



static int http_rest_post(http_request_t *request){
    char tmp[20];
    if (!strcmp(request->url, "api/pins")){
        return http_rest_post_pins(request);
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
        sprintf(tmp,"Failed to parse JSON: %d\n", r);
        poststr(request, tmp);
        poststr(request, NULL);
        free(p);
        free(t);
        return 0;
    }

    /* Assume the top-level element is an object */
    if (r < 1 || t[0].type != JSMN_OBJECT) {
        sprintf(tmp,"Object expected\n");
        poststr(request, tmp);
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
        if (jsoneq(json_str, &t[i], "roles") == 0) {
            int j;
            sprintf(tmp,"- Roles:\n");
            poststr(request, tmp);
            if (t[i + 1].type != JSMN_ARRAY) {
                continue; /* We expect groups to be an array of strings */
            }
            for (j = 0; j < t[i + 1].size; j++) {
                jsmntok_t *g = &t[i + j + 2];
                sprintf(tmp,"  * %.*s\n", g->end - g->start, json_str + g->start);
                poststr(request, tmp);
            }
            i += t[i + 1].size + 1;
        } else if (jsoneq(json_str, &t[i], "channels") == 0) {
            int j;
            sprintf(tmp,"- Channels:\n");
            poststr(request, tmp);
            if (t[i + 1].type != JSMN_ARRAY) {
                continue; /* We expect groups to be an array of strings */
            }
            for (j = 0; j < t[i + 1].size; j++) {
                jsmntok_t *g = &t[i + j + 2];
                sprintf(tmp,"  * %.*s\n", g->end - g->start, json_str + g->start);
                poststr(request, tmp);
            }
            i += t[i + 1].size + 1;
        } else {
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
