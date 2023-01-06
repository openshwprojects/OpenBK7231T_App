
#include "../err.h"
#include <stdint.h>

#define MQTT_OUTPUT_RINGBUF_SIZE 8192
#define MQTT_VAR_HEADER_BUFFER_LEN 8192
#define MQTT_REQ_MAX_IN_FLIGHT 32

typedef struct mqtt_client_s mqtt_client_t;

/*---------------------------------------------------------------------------------------------- */
/* Connection with server */

/**
 * @ingroup mqtt
 * Client information and connection parameters */
struct mqtt_connect_client_info_t {
  /** Client identifier, must be set by caller */
  const char *client_id;
  /** User name and password, set to NULL if not used */
  const char* client_user;
  const char* client_pass;
  /** keep alive time in seconds, 0 to disable keep alive functionality*/
  uint16_t keep_alive;
  /** will topic, set to NULL if will is not to be used,
      will_msg, will_qos and will retain are then ignored */
  const char* will_topic;
  const char* will_msg;
  uint8_t will_qos;
  uint8_t will_retain;
};

/**
 * @ingroup mqtt
 * Connection status codes */
typedef enum
{
  MQTT_CONNECT_ACCEPTED                 = 0,
  MQTT_CONNECT_REFUSED_PROTOCOL_VERSION = 1,
  MQTT_CONNECT_REFUSED_IDENTIFIER       = 2,
  MQTT_CONNECT_REFUSED_SERVER           = 3,
  MQTT_CONNECT_REFUSED_USERNAME_PASS    = 4,
  MQTT_CONNECT_REFUSED_NOT_AUTHORIZED_  = 5,
  MQTT_CONNECT_DISCONNECTED             = 256,
  MQTT_CONNECT_TIMEOUT                  = 257
} mqtt_connection_status_t;

/**
 * @ingroup mqtt
 * Function prototype for mqtt connection status callback. Called when
 * client has connected to the server after initiating a mqtt connection attempt by
 * calling mqtt_connect() or when connection is closed by server or an error
 *
 * @param client MQTT client itself
 * @param arg Additional argument to pass to the callback function
 * @param status Connect result code or disconnection notification @see mqtt_connection_status_t
 *
 */
typedef void (*mqtt_connection_cb_t)(mqtt_client_t *client, void *arg, mqtt_connection_status_t status);




/**
 * @ingroup mqtt
 * Data callback flags */
enum {
  /** Flag set when last fragment of data arrives in data callback */
  MQTT_DATA_FLAG_LAST = 1
};

/** 
 * @ingroup mqtt
 * Function prototype for MQTT incoming publish data callback function. Called when data
 * arrives to a subscribed topic @see mqtt_subscribe
 *
 * @param arg Additional argument to pass to the callback function
 * @param data User data, pointed object, data may not be referenced after callback return,
          NULL is passed when all publish data are delivered
 * @param len Length of publish data fragment
 * @param flags MQTT_DATA_FLAG_LAST set when this call contains the last part of data from publish message
 *
 */
typedef void (*mqtt_incoming_data_cb_t)(void *arg, const u8_t *data, u16_t len, u8_t flags);


/** 
 * @ingroup mqtt
 * Function prototype for MQTT incoming publish function. Called when an incoming publish
 * arrives to a subscribed topic @see mqtt_subscribe
 *
 * @param arg Additional argument to pass to the callback function
 * @param topic Zero terminated Topic text string, topic may not be referenced after callback return
 * @param tot_len Total length of publish data, if set to 0 (no publish payload) data callback will not be invoked
 */
typedef void (*mqtt_incoming_publish_cb_t)(void *arg, const char *topic, u32_t tot_len);


/**
 * @ingroup mqtt
 * Function prototype for mqtt request callback. Called when a subscribe, unsubscribe
 * or publish request has completed
 * @param arg Pointer to user data supplied when invoking request
 * @param err ERR_OK on success
 *            ERR_TIMEOUT if no response was received within timeout,
 *            ERR_ABRT if (un)subscribe was denied
 */
typedef void (*mqtt_request_cb_t)(void *arg, err_t err);


/** Ring buffer */
struct mqtt_ringbuf_t {
	u16_t put;
	u16_t get;
	u8_t buf[MQTT_OUTPUT_RINGBUF_SIZE];
};

/** Pending request item, binds application callback to pending server requests */
struct mqtt_request_t
{
	/** Next item in list, NULL means this is the last in chain,
		next pointing at itself means request is unallocated */
	struct mqtt_request_t *next;
	/** Callback to upper layer */
	mqtt_request_cb_t cb;
	void *arg;
	/** MQTT packet identifier */
	u16_t pkt_id;
	/** Expire time relative to element before this  */
	u16_t timeout_diff;
};
typedef struct mqtt_client_s {
	/** Timers and timeouts */
	u16_t cyclic_tick;
	u16_t keep_alive;
	u16_t server_watchdog;
	/** Packet identifier generator*/
	u16_t pkt_id_seq;
	/** Packet identifier of pending incoming publish */
	u16_t inpub_pkt_id;
	/** Connection state */
	u8_t conn_state;
	//struct altcp_pcb *conn;
	/** Connection callback */
	void *connect_arg;
	mqtt_connection_cb_t connect_cb;
	/** Pending requests to server */
	struct mqtt_request_t *pend_req_queue;
	struct mqtt_request_t req_list[MQTT_REQ_MAX_IN_FLIGHT];
	void *inpub_arg;
	/** Incoming data callback */
	mqtt_incoming_data_cb_t data_cb;
	mqtt_incoming_publish_cb_t pub_cb;
	/** Input */
	u32_t msg_idx;
	u8_t rx_buffer[MQTT_VAR_HEADER_BUFFER_LEN];
	/** Output ring-buffer */
	struct mqtt_ringbuf_t output; 
	struct altcp_pcb *conn;
} mqtt_client_t;

