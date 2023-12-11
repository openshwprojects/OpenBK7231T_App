#ifndef _HTTP_BASIC_AUTH_H
#define _HTTP_BASIC_AUTH_H

#include "new_http.h"

#define HTTP_BASIC_AUTH_FAIL 0
#define HTTP_BASIC_AUTH_OK 1

int http_basic_auth_eval(http_request_t *request);
int http_basic_auth_run(http_request_t *request);

#endif