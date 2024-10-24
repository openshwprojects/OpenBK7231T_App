#include <string.h>
#include <libemqtt.h>

#define MQTT_DUP_FLAG     (1<<3)
#define MQTT_QOS0_FLAG    (0<<1)
#define MQTT_QOS1_FLAG    (1<<1)
#define MQTT_QOS2_FLAG    (2<<1)

#define MQTT_RETAIN_FLAG  1

#define MQTT_CLEAN_SESSION  (1<<1)
#define MQTT_WILL_FLAG      (1<<2)
#define MQTT_WILL_RETAIN    (1<<5)
#define MQTT_USERNAME_FLAG  (1<<7)
#define MQTT_PASSWORD_FLAG  (1<<6)

uint8_t mqtt_num_rem_len_bytes(const uint8_t* buf) {
    uint8_t num_bytes = 1;

    //printf("mqtt_num_rem_len_bytes\n");

    if ((buf[1] & 0x80) == 0x80) {
        num_bytes++;
        if ((buf[2] & 0x80) == 0x80) {
            num_bytes ++;
            if ((buf[3] & 0x80) == 0x80) {
                num_bytes ++;
            }
        }
    }
    return num_bytes;
}

uint16_t mqtt_parse_rem_len(const uint8_t* buf) {
    uint16_t multiplier = 1;
    uint16_t value = 0;
    uint8_t digit;

    //printf("mqtt_parse_rem_len\n");

    buf++;	// skip "flags" byte in fixed header

    do {
        digit = *buf;
        value += (digit & 127) * multiplier;
        multiplier *= 128;
        buf++;
    } while ((digit & 128) != 0);

    return value;
}

uint16_t mqtt_parse_msg_id(const uint8_t* buf) {
    uint8_t type = MQTTParseMessageType(buf);
    uint8_t qos = MQTTParseMessageQos(buf);
    uint16_t id = 0;
	
    //printf("mqtt_parse_msg_id\n");
	
    if(type >= MQTT_MSG_PUBLISH && type <= MQTT_MSG_UNSUBACK) {
        if(type == MQTT_MSG_PUBLISH) {
            if(qos != 0) {
                // fixed header length + Topic (UTF encoded)
                // = 1 for "flags" byte + rlb for length bytes + topic size
                uint8_t rlb = mqtt_num_rem_len_bytes(buf);
                uint8_t offset = *(buf+1+rlb)<<8;	// topic UTF MSB
                offset |= *(buf+1+rlb+1);			// topic UTF LSB
                offset += (1+rlb+2);					// fixed header + topic size
                id = *(buf+offset)<<8;				// id MSB
                id |= *(buf+offset+1);				// id LSB
            }
        } else {
            // fixed header length
            // 1 for "flags" byte + rlb for length bytes
            uint8_t rlb = mqtt_num_rem_len_bytes(buf);
            id = *(buf+1+rlb)<<8;	// id MSB
            id |= *(buf+1+rlb+1);	// id LSB
        }
    }
    return id;
}

uint16_t mqtt_parse_pub_topic(const uint8_t* buf, uint8_t* topic) {
    const uint8_t* ptr;
    uint16_t topic_len = mqtt_parse_pub_topic_ptr(buf, &ptr);
	
    //printf("mqtt_parse_pub_topic\n");
	
    if(topic_len != 0 && ptr != NULL) {
        memcpy(topic, ptr, topic_len);
    }
	
    return topic_len;
}

uint16_t mqtt_parse_pub_topic_ptr(const uint8_t* buf, const uint8_t **topic_ptr) {
    uint16_t len = 0;
	
    //printf("mqtt_parse_pub_topic_ptr\n");

    if(MQTTParseMessageType(buf) == MQTT_MSG_PUBLISH) {
        // fixed header length = 1 for "flags" byte + rlb for length bytes
        uint8_t rlb = mqtt_num_rem_len_bytes(buf);
        len = *(buf+1+rlb)<<8;	// MSB of topic UTF
        len |= *(buf+1+rlb+1);	// LSB of topic UTF
        // start of topic = add 1 for "flags", rlb for remaining length, 2 for UTF
        *topic_ptr = (buf + (1+rlb+2));
    } else {
        *topic_ptr = NULL;
    }
    return len;
}

uint16_t mqtt_parse_publish_msg(const uint8_t* buf, uint8_t** msg) {
    uint8_t* ptr;
    uint16_t msg_len = mqtt_parse_pub_msg_ptr(buf, (const uint8_t **)&ptr);

    if(msg_len != 0 && ptr != NULL) {
        //memcpy(msg, ptr, msg_len);
        (*msg)=ptr;
    }

    return msg_len;
}

uint16_t mqtt_parse_pub_msg_ptr(const uint8_t* buf, const uint8_t **msg_ptr) {
    uint16_t len = 0;

    //printf("mqtt_parse_pub_msg_ptr\n");

    if(MQTTParseMessageType(buf) == MQTT_MSG_PUBLISH) {
        // message starts at
        // fixed header length + Topic (UTF encoded) + msg id (if QoS>0)
        uint8_t rlb = mqtt_num_rem_len_bytes(buf);
        uint8_t offset = (*(buf+1+rlb))<<8;	// topic UTF MSB
        offset |= *(buf+1+rlb+1);			// topic UTF LSB
        offset += (1+rlb+2);				// fixed header + topic size

        if(MQTTParseMessageQos(buf)) {
            offset += 2;					// add two bytes of msg id
        }

        *msg_ptr = (buf + offset);
				
        // offset is now pointing to start of message
        // length of the message is remaining length - variable header
        // variable header is offset - fixed header
        // fixed header is 1 + rlb
        // so, lom = remlen - (offset - (1+rlb))
      	len = mqtt_parse_rem_len(buf) - (offset-(rlb+1));
    } else {
        *msg_ptr = NULL;
    }
    return len;
}

void mqtt_init(mqtt_broker_handle_t* broker, const char* clientid) {

    // Connection options
    broker->alive =  CLOUD_MQTT_SET_ALIVE; // 120 seconds = 2 minutes
    broker->seq = 1; // Sequency for message indetifiers
    // Client options
    memset(broker->clientid, 0, sizeof(broker->clientid));
    memset(broker->username, 0, sizeof(broker->username));
    memset(broker->password, 0, sizeof(broker->password));
    if(clientid) {
        strncpy(broker->clientid, clientid, sizeof(broker->clientid));
    } else {
        strcpy(broker->clientid, "emqtt");
    }
    // Will topic
    broker->clean_session = 1;
}

void mqtt_init_auth(mqtt_broker_handle_t* broker, const char* username, const char* password) {
	
    if(username && username[0] != '\0')
    {
        strncpy(broker->username, username, sizeof(broker->username)-1);
    }
    if(password && password[0] != '\0')
    {
        strncpy(broker->password, password, sizeof(broker->password)-1);
    }
	
}

void mqtt_set_alive(mqtt_broker_handle_t* broker, uint16_t alive) {
    broker->alive = alive;
}

int mqtt_connect(mqtt_broker_handle_t* broker)
{
    uint8_t flags = 0x00;
    int ret;
    /****************************add by alex********************************************/
    uint8_t *fixed_header = NULL;
    int fixed_headerLen;
    uint8_t *packet =NULL;
    int packetLen;
    uint16_t offset = 0;
    uint16_t clientidlen,usernamelen,passwordlen,payload_len;
    uint8_t fixedHeaderSize;
    uint8_t remainLen;
    uint8_t var_header[12] = {
        0x00,0x06,0x4d,0x51,0x49,0x73,0x64,0x70, // Protocol name: MQIsdp
        0x03, // Protocol version
        0, // Connect flags
        0, 0, // Keep alive
    };
		
    var_header[10]=  broker->alive>>8;
    var_header[11]= broker->alive&0xFF;

    /***********************************************************************************/	
	
    clientidlen = strlen(broker->clientid);
    usernamelen = strlen(broker->username);
    passwordlen = strlen(broker->password);
    payload_len = clientidlen + 2;
    // Preparing the flags
    if(usernamelen) {
        payload_len += usernamelen + 2;
        flags |= MQTT_USERNAME_FLAG;
    }
    if(passwordlen) {
        payload_len += passwordlen + 2;
        flags |= MQTT_PASSWORD_FLAG;
    }
    if(broker->clean_session) {
        flags |= MQTT_CLEAN_SESSION;
    }
    var_header[9]= flags;

    // Fixed header
    fixedHeaderSize = 2;    // Default size = one byte Message Type + one byte Remaining Length
    remainLen = sizeof(var_header)+payload_len;
    if (remainLen > 127) {
        fixedHeaderSize++;          // add an additional byte for Remaining Length
    }
    
    fixed_header = (uint8_t*)tls_mem_alloc( fixedHeaderSize );
    if( fixed_header==NULL )
    {
        return -1;
    }
    fixed_headerLen = fixedHeaderSize;
    // Message Type
    fixed_header[0] = MQTT_MSG_CONNECT;

    // Remaining Length
    if (remainLen <= 127) {
        fixed_header[1] = remainLen;
    } else {
        // first byte is remainder (mod) of 128, then set the MSB to indicate more bytes
        fixed_header[1] = remainLen % 128;
        fixed_header[1] = fixed_header[1] | 0x80;
        // second byte is number of 128s
        fixed_header[2] = remainLen / 128;
    }
	
    packet = (uint8_t*)tls_mem_alloc( fixed_headerLen+sizeof(var_header)+payload_len );
    if( packet==NULL )
    {
        tls_mem_free(fixed_header);
        return -2;
    }
    packetLen = fixed_headerLen+sizeof(var_header)+payload_len ;
    memset(packet, 0, packetLen);
    memcpy(packet, fixed_header, fixed_headerLen);
    offset += fixed_headerLen;
    memcpy(packet+offset, var_header, sizeof(var_header));
    offset += sizeof(var_header);
    // Client ID - UTF encoded
    packet[offset++] = clientidlen>>8;
    packet[offset++] = clientidlen&0xFF;
    memcpy(packet+offset, broker->clientid, clientidlen);
    offset += clientidlen;

    if(usernamelen) {
        // Username - UTF encoded
        packet[offset++] = usernamelen>>8;
        packet[offset++] = usernamelen&0xFF;
        memcpy(packet+offset, broker->username, usernamelen);
        offset += usernamelen;
    }

    if(passwordlen) {
        // Password - UTF encoded
        packet[offset++] = passwordlen>>8;
        packet[offset++] = passwordlen&0xFF;
        memcpy(packet+offset, broker->password, passwordlen);
        offset += passwordlen;
    }	

    ret = broker->mqttsend(broker->socketid, packet, packetLen);
    if(ret < packetLen) {      
        tls_mem_free(fixed_header);
        tls_mem_free(packet);
        return -3;
    } 
    tls_mem_free(fixed_header);
    tls_mem_free(packet);
    return 0;
}


int mqtt_disconnect(mqtt_broker_handle_t* broker) {
    uint8_t packet[] = {
        MQTT_MSG_DISCONNECT, // Message Type, DUP flag, QoS level, Retain
        0x00 // Remaining length
    };

    // Send the packet
    if(broker->mqttsend(broker->socketid, packet, sizeof(packet)) < sizeof(packet)) {
        return -1;
    }

    return 1;
}

int mqtt_ping(mqtt_broker_handle_t* broker) {
    uint8_t packet[] = {
        MQTT_MSG_PINGREQ, // Message Type, DUP flag, QoS level, Retain
        0x00 // Remaining length
    };

    // Send the packet
    if(broker->mqttsend(broker->socketid, packet, sizeof(packet)) < sizeof(packet)) {
        return -1;
    }

    return 1;
}

int mqtt_publish(mqtt_broker_handle_t* broker, const char* topic, const char* msg, int msgLen, uint8_t retain) {
    return mqtt_publish_with_qos(broker, topic, msg, msgLen,retain, 0, NULL);
}
int mqtt_publish_with_qos(mqtt_broker_handle_t* broker, 
                             const char* topic, 
                             const char* msg,
                             int msgLen, 
                             uint8_t retain, 
                             uint8_t qos, 
                             uint16_t* message_id) 
{
    uint16_t topiclen = strlen(topic);

    uint8_t qos_flag = MQTT_QOS0_FLAG;
    uint8_t qos_size = 0; // No QoS included
/************************add by alex****************************/
    int var_headerLen;
    int fixed_headerLen;
    int packetLen;

    uint8_t *var_header=NULL;
    uint8_t *fixed_header=NULL;
    uint8_t fixedHeaderSize = 2;
    uint16_t remainLen;
/***************************************************************/	

    if(qos == 1) {
        qos_size = 2; // 2 bytes for QoS
        qos_flag = MQTT_QOS1_FLAG;
    }
    else if(qos == 2) {
        qos_size = 2; // 2 bytes for QoS
        qos_flag = MQTT_QOS2_FLAG;
    }
/************************add by alex*******************************/
    // Variable header
    var_header = ( uint8_t* )tls_mem_alloc( topiclen+2+qos_size );
    
    if( var_header==NULL )
    {
        return -1;
    }
    
    var_headerLen = topiclen+2+qos_size;
/******************************************************************/
    memset(var_header, 0, var_headerLen);
    var_header[0] = topiclen>>8;
    var_header[1] = topiclen&0xFF;
    memcpy(var_header+2, topic, topiclen);
    if(qos_size) {
        var_header[topiclen+2] = broker->seq>>8;
        var_header[topiclen+3] = broker->seq&0xFF;
        if(message_id) { // Returning message id
            *message_id = broker->seq;
        }
        broker->seq++;
    }

    // Fixed header
    // the remaining length is one byte for messages up to 127 bytes, then two bytes after that
    // actually, it can be up to 4 bytes but I'm making the assumption the embedded device will only
    // need up to two bytes of length (handles up to 16,383 (almost 16k) sized message)

    /**********************add by alex**************************************/
    remainLen = var_headerLen+msgLen;
    /***********************************************************************/
    if (remainLen > 127) {
        fixedHeaderSize++;          // add an additional byte for Remaining Length
    }

    /***********************add by alex *******************/
    fixed_headerLen = fixedHeaderSize;
    fixed_header = (uint8_t *)tls_mem_alloc( fixed_headerLen );
    if( fixed_header==NULL )
    {
        tls_mem_free( var_header );
        return -1;
    }
    /******************************************************/
    
    // Message Type, DUP flag, QoS level, Retain
    fixed_header[0] = MQTT_MSG_PUBLISH | qos_flag;
    if(retain) {
        fixed_header[0] |= MQTT_RETAIN_FLAG;
    }
    // Remaining Length
    if (remainLen <= 127) {
        fixed_header[1] = remainLen;
    } else {
        // first byte is remainder (mod) of 128, then set the MSB to indicate more bytes
        fixed_header[1] = remainLen % 128;
        fixed_header[1] = fixed_header[1] | 0x80;
        // second byte is number of 128s
        fixed_header[2] = remainLen / 128;
    }
    /**********************add by alex******************************/
    packetLen = fixed_headerLen+var_headerLen+msgLen;
    //uint8_t packet[packetLen];
    uint8_t *packet = tls_mem_alloc(packetLen);
    if (!packet)
    {
        tls_mem_free(var_header);
        tls_mem_free( fixed_header );
        return -1;
    }
    memset(packet, 0, packetLen);
    memcpy(packet, fixed_header, fixed_headerLen);
    memcpy(packet+fixed_headerLen, var_header, var_headerLen);
    memcpy(packet+fixed_headerLen+var_headerLen, msg, msgLen);

    // Send the packet
    if(broker->mqttsend(broker->socketid, packet, packetLen ) < packetLen) {
        tls_mem_free(var_header);
        tls_mem_free( fixed_header );
        tls_mem_free(packet);
        return -1;
    }
    tls_mem_free( fixed_header );
    tls_mem_free(var_header);
    tls_mem_free(packet);
    return 1;
}

int mqtt_pubrel(mqtt_broker_handle_t* broker, uint16_t message_id) {
    uint8_t packet[4] = {
        MQTT_MSG_PUBREL | MQTT_QOS1_FLAG, // Message Type, DUP flag, QoS level, Retain
        0x02, // Remaining length
        0/*message_id>>8*/,
        0/*message_id&0xFF*/
    };
    /************add by alex*****************/
    packet[2] = message_id>>8;
    packet[3] = message_id&0xFF;
    /****************************************/
    // Send the packet
    if(broker->mqttsend(broker->socketid, packet, sizeof(packet)) < sizeof(packet)) {
        return -1;
    }

    return 1;
}

int mqtt_subscribe(mqtt_broker_handle_t* broker, const char* topic, uint16_t* message_id) {
    uint16_t topiclen = strlen(topic);

    /******************add by alex*********************/
    uint8_t *utf_topic=NULL;
    uint8_t *packet=NULL;	
    int utf_topicLen;
    int packetLen;	
    uint8_t fixed_header[2];

    /*************************************************/

    // Variable header
    uint8_t var_header[2]; // Message ID
    var_header[0] = broker->seq>>8;
    var_header[1] = broker->seq&0xFF;
    if(message_id) { // Returning message id
        *message_id = broker->seq;
    }
    broker->seq++;

    /*******************add by alex**************************/
    // utf topic
    //uint8_t utf_topic[topiclen+3]; // Topic size (2 bytes), utf-encoded topic, QoS byte
    utf_topic = ( uint8_t*)tls_mem_alloc( topiclen+3 );
    if( utf_topic==NULL )
    {
        return -1;
    }
    utf_topicLen = topiclen+3;
    memset(utf_topic, 0, utf_topicLen);
    utf_topic[0] = topiclen>>8;
    utf_topic[1] = topiclen&0xFF;
    memcpy(utf_topic+2, topic, topiclen);
    /*********************************************************/
	
    /********************add by alex***********************/
    // Fixed header
    fixed_header[0] = MQTT_MSG_SUBSCRIBE | MQTT_QOS1_FLAG;
    fixed_header[1] = sizeof(var_header)+utf_topicLen;
    /******************************************************/
	
    /***********************add by alex********************/		
    packetLen = sizeof(var_header)+sizeof(fixed_header)+utf_topicLen;
    packet = (uint8_t*)tls_mem_alloc( packetLen );
    if( packet==NULL )
    {
        tls_mem_free(utf_topic);
        return -1;
    }
    memset(packet, 0, packetLen);
    memcpy(packet, fixed_header, sizeof(fixed_header));
    memcpy(packet+sizeof(fixed_header), var_header, sizeof(var_header));
    memcpy(packet+sizeof(fixed_header)+sizeof(var_header), utf_topic, utf_topicLen);

    // Send the packet
    if(broker->mqttsend(broker->socketid, packet, packetLen) < packetLen) {
        tls_mem_free(utf_topic);
        tls_mem_free(packet);
        return -1;
    }
    /*******************************************************/
    tls_mem_free(utf_topic);
    tls_mem_free(packet);
    return 1;
}

int mqtt_unsubscribe(mqtt_broker_handle_t* broker, const char* topic, uint16_t* message_id) {

    uint16_t topiclen = strlen(topic);

    /******************add by alex*******************/
    uint8_t *utf_topic=NULL;
    int utf_topicLen;
    uint8_t fixed_header[2];
    uint8_t *packet = NULL;
    int packetLen;

    /************************************************/
    // Variable header
    uint8_t var_header[2]; // Message ID
    var_header[0] = broker->seq>>8;
    var_header[1] = broker->seq&0xFF;
    if(message_id) { // Returning message id
        *message_id = broker->seq;
    }
    broker->seq++;
    /******************add by alex**********************************/
    // utf topic	
    utf_topic = (uint8_t*)tls_mem_alloc( topiclen+2 );
    if( utf_topic==NULL )
    {
        return -1;
    }
    utf_topicLen = topiclen+2;
    memset(utf_topic, 0, utf_topicLen);
    utf_topic[0] = topiclen>>8;
    utf_topic[1] = topiclen&0xFF;
    memcpy(utf_topic+2, topic, topiclen);
    /***************************************************************/

    /*************************add by alex*******************************/
    // Fixed header
    fixed_header[0] = MQTT_MSG_UNSUBSCRIBE | MQTT_QOS1_FLAG;
    fixed_header[1] = sizeof(var_header)+utf_topicLen;

    packetLen = sizeof(var_header)+sizeof(fixed_header)+utf_topicLen;
    packet = (uint8_t*)tls_mem_alloc( packetLen );
    if( packet==NULL )
    {
        tls_mem_free(utf_topic);
        return -1;
    }
    memset(packet, 0, packetLen);
    memcpy(packet, fixed_header, sizeof(fixed_header));
    memcpy(packet+sizeof(fixed_header), var_header, sizeof(var_header));
    memcpy(packet+sizeof(fixed_header)+sizeof(var_header), utf_topic, utf_topicLen);

    // Send the packet
    if(broker->mqttsend(broker->socketid, packet, packetLen) < packetLen) {
        tls_mem_free(utf_topic);
        tls_mem_free(packet);
        return -1;
    }
    /***************************************************************/
    tls_mem_free(utf_topic);
    tls_mem_free(packet);
    return 1;
}

void mqtt_login( mqtt_broker_handle_t* broker )
{
}
