/*
 * This file is part of libemqtt.
 *
 * libemqtt is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * libemqtt is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with libemqtt.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 *
 * Created by Filipe Varela on 09/10/16.
 * Copyright 2009 Caixa Mágica Software. All rights reserved.
 *
 * Fork developed by Vicente Ruiz Rodríguez
 * Copyright 2012 Vicente Ruiz Rodríguez <vruiz2.0@gmail.com>. All rights reserved.
 *
 */
//#include ".h"
#ifndef __LIBEMQTT_H__
#define __LIBEMQTT_H__

#include "wm_type_def.h"
#include <stdint.h>
#include "wm_mem.h"

#ifndef MQTT_CONF_USERNAME_LENGTH
	#define MQTT_CONF_USERNAME_LENGTH 24 // Recommended by MQTT Specification (12 + '\0')
#endif

#ifndef MQTT_CONF_PASSWORD_LENGTH
	#define MQTT_CONF_PASSWORD_LENGTH 24 // Recommended by MQTT Specification (12 + '\0')
#endif

#define CLOUD_MQTT_SET_ALIVE      (120)

#define MQTT_MSG_CONNECT       (1<<4)
#define MQTT_MSG_CONNACK       (2<<4)
#define MQTT_MSG_PUBLISH       (3<<4)
#define MQTT_MSG_PUBACK        (4<<4)
#define MQTT_MSG_PUBREC        (5<<4)
#define MQTT_MSG_PUBREL        (6<<4)
#define MQTT_MSG_PUBCOMP       (7<<4)
#define MQTT_MSG_SUBSCRIBE     (8<<4)
#define MQTT_MSG_SUBACK        (9<<4)
#define MQTT_MSG_UNSUBSCRIBE  (10<<4)
#define MQTT_MSG_UNSUBACK     (11<<4)
#define MQTT_MSG_PINGREQ      (12<<4)
#define MQTT_MSG_PINGRESP     (13<<4)
#define MQTT_MSG_DISCONNECT   (14<<4)


/** Extract the message type from buffer.
 * @param buffer Pointer to the packet.
 *
 * @return Message Type byte.
 */
#define MQTTParseMessageType(buffer) ( *buffer & 0xF0 )

/** Indicate if it is a duplicate packet.
 * @param buffer Pointer to the packet.
 *
 * @retval   0 Not duplicate.
 * @retval !=0 Duplicate.
 */
#define MQTTParseMessageDuplicate(buffer) ( *buffer & 0x08 )

/** Extract the message QoS level.
 * @param buffer Pointer to the packet.
 *
 * @return QoS Level (0, 1 or 2).
 */
#define MQTTParseMessageQos(buffer) ( (*buffer & 0x06) >> 1 )

/** Indicate if this packet has a retain flag.
 * @param buffer Pointer to the packet.
 *
 * @retval   0 Not duplicate.
 * @retval !=0 Duplicate.
 */
#define MQTTParseMessageRetain(buffer) ( *buffer & 0x01 )


/** Parse packet buffer for number of bytes in remaining length field.
 *
 * Given a packet, return number of bytes in remaining length
 * field in MQTT fixed header.  Can be from 1 - 4 bytes depending
 * on length of message.
 *
 * @param buf Pointer to the packet.
 *
 * @retval number of bytes
 */
uint8_t mqtt_num_rem_len_bytes(const uint8_t* buf);

/** Parse packet buffer for remaning length value.
 *
 * Given a packet, return remaining length value (in fixed header).
 * 
 * @param buf Pointer to the packet.
 *
 * @retval remaining length
 */
uint16_t mqtt_parse_rem_len(const uint8_t* buf);

/** Parse packet buffer for message id.
 *
 * @param buf Pointer to the packet.
 *
 * @retval message id
 */
uint16_t mqtt_parse_msg_id(const uint8_t* buf);

/** Parse a packet buffer for the publish topic.
 *
 * Given a packet containing an MQTT publish message,
 * return the message topic.
 *
 * @param buf Pointer to the packet.
 * @param topic  Pointer destination buffer for topic
 *
 * @retval size in bytes of topic (0 = no publish message in buffer)
 */
uint16_t mqtt_parse_pub_topic(const uint8_t* buf, uint8_t* topic);

/** Parse a packet buffer for a pointer to the publish topic.
 *
 *  Not called directly - called by mqtt_parse_pub_topic
 */
uint16_t mqtt_parse_pub_topic_ptr(const uint8_t* buf, const uint8_t** topic_ptr);

/** Parse a packet buffer for the publish message.
 *
 * Given a packet containing an MQTT publish message,
 * return the message.
 *
 * @param buf Pointer to the packet.
 * @param msg Pointer destination buffer for message
 *
 * @retval size in bytes of topic (0 = no publish message in buffer)
 */
uint16_t mqtt_parse_publish_msg(const uint8_t* buf, uint8_t** msg);

/** Parse a packet buffer for a pointer to the publish message.
 *
 *  Not called directly - called by mqtt_parse_pub_msg
 */
uint16_t mqtt_parse_pub_msg_ptr(const uint8_t* buf, const uint8_t** msg_ptr);


typedef struct {
	int socketid;    
	int (*mqttsend)(int socket_info, const void* buf, unsigned int count);
	// Connection info
	char clientid[50];
	// Auth fields
	char username[MQTT_CONF_USERNAME_LENGTH];
	char password[MQTT_CONF_PASSWORD_LENGTH];
	// Will topic
	uint8_t will_retain;
	uint8_t will_qos;
	uint8_t clean_session;
	// Management fields
	uint16_t seq;
	uint16_t alive;
} mqtt_broker_handle_t;


/** Initialize the information to connect to the broker.
 * @param broker Data structure that contains the connection information with the broker.
 * @param clientid A string that identifies the client id.
 *
 * @note Only has effect before to call mqtt_connect
 */
void mqtt_init(mqtt_broker_handle_t* broker, const char* clientid);

/** Enable the authentication to connect to the broker.
 * @param broker Data structure that contains the connection information with the broker.
 * @param username A string that contains the username.
 * @param password A string that contains the password.
 *
 * @note Only has effect before to call mqtt_connect
 */
void mqtt_init_auth(mqtt_broker_handle_t* broker, const char* username, const char* password);

/** Set the keep alive timer.
 * @param broker Data structure that contains the connection information with the broker.
 * @param alive Keep aliver timer value (in seconds).
 *
 * @note Only has effect before to call mqtt_connect
 */
void mqtt_set_alive(mqtt_broker_handle_t* broker, uint16_t alive);

/** Connect to the broker.
 * @param broker Data structure that contains the connection information with the broker.
 *
 * @retval  1 On success.
 * @retval  0 On connection error.
 * @retval -1 On IO error.
 */
int mqtt_connect(mqtt_broker_handle_t* broker);

/** Disconnect to the broker.
 * @param broker Data structure that contains the connection information with the broker.
 *
 * @note The socket must also be closed.
 *
 * @retval  1 On success.
 * @retval  0 On connection error.
 * @retval -1 On IO error.
 */
int mqtt_disconnect(mqtt_broker_handle_t* broker);

/** Publish a message on a topic. This message will be published with 0 Qos level.
 * @param broker Data structure that contains the connection information with the broker.
 * @param topic The topic name.
 * @param msg The message.
 * @param retain Enable or disable the Retain flag (values: 0 or 1).
 *
 * @retval  1 On success.
 * @retval  0 On connection error.
 * @retval -1 On IO error.
 */
int mqtt_publish(mqtt_broker_handle_t* broker, const char* topic, const char* msg, int msgLen, uint8_t retain);

/** Publish a message on a topic.
 * @param broker Data structure that contains the connection information with the broker.
 * @param topic The topic name.
 * @param msg The message.
 * @param retain Enable or disable the Retain flag (values: 0 or 1).
 * @param qos Quality of Service (values: 0, 1 or 2)
 * @param message_id Variable that will store the Message ID, if the pointer is not NULL.
 *
 * @retval  1 On success.
 * @retval  0 On connection error.
 * @retval -1 On IO error.
 */
int mqtt_publish_with_qos(mqtt_broker_handle_t* broker, const char* topic, const char* msg, int msgLen, uint8_t retain, uint8_t qos, uint16_t* message_id);

/** Send a PUBREL message. It's used for PUBLISH message with 2 QoS level.
 * @param broker Data structure that contains the connection information with the broker.
 * @param message_id Message ID
 *
 * @retval  1 On success.
 * @retval  0 On connection error.
 * @retval -1 On IO error.
 */
int mqtt_pubrel(mqtt_broker_handle_t* broker, uint16_t message_id);

/** Subscribe to a topic.
 * @param broker Data structure that contains the connection information with the broker.
 * @param topic The topic name.
 * @param message_id Variable that will store the Message ID, if the pointer is not NULL.
 *
 * @retval  1 On success.
 * @retval  0 On connection error.
 * @retval -1 On IO error.
 */
int mqtt_subscribe(mqtt_broker_handle_t* broker, const char* topic, uint16_t* message_id);

/** Unsubscribe from a topic.
 * @param broker Data structure that contains the connection information with the broker.
 * @param topic The topic name.
 * @param message_id Variable that will store the Message ID, if the pointer is not NULL.
 *
 * @retval  1 On success.
 * @retval  0 On connection error.
 * @retval -1 On IO error.
 */
int mqtt_unsubscribe(mqtt_broker_handle_t* broker, const char* topic, uint16_t* message_id);

/** Make a ping.
 * @param broker Data structure that contains the connection information with the broker.
 *
 * @retval  1 On success.
 * @retval  0 On connection error.
 * @retval -1 On IO error.
 */
int mqtt_ping(mqtt_broker_handle_t* broker);

/***************************add by Alex lin**********************************************/


#endif // __LIBEMQTT_H__
