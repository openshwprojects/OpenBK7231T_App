#include "../new_common.h"
#include "../logging/logging.h"
#include "../httpserver/new_http.h"
#include "str_pub.h"

static int http_rest_get(http_request_t *request);
static int http_rest_post(http_request_t *request);
static int http_rest_app(http_request_t *request);

void init_rest(){
    HTTP_RegisterCallback( "/api", HTTP_GET, http_rest_get);
    HTTP_RegisterCallback( "/api", HTTP_POST, http_rest_post);
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
const char * apppage4 = "/startup.js\"></script>"
"    </head>"
"<body>"
"</body>"
"</html>";


static int http_rest_app(http_request_t *request){
    http_setup(request, httpMimeTypeHTML);
    //char *webhost = "http://raspberrypi:1880";//CFG_GetWebRoot();
    char *webhost = "https://btsimonh.github.io/testwebpages/";
    char *ourip = "192.168.1.176"; //CFG_GetOurIP();
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
    return 0;
}

static int http_rest_post(http_request_t *request){
    return 0;
}
