#include "new_http.h"


extern const char *g_header;

int http_fn_about(http_request_t *request);
int http_fn_cfg_mqtt(http_request_t *request);
int http_fn_cfg_mqtt_set(http_request_t *request);
int http_fn_cfg_webapp(http_request_t *request);
int http_fn_config_dump_table(http_request_t *request);
int http_fn_cfg_webapp_set(http_request_t *request);
int http_fn_cfg_wifi_set(http_request_t *request);
int http_fn_cfg_loglevel_set(http_request_t *request);
int http_fn_cfg_wifi(http_request_t *request);
int http_fn_cfg_mac(http_request_t *request);
int http_fn_flash_read_tool(http_request_t *request);
int http_fn_cfg_quick(http_request_t *request);
int http_fn_cfg_ha(http_request_t *request);
int http_fn_cfg(http_request_t *request);
int http_fn_cfg_pins(http_request_t *request);
int http_fn_index(http_request_t *request);
int http_fn_ota_exec(http_request_t *request);
int http_fn_ota(http_request_t *request);
int http_fn_empty_url(http_request_t *request);
int http_fn_other(http_request_t *request);
