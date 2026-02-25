/*
ms_publish cmnd/obkLEDstripWindow/POWER TOGGLE
ms_publish cmnd/obk174083A4/POWER TOGGLE





*/

#include "../cmnds/cmd_public.h"
#include "../httpserver/new_http.h"
#include "../logging/logging.h"
#include "../new_common.h"
#include "../obk_config.h"
#include "lwip/inet.h"
#include "lwip/ip_addr.h"
#include "lwip/sockets.h"

#if ENABLE_DRIVER_MQTTSERVER

#define MQTT_SERVER_PORT 1883
#define MQTT_MAX_CLIENTS 4
#define MQTT_RECV_BUF_SIZE 2048
#define MQTT_MAX_SUBSCRIPTIONS 16

// MQTT packet types
#define MQTT_CONNECT 1
#define MQTT_CONNACK 2
#define MQTT_PUBLISH 3
#define MQTT_PUBACK 4
#define MQTT_SUBSCRIBE 8
#define MQTT_SUBACK 9
#define MQTT_UNSUBSCRIBE 10
#define MQTT_UNSUBACK 11
#define MQTT_PINGREQ 12
#define MQTT_PINGRESP 13
#define MQTT_DISCONNECT 14

static int g_mqttServer_secondsElapsed = 0;
static int g_totalPublishForwarded = 0;

// single user only
static char g_password[128];
static char g_user[128];

typedef struct mqttSubscription_s {
  char topic[128];
} mqttSubscription_t;

typedef struct mqttClient_s {
  int socket;
  int bConnected;
  char clientID[64];
  char ipAddr[20];
  mqttSubscription_t subs[MQTT_MAX_SUBSCRIPTIONS];
  int numSubs;
  int bytesRecv;
  int bytesSent;
  int packetsRecv;
  int packetsSent;
} mqttClient_t;

static int g_listenSocket = -1;
static mqttClient_t g_clients[MQTT_MAX_CLIENTS];
static byte g_recvBuf[MQTT_RECV_BUF_SIZE];

// Decode MQTT remaining length (variable-length encoding)
static int MQTTS_DecodeRemainingLength(const byte *buf, int bufLen,
                                       int *bytesUsed) {
  int multiplier = 1;
  int value = 0;
  int idx = 0;
  byte encodedByte;
  do {
    if (idx >= bufLen) {
      *bytesUsed = 0;
      return -1;
    }
    encodedByte = buf[idx++];
    value += (encodedByte & 0x7F) * multiplier;
    multiplier *= 128;
    if (multiplier > 128 * 128 * 128) {
      *bytesUsed = 0;
      return -1;
    }
  } while ((encodedByte & 0x80) != 0);
  *bytesUsed = idx;
  return value;
}

// Read a 2-byte big-endian uint16
static int MQTTS_ReadUint16(const byte *buf) { return (buf[0] << 8) | buf[1]; }

// Read an MQTT UTF-8 string (2-byte length prefix + data)
// Returns pointer to string start (NOT null-terminated), sets outLen
static const byte *MQTTS_ReadString(const byte *buf, int bufLen, int *outLen) {
  if (bufLen < 2)
    return NULL;
  int slen = MQTTS_ReadUint16(buf);
  if (bufLen < 2 + slen)
    return NULL;
  *outLen = slen;
  return buf + 2;
}

// Helper: send and track bytes for a specific client index
static void MQTTS_SendToClient(int clientIdx, const byte *data, int len) {
  int r = send(g_clients[clientIdx].socket, (const char *)data, len, 0);
  if (r > 0) {
    g_clients[clientIdx].bytesSent += r;
  }
}

static void MQTTS_SendConnack(int clientIdx, byte returnCode) {
  byte pkt[4];
  pkt[0] = (MQTT_CONNACK << 4);
  pkt[1] = 2; // remaining length
  pkt[2] = 0; // session present = 0
  pkt[3] = returnCode;
  MQTTS_SendToClient(clientIdx, pkt, 4);
  g_clients[clientIdx].packetsSent++;
}

static void MQTTS_SendSuback(int clientIdx, int packetID, byte grantedQoS) {
  byte pkt[5];
  pkt[0] = (MQTT_SUBACK << 4);
  pkt[1] = 3; // remaining length
  pkt[2] = (packetID >> 8) & 0xFF;
  pkt[3] = packetID & 0xFF;
  pkt[4] = grantedQoS;
  MQTTS_SendToClient(clientIdx, pkt, 5);
  g_clients[clientIdx].packetsSent++;
}

static void MQTTS_SendUnsuback(int clientIdx, int packetID) {
  byte pkt[4];
  pkt[0] = (MQTT_UNSUBACK << 4);
  pkt[1] = 2;
  pkt[2] = (packetID >> 8) & 0xFF;
  pkt[3] = packetID & 0xFF;
  MQTTS_SendToClient(clientIdx, pkt, 4);
  g_clients[clientIdx].packetsSent++;
}

static void MQTTS_SendPingresp(int clientIdx) {
  byte pkt[2];
  pkt[0] = (MQTT_PINGRESP << 4);
  pkt[1] = 0;
  MQTTS_SendToClient(clientIdx, pkt, 2);
  g_clients[clientIdx].packetsSent++;
}

static void MQTTS_SendPuback(int clientIdx, int packetID) {
  byte pkt[4];
  pkt[0] = (MQTT_PUBACK << 4);
  pkt[1] = 2;
  pkt[2] = (packetID >> 8) & 0xFF;
  pkt[3] = packetID & 0xFF;
  MQTTS_SendToClient(clientIdx, pkt, 4);
  g_clients[clientIdx].packetsSent++;
}

// Match topic against subscription filter with + and # wildcards
// + matches exactly one level, # matches remaining levels (must be last)
static int MQTTS_TopicMatch(const char *topic, const char *filter) {
  while (*filter) {
    if (*filter == '#') {
      // # must be last char and matches everything remaining
      return 1;
    }
    if (*filter == '+') {
      // + matches one level: skip topic chars until / or end
      while (*topic && *topic != '/')
        topic++;
      filter++;
      // both should now be at / or end
      if (*topic == '/' && *filter == '/') {
        topic++;
        filter++;
        continue;
      }
      // both at end = match, otherwise mismatch
      return (*topic == 0 && *filter == 0) ? 1 : 0;
    }
    // literal compare
    if (*topic != *filter)
      return 0;
    topic++;
    filter++;
  }
  return (*topic == 0) ? 1 : 0;
}

// Forward a PUBLISH to all subscribed clients
static void MQTTS_ForwardPublish(const byte *topicData, int topicLen,
                                 const byte *payload, int payloadLen,
                                 int senderIdx) {
  int i, s;
  char topicStr[128];
  int copyLen = topicLen < 127 ? topicLen : 127;
  memcpy(topicStr, topicData, copyLen);
  topicStr[copyLen] = 0;

  for (i = 0; i < MQTT_MAX_CLIENTS; i++) {
    if (g_clients[i].socket < 0 || !g_clients[i].bConnected)
      continue;
    for (s = 0; s < g_clients[i].numSubs; s++) {
      int match = MQTTS_TopicMatch(topicStr, g_clients[i].subs[s].topic);
      if (match) {
        // Build PUBLISH packet: fixed header + topic + payload
        byte hdr[5];
        int totalPayload = 2 + topicLen + payloadLen;
        int hdrLen = 1;
        hdr[0] = (MQTT_PUBLISH << 4); // QoS 0, no retain
        // encode remaining length
        int rl = totalPayload;
        do {
          byte eb = rl % 128;
          rl /= 128;
          if (rl > 0)
            eb |= 0x80;
          hdr[hdrLen++] = eb;
        } while (rl > 0);
        MQTTS_SendToClient(i, hdr, hdrLen);
        // topic length + topic
        byte topicHdr[2];
        topicHdr[0] = (topicLen >> 8) & 0xFF;
        topicHdr[1] = topicLen & 0xFF;
        MQTTS_SendToClient(i, topicHdr, 2);
        MQTTS_SendToClient(i, topicData, topicLen);
        // payload
        if (payloadLen > 0) {
          MQTTS_SendToClient(i, payload, payloadLen);
        }
        g_clients[i].packetsSent++;
        g_totalPublishForwarded++;
        break; // only send once per client
      }
    }
  }
}

static void MQTTS_HandlePacket(int clientIdx, const byte *buf, int totalLen) {
  if (totalLen < 2)
    return;

  int pktType = (buf[0] >> 4) & 0x0F;
  int flags = buf[0] & 0x0F;
  int lenBytes = 0;
  int remainLen = MQTTS_DecodeRemainingLength(buf + 1, totalLen - 1, &lenBytes);
  if (remainLen < 0)
    return;

  int headerSize = 1 + lenBytes;
  const byte *payload = buf + headerSize;
  mqttClient_t *client = &g_clients[clientIdx];
  client->packetsRecv++;

  switch (pktType) {
  case MQTT_CONNECT: {
    // Variable header: protocol name + level + flags + keepalive
    if (remainLen < 10) {
      MQTTS_SendConnack(clientIdx, 1);
      return;
    }
    int pos = 0;
    // protocol name
    int protoNameLen;
    MQTTS_ReadString(payload + pos, remainLen - pos, &protoNameLen);
    pos += 2 + protoNameLen;
    // protocol level
    pos++; // skip level byte
    // connect flags
    byte connectFlags = payload[pos++];
    int hasUsername = (connectFlags >> 7) & 1;
    int hasPassword = (connectFlags >> 6) & 1;
    // keepalive
    pos += 2; // skip keepalive

    // Client ID
    int clientIDLen;
    const byte *clientIDData =
        MQTTS_ReadString(payload + pos, remainLen - pos, &clientIDLen);
    if (clientIDData) {
      int copyLen = clientIDLen < 63 ? clientIDLen : 63;
      memcpy(client->clientID, clientIDData, copyLen);
      client->clientID[copyLen] = 0;
      pos += 2 + clientIDLen;
    }

    // Skip will topic/message if present
    if (connectFlags & 0x04) {
      int willTopicLen;
      MQTTS_ReadString(payload + pos, remainLen - pos, &willTopicLen);
      pos += 2 + willTopicLen;
      int willMsgLen;
      MQTTS_ReadString(payload + pos, remainLen - pos, &willMsgLen);
      pos += 2 + willMsgLen;
    }

    // Check username/password
    if (hasUsername && g_user[0]) {
      int usernameLen;
      const byte *usernameData =
          MQTTS_ReadString(payload + pos, remainLen - pos, &usernameLen);
      pos += 2 + usernameLen;
      if (!usernameData || usernameLen != (int)strlen(g_user) ||
          memcmp(usernameData, g_user, usernameLen)) {
        addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, "MQTTS: bad username from %s",
                  client->clientID);
        MQTTS_SendConnack(clientIdx, 4); // bad username
        return;
      }
      if (hasPassword && g_password[0]) {
        int passwordLen;
        const byte *passwordData =
            MQTTS_ReadString(payload + pos, remainLen - pos, &passwordLen);
        pos += 2 + passwordLen;
        if (!passwordData || passwordLen != (int)strlen(g_password) ||
            memcmp(passwordData, g_password, passwordLen)) {
          addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL,
                    "MQTTS: bad password from %s", client->clientID);
          MQTTS_SendConnack(clientIdx, 5); // bad password
          return;
        }
      }
    }

    client->bConnected = 1;
    client->numSubs = 0;
    MQTTS_SendConnack(clientIdx, 0); // accepted
    addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, "MQTTS: client '%s' connected",
              client->clientID);
    break;
  }
  case MQTT_PUBLISH: {
    if (!client->bConnected)
      return;
    int qos = (flags >> 1) & 0x03;
    int pos = 0;
    int topicLen;
    const byte *topicData =
        MQTTS_ReadString(payload + pos, remainLen - pos, &topicLen);
    if (!topicData)
      return;
    pos += 2 + topicLen;
    int packetID = 0;
    if (qos > 0) {
      packetID = MQTTS_ReadUint16(payload + pos);
      pos += 2;
    }
    int payloadLen = remainLen - pos;
    const byte *pubPayload = payload + pos;
    // Log it
    char topicStr[128];
    int copyLen = topicLen < 127 ? topicLen : 127;
    memcpy(topicStr, topicData, copyLen);
    topicStr[copyLen] = 0;
    addLogAdv(LOG_DEBUG, LOG_FEATURE_GENERAL,
              "MQTTS: PUBLISH '%s' (%d bytes) from '%s'", topicStr, payloadLen,
              client->clientID);
    // Forward to subscribers
    MQTTS_ForwardPublish(topicData, topicLen, pubPayload, payloadLen,
                         clientIdx);
    // Send PUBACK for QoS 1
    if (qos == 1) {
      MQTTS_SendPuback(clientIdx, packetID);
    }
    break;
  }
  case MQTT_SUBSCRIBE: {
    if (!client->bConnected)
      return;
    int pos = 0;
    int packetID = MQTTS_ReadUint16(payload + pos);
    pos += 2;
    // Parse topic filters
    while (pos < remainLen && client->numSubs < MQTT_MAX_SUBSCRIPTIONS) {
      int topicLen;
      const byte *topicData =
          MQTTS_ReadString(payload + pos, remainLen - pos, &topicLen);
      if (!topicData)
        break;
      pos += 2 + topicLen;
      byte requestedQoS = payload[pos++];
      int copyLen = topicLen < 127 ? topicLen : 127;
      memcpy(client->subs[client->numSubs].topic, topicData, copyLen);
      client->subs[client->numSubs].topic[copyLen] = 0;
      client->numSubs++;
      addLogAdv(LOG_DEBUG, LOG_FEATURE_GENERAL,
                "MQTTS: client '%s' subscribed to '%s'", client->clientID,
                client->subs[client->numSubs - 1].topic);
    }
    MQTTS_SendSuback(clientIdx, packetID, 0); // granted QoS 0
    break;
  }
  case MQTT_UNSUBSCRIBE: {
    if (!client->bConnected)
      return;
    int pos = 0;
    int packetID = MQTTS_ReadUint16(payload + pos);
    pos += 2;
    while (pos < remainLen) {
      int topicLen;
      const byte *topicData =
          MQTTS_ReadString(payload + pos, remainLen - pos, &topicLen);
      if (!topicData)
        break;
      pos += 2 + topicLen;
      char topicStr[128];
      int copyLen = topicLen < 127 ? topicLen : 127;
      memcpy(topicStr, topicData, copyLen);
      topicStr[copyLen] = 0;
      // Remove matching subscription
      int s;
      for (s = 0; s < client->numSubs; s++) {
        if (!strcmp(client->subs[s].topic, topicStr)) {
          // shift remaining subs down
          if (s < client->numSubs - 1) {
            memmove(&client->subs[s], &client->subs[s + 1],
                    (client->numSubs - s - 1) * sizeof(mqttSubscription_t));
          }
          client->numSubs--;
          break;
        }
      }
    }
    MQTTS_SendUnsuback(clientIdx, packetID);
    break;
  }
  case MQTT_PINGREQ:
    MQTTS_SendPingresp(clientIdx);
    break;
  case MQTT_DISCONNECT:
    addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, "MQTTS: client '%s' disconnected",
              client->clientID);
    close(client->socket);
    client->socket = -1;
    client->bConnected = 0;
    client->numSubs = 0;
    break;
  default:
    addLogAdv(LOG_DEBUG, LOG_FEATURE_GENERAL,
              "MQTTS: unknown packet type %d from '%s'", pktType,
              client->clientID);
    break;
  }
}

static void MQTTS_CreateListenSocket() {
  struct sockaddr_in addr;

  g_listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (g_listenSocket < 0) {
    g_listenSocket = -1;
    addLogAdv(LOG_ERROR, LOG_FEATURE_GENERAL,
              "MQTTS: failed to create listen socket");
    return;
  }

  int flag = 1;
  setsockopt(g_listenSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&flag,
             sizeof(flag));

  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(MQTT_SERVER_PORT);

  if (bind(g_listenSocket, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    addLogAdv(LOG_ERROR, LOG_FEATURE_GENERAL, "MQTTS: bind failed on port %d",
              MQTT_SERVER_PORT);
    close(g_listenSocket);
    g_listenSocket = -1;
    return;
  }

  if (listen(g_listenSocket, MQTT_MAX_CLIENTS) < 0) {
    addLogAdv(LOG_ERROR, LOG_FEATURE_GENERAL, "MQTTS: listen failed");
    close(g_listenSocket);
    g_listenSocket = -1;
    return;
  }

  lwip_fcntl(g_listenSocket, F_SETFL, O_NONBLOCK);

  addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, "MQTTS: listening on port %d",
            MQTT_SERVER_PORT);
}
static commandResult_t Cmd_MQTTServer_Publish(const void *context,
                                              const char *cmd, const char *args,
                                              int cmdFlags) {
  const char *topic;
  const char *payload;
  int topicLen, payloadLen;

  Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES);
  // following check must be done after 'Tokenizer_TokenizeString',
  // so we know arguments count in Tokenizer. 'cmd' argument is
  // only for warning display
  if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 2)) {
    return CMD_RES_NOT_ENOUGH_ARGUMENTS;
  }

  topic = Tokenizer_GetArg(0);
  payload = Tokenizer_GetArg(1);
  topicLen = strlen(topic);
  payloadLen = strlen(payload);

  addLogAdv(LOG_DEBUG, LOG_FEATURE_GENERAL, "MQTTS: ms_publish '%s' '%s'",
            topic, payload);
  MQTTS_ForwardPublish((const byte *)topic, topicLen, (const byte *)payload,
                       payloadLen, -1);

  return CMD_RES_OK;
}

void DRV_MQTTServer_AppendInformationToHTTPIndexPage(http_request_t *request,
                                                     int bPreState) {
  int i, s, cnt;
  if (bPreState)
    return;
  hprintf255(request, "<h5>MQTT Server (port %d, uptime %ds, %d fwd)</h5>",
             MQTT_SERVER_PORT, g_mqttServer_secondsElapsed,
             g_totalPublishForwarded);
  cnt = 0;
  for (i = 0; i < MQTT_MAX_CLIENTS; i++) {
    if (g_clients[i].socket < 0)
      continue;
    cnt++;
    hprintf255(request, "<b>#%d %s %s %s In %d/%d Out %d/%d</b><br>", i,
               g_clients[i].clientID[0] ? g_clients[i].clientID : "(no id)",
               g_clients[i].bConnected ? "Connected" : "TCP",
               g_clients[i].ipAddr, g_clients[i].bytesRecv,
               g_clients[i].packetsRecv, g_clients[i].bytesSent,
               g_clients[i].packetsSent);
    if (g_clients[i].numSubs > 0) {
      hprintf255(request, "&nbsp;&nbsp;Subs: ");
      for (s = 0; s < g_clients[i].numSubs; s++) {
        if (s > 0)
          hprintf255(request, ", ");
        hprintf255(request, "%s", g_clients[i].subs[s].topic);
      }
      hprintf255(request, "<br>");
    }
  }
  if (cnt == 0) {
    hprintf255(request, "No clients connected.<br>");
  }
}

void DRV_MQTTServer_Init() {
  int i;
  strcpy(g_password,
         "ma1oovoo0pooTie7koa8Eiwae9vohth1vool8ekaej8Voohi7beif5uMuph9Diex");
  strcpy(g_user, "homeassistant");

  for (i = 0; i < MQTT_MAX_CLIENTS; i++) {
    g_clients[i].socket = -1;
    g_clients[i].bConnected = 0;
    g_clients[i].numSubs = 0;
    g_clients[i].clientID[0] = 0;
    g_clients[i].bytesRecv = 0;
    g_clients[i].bytesSent = 0;
    g_clients[i].packetsRecv = 0;
    g_clients[i].packetsSent = 0;
  }

  g_mqttServer_secondsElapsed = 0;
  g_totalPublishForwarded = 0;
  MQTTS_CreateListenSocket();
  // cmddetail:{"name":"ms_publish","args":"[Topic][Payload]",
  // cmddetail:"descr":"Publish a message via the built-in MQTT server to all
  // subscribed clients.",
  // cmddetail:"fn":"Cmd_MQTTServer_Publish","file":"driver/drv_mqttServer.c","requires":"MQTTSERVER",
  // cmddetail:"examples":""}
  CMD_RegisterCommand("ms_publish", Cmd_MQTTServer_Publish, NULL);
  addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL,
            "DRV_MQTTServer_Init: MQTT Server driver started");
}

void DRV_MQTTServer_RunQuickTick() {
  int i;
  if (g_listenSocket < 0)
    return;

  // Accept new connections (non-blocking)
  struct sockaddr_in clientAddr;
  socklen_t addrLen = sizeof(clientAddr);
  int newSock =
      accept(g_listenSocket, (struct sockaddr *)&clientAddr, &addrLen);
  if (newSock >= 0) {
    lwip_fcntl(newSock, F_SETFL, O_NONBLOCK);
    // Find free client slot
    int found = 0;
    for (i = 0; i < MQTT_MAX_CLIENTS; i++) {
      if (g_clients[i].socket < 0) {
        g_clients[i].socket = newSock;
        g_clients[i].bConnected = 0;
        g_clients[i].numSubs = 0;
        g_clients[i].clientID[0] = 0;
        g_clients[i].bytesRecv = 0;
        g_clients[i].bytesSent = 0;
        g_clients[i].packetsRecv = 0;
        g_clients[i].packetsSent = 0;
        strncpy(g_clients[i].ipAddr, inet_ntoa(clientAddr.sin_addr),
                sizeof(g_clients[i].ipAddr) - 1);
        g_clients[i].ipAddr[sizeof(g_clients[i].ipAddr) - 1] = 0;
        addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL,
                  "MQTTS: new TCP connection from %s", g_clients[i].ipAddr);
        found = 1;
        break;
      }
    }
    if (!found) {
      addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL,
                "MQTTS: max clients reached, rejecting");
      close(newSock);
    }
  }

  // Poll each connected client for data
  for (i = 0; i < MQTT_MAX_CLIENTS; i++) {
    if (g_clients[i].socket < 0)
      continue;
    int nbytes =
        recv(g_clients[i].socket, (char *)g_recvBuf, MQTT_RECV_BUF_SIZE, 0);
    if (nbytes > 0) {
      g_clients[i].bytesRecv += nbytes;
      // Process all complete packets in buffer
      int offset = 0;
      while (offset < nbytes) {
        if (nbytes - offset < 2)
          break;
        int lenBytes = 0;
        int remainLen = MQTTS_DecodeRemainingLength(
            g_recvBuf + offset + 1, nbytes - offset - 1, &lenBytes);
        if (remainLen < 0)
          break;
        int pktTotalLen = 1 + lenBytes + remainLen;
        if (offset + pktTotalLen > nbytes)
          break;
        MQTTS_HandlePacket(i, g_recvBuf + offset, pktTotalLen);
        offset += pktTotalLen;
      }
    } else if (nbytes == 0) {
      // Connection closed by peer
      addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, "MQTTS: client '%s' TCP closed",
                g_clients[i].clientID);
      close(g_clients[i].socket);
      g_clients[i].socket = -1;
      g_clients[i].bConnected = 0;
      g_clients[i].numSubs = 0;
    }
    // nbytes < 0 means EWOULDBLOCK, ignore
  }
}

void DRV_MQTTServer_RunEverySecond() { g_mqttServer_secondsElapsed++; }

void DRV_MQTTServer_Stop() {
  int i;
  for (i = 0; i < MQTT_MAX_CLIENTS; i++) {
    if (g_clients[i].socket >= 0) {
      close(g_clients[i].socket);
      g_clients[i].socket = -1;
      g_clients[i].bConnected = 0;
      g_clients[i].numSubs = 0;
    }
  }
  if (g_listenSocket >= 0) {
    close(g_listenSocket);
    g_listenSocket = -1;
  }
  addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL,
            "DRV_MQTTServer_Stop: MQTT Server driver stopped");
}

#endif // ENABLE_DRIVER_MQTTSERVER