#include "new_http.h"


// TODO: move it out 
void doHomeAssistantDiscovery(const char *topic, http_request_t *request);

int http_fn_about(http_request_t* request);
int http_fn_cfg_mqtt(http_request_t* request);
int http_fn_cfg_ip(http_request_t* request);
int http_fn_cfg_mqtt_set(http_request_t* request);
int http_fn_cfg_webapp(http_request_t* request);
int http_fn_cfg_webapp_set(http_request_t* request);
int http_fn_cfg_wifi_set(http_request_t* request);
int http_fn_cfg_name(http_request_t* request);
int http_fn_cfg_loglevel_set(http_request_t* request);
int http_fn_cfg_wifi(http_request_t* request);
int http_fn_cfg_mac(http_request_t* request);
int http_fn_flash_read_tool(http_request_t* request);
int http_fn_cmd_tool(http_request_t* request);
int http_fn_ha_cfg(http_request_t* request);
int http_fn_ha_discovery(http_request_t* request);
int http_fn_cfg(http_request_t* request);
int http_fn_cfg_pins(http_request_t* request);
int http_fn_cfg_ping(http_request_t* request);
int http_fn_index(http_request_t* request);
int http_fn_testmsg(http_request_t* request);
int http_fn_ota_exec(http_request_t* request);
int http_fn_ota(http_request_t* request);
int http_fn_empty_url(http_request_t* request);
int http_fn_other(http_request_t* request);
int http_fn_cm(http_request_t* request);
int http_fn_startup_command(http_request_t* request);
int http_fn_cfg_generic(http_request_t* request);
int http_fn_cfg_startup(http_request_t* request);
int http_fn_cfg_dgr(http_request_t* request);
int http_fn_pmntp(http_request_t* request);
