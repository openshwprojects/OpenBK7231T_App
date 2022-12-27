#ifdef WINDOWS

#include "../../../new_common.h"
#include "apps/mqtt.h"
#include "ip_addr.h"

/** Connect to server */
err_t mqtt_client_connect(mqtt_client_t *client, const ip_addr_t *ipaddr, u16_t port, mqtt_connection_cb_t cb, void *arg,
                   const struct mqtt_connect_client_info_t *client_info) {
	return 0;
}

/** Disconnect from server */
void mqtt_disconnect(mqtt_client_t *client) {
	return 0;
}

/** Create new client */
mqtt_client_t *mqtt_client_new(void) {
	mqtt_client_t *r;
	r = (mqtt_client_t*)malloc(sizeof(mqtt_client_t));
	memset(r, 0, sizeof(mqtt_client_t));
	return r;
}

/** Check connection status */
u8_t mqtt_client_is_connected(mqtt_client_t *client) {
	return 1;
}

/** Set callback to call for incoming publish */
void mqtt_set_inpub_callback(mqtt_client_t *client, mqtt_incoming_publish_cb_t cb,
                             mqtt_incoming_data_cb_t data_cb, void *arg) {
	return ;
}

/** Common function for subscribe and unsubscribe */
err_t mqtt_sub_unsub(mqtt_client_t *client, const char *topic, u8_t qos, mqtt_request_cb_t cb, void *arg, u8_t sub) {
	return 0;
}

void SIM_OnMQTTPublish(const char *topic, const char *value, int len, int qos, bool bRetain);

/** Publish data to topic */
err_t mqtt_publish(mqtt_client_t *client, const char *topic, const void *payload, u16_t payload_length, u8_t qos, u8_t retain,
				   mqtt_request_cb_t cb, void *arg) {
	// on Windows simulator, forward MQTT publish for unit testing
	SIM_OnMQTTPublish(topic, payload, payload_length, qos, retain);
	return 0;
}
#endif
