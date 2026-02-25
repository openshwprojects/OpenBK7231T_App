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

// drv_mqttServerBerry.c
int MQTTS_Berry_OnPublish(const char *topic, const byte *payload,
                          int payloadLen);
void MQTTS_Berry_Init();
void MQTTS_Berry_Shutdown();

#define MQTT_SERVER_PORT_DEFAULT 1883
#define MQTT_RECV_BUF_SIZE 2048
#define MQTT_RECV_BUF_INITIAL 128
#define MQTT_MAX_CLIENTS 8

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
static int g_mqttServerPort = MQTT_SERVER_PORT_DEFAULT;

// single user only
static char g_password[128];
static char g_user[128];

typedef struct mqttSubscription_s {
  char *topic;
  struct mqttSubscription_s *next;
} mqttSubscription_t;

typedef struct mqttClient_s {
  int socket;
  int bConnected;
  char clientID[64];
  char ipAddr[20];
  mqttSubscription_t *subs;
  int bytesRecv;
  int bytesSent;
  int packetsRecv;
  int packetsSent;
  byte *recvBuf;
  int recvBufUsed;
  int recvBufCap;
  struct mqttClient_s *next;
} mqttClient_t;

static int g_listenSocket = -1;
static mqttClient_t *g_clientList = NULL;

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
static const byte *MQTTS_ReadString(const byte *buf, int bufLen, int *outLen) {
  if (bufLen < 2)
    return NULL;
  int slen = MQTTS_ReadUint16(buf);
  if (bufLen < 2 + slen)
    return NULL;
  *outLen = slen;
  return buf + 2;
}

// Helper: send and track bytes
static void MQTTS_SendToClient(mqttClient_t *c, const byte *data, int len) {
  int r = send(c->socket, (const char *)data, len, 0);
  if (r > 0) {
    c->bytesSent += r;
  }
}

static void MQTTS_SendConnack(mqttClient_t *c, byte returnCode) {
  byte pkt[4];
  pkt[0] = (MQTT_CONNACK << 4);
  pkt[1] = 2;
  pkt[2] = 0;
  pkt[3] = returnCode;
  MQTTS_SendToClient(c, pkt, 4);
  c->packetsSent++;
}

static void MQTTS_SendSuback(mqttClient_t *c, int packetID, int topicCount) {
  // SUBACK: fixed header + packetID + one return code per topic
  int totalLen = 2 + topicCount; // packetID(2) + N return codes
  byte pkt[2 + 2 + 16];          // header(2) + packetID(2) + up to 16 topics
  if (topicCount > 16)
    topicCount = 16;
  pkt[0] = (MQTT_SUBACK << 4);
  pkt[1] = (byte)totalLen;
  pkt[2] = (packetID >> 8) & 0xFF;
  pkt[3] = packetID & 0xFF;
  for (int i = 0; i < topicCount; i++) {
    pkt[4 + i] = 0x00; // granted QoS 0 for each
  }
  MQTTS_SendToClient(c, pkt, 4 + topicCount);
  c->packetsSent++;
}

static void MQTTS_SendUnsuback(mqttClient_t *c, int packetID) {
  byte pkt[4];
  pkt[0] = (MQTT_UNSUBACK << 4);
  pkt[1] = 2;
  pkt[2] = (packetID >> 8) & 0xFF;
  pkt[3] = packetID & 0xFF;
  MQTTS_SendToClient(c, pkt, 4);
  c->packetsSent++;
}

static void MQTTS_SendPingresp(mqttClient_t *c) {
  byte pkt[2];
  pkt[0] = (MQTT_PINGRESP << 4);
  pkt[1] = 0;
  MQTTS_SendToClient(c, pkt, 2);
  c->packetsSent++;
}

static void MQTTS_SendPuback(mqttClient_t *c, int packetID) {
  byte pkt[4];
  pkt[0] = (MQTT_PUBACK << 4);
  pkt[1] = 2;
  pkt[2] = (packetID >> 8) & 0xFF;
  pkt[3] = packetID & 0xFF;
  MQTTS_SendToClient(c, pkt, 4);
  c->packetsSent++;
}

// Free all subscription nodes
static void MQTTS_FreeSubs(mqttClient_t *c) {
  mqttSubscription_t *s = c->subs;
  while (s) {
    mqttSubscription_t *next = s->next;
    if (s->topic)
      free(s->topic);
    free(s);
    s = next;
  }
  c->subs = NULL;
}

// Unlink client from list, close socket, free subs, free client
static void MQTTS_FreeClient(mqttClient_t *c) {
  // unlink from g_clientList
  mqttClient_t **pp = &g_clientList;
  while (*pp) {
    if (*pp == c) {
      *pp = c->next;
      break;
    }
    pp = &(*pp)->next;
  }
  if (c->socket >= 0) {
    close(c->socket);
  }
  MQTTS_FreeSubs(c);
  if (c->recvBuf)
    free(c->recvBuf);
  free(c);
}

// Add a subscription to client (prepend)
static void MQTTS_AddSub(mqttClient_t *c, const char *topic) {
  mqttSubscription_t *s =
      (mqttSubscription_t *)malloc(sizeof(mqttSubscription_t));
  if (!s)
    return;
  s->topic = strdup(topic);
  s->next = c->subs;
  c->subs = s;
}

// Remove a subscription by topic
static void MQTTS_RemoveSub(mqttClient_t *c, const char *topic) {
  mqttSubscription_t **pp = &c->subs;
  while (*pp) {
    if ((*pp)->topic && !strcmp((*pp)->topic, topic)) {
      mqttSubscription_t *victim = *pp;
      *pp = victim->next;
      free(victim->topic);
      free(victim);
      return;
    }
    pp = &(*pp)->next;
  }
}

// Count clients in list
static int MQTTS_ClientCount() {
  int n = 0;
  mqttClient_t *c = g_clientList;
  while (c) {
    n++;
    c = c->next;
  }
  return n;
}

// Match topic against subscription filter with + and # wildcards
int MQTTS_TopicMatch(const char *topic, const char *filter) {
  while (*filter) {
    if (*filter == '#') {
      return 1;
    }
    if (*filter == '+') {
      while (*topic && *topic != '/')
        topic++;
      filter++;
      if (*topic == '/' && *filter == '/') {
        topic++;
        filter++;
        continue;
      }
      return (*topic == 0 && *filter == 0) ? 1 : 0;
    }
    if (*topic != *filter)
      return 0;
    topic++;
    filter++;
  }
  return (*topic == 0) ? 1 : 0;
}

// Forward a PUBLISH to all subscribed clients
void MQTTS_ForwardPublish(const byte *topicData, int topicLen,
                          const byte *payload, int payloadLen, void *sender) {
  char topicStr[128];
  int copyLen = topicLen < 127 ? topicLen : 127;
  memcpy(topicStr, topicData, copyLen);
  topicStr[copyLen] = 0;

  mqttClient_t *c;
  for (c = g_clientList; c; c = c->next) {
    if (!c->bConnected || c == sender)
      continue;
    mqttSubscription_t *s;
    for (s = c->subs; s; s = s->next) {
      if (!s->topic)
        continue;
      if (MQTTS_TopicMatch(topicStr, s->topic)) {
        // Build PUBLISH packet: fixed header + topic + payload
        byte hdr[5];
        int totalPayload = 2 + topicLen + payloadLen;
        int hdrLen = 1;
        hdr[0] = (MQTT_PUBLISH << 4); // QoS 0, no retain
        int rl = totalPayload;
        do {
          byte eb = rl % 128;
          rl /= 128;
          if (rl > 0)
            eb |= 0x80;
          hdr[hdrLen++] = eb;
        } while (rl > 0);
        MQTTS_SendToClient(c, hdr, hdrLen);
        byte topicHdr[2];
        topicHdr[0] = (topicLen >> 8) & 0xFF;
        topicHdr[1] = topicLen & 0xFF;
        MQTTS_SendToClient(c, topicHdr, 2);
        MQTTS_SendToClient(c, topicData, topicLen);
        if (payloadLen > 0) {
          MQTTS_SendToClient(c, payload, payloadLen);
        }
        c->packetsSent++;
        g_totalPublishForwarded++;
        break; // only send once per client
      }
    }
  }
}

static void MQTTS_HandlePacket(mqttClient_t *client, const byte *buf,
                               int totalLen) {
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
  client->packetsRecv++;

  switch (pktType) {
  case MQTT_CONNECT: {
    if (remainLen < 10) {
      MQTTS_SendConnack(client, 1);
      return;
    }
    int pos = 0;
    int protoNameLen;
    MQTTS_ReadString(payload + pos, remainLen - pos, &protoNameLen);
    pos += 2 + protoNameLen;
    pos++; // skip level byte
    byte connectFlags = payload[pos++];
    int hasUsername = (connectFlags >> 7) & 1;
    int hasPassword = (connectFlags >> 6) & 1;
    pos += 2; // skip keepalive

    int clientIDLen;
    const byte *clientIDData =
        MQTTS_ReadString(payload + pos, remainLen - pos, &clientIDLen);
    if (clientIDData) {
      int cLen = clientIDLen < 63 ? clientIDLen : 63;
      memcpy(client->clientID, clientIDData, cLen);
      client->clientID[cLen] = 0;
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
        MQTTS_SendConnack(client, 4);
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
          MQTTS_SendConnack(client, 5);
          return;
        }
      }
    }

    client->bConnected = 1;
    MQTTS_FreeSubs(client);
    MQTTS_SendConnack(client, 0);
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
    int pubPayloadLen = remainLen - pos;
    const byte *pubPayload = payload + pos;
    char topicStr[128];
    int cLen = topicLen < 127 ? topicLen : 127;
    memcpy(topicStr, topicData, cLen);
    topicStr[cLen] = 0;
    addLogAdv(LOG_DEBUG, LOG_FEATURE_GENERAL,
              "MQTTS: PUBLISH '%s' (%d bytes) from '%s'", topicStr,
              pubPayloadLen, client->clientID);
    MQTTS_ForwardPublish(topicData, topicLen, pubPayload, pubPayloadLen,
                         client);
#if ENABLE_OBK_BERRY
    MQTTS_Berry_OnPublish(topicStr, pubPayload, pubPayloadLen);
#endif
    if (qos == 1) {
      MQTTS_SendPuback(client, packetID);
    }
    break;
  }
  case MQTT_SUBSCRIBE: {
    if (!client->bConnected)
      return;
    int pos = 0;
    int packetID = MQTTS_ReadUint16(payload + pos);
    pos += 2;
    int topicCount = 0;
    while (pos < remainLen) {
      int topicLen;
      const byte *topicData =
          MQTTS_ReadString(payload + pos, remainLen - pos, &topicLen);
      if (!topicData)
        break;
      pos += 2 + topicLen;
      byte requestedQoS = payload[pos++];
      char tmp[128];
      int cLen = topicLen < 127 ? topicLen : 127;
      memcpy(tmp, topicData, cLen);
      tmp[cLen] = 0;
      MQTTS_AddSub(client, tmp);
      topicCount++;
      addLogAdv(LOG_DEBUG, LOG_FEATURE_GENERAL,
                "MQTTS: client '%s' subscribed to '%s'", client->clientID, tmp);
    }
    MQTTS_SendSuback(client, packetID, topicCount);
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
      int cLen = topicLen < 127 ? topicLen : 127;
      memcpy(topicStr, topicData, cLen);
      topicStr[cLen] = 0;
      MQTTS_RemoveSub(client, topicStr);
    }
    MQTTS_SendUnsuback(client, packetID);
    break;
  }
  case MQTT_PUBACK:
    // QoS 1 ack from client, nothing to do
    break;
  case MQTT_PINGREQ:
    MQTTS_SendPingresp(client);
    break;
  case MQTT_DISCONNECT:
    addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, "MQTTS: client '%s' disconnected",
              client->clientID);
    MQTTS_FreeClient(client);
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
  addr.sin_port = htons(g_mqttServerPort);

  if (bind(g_listenSocket, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    addLogAdv(LOG_ERROR, LOG_FEATURE_GENERAL, "MQTTS: bind failed on port %d",
              g_mqttServerPort);
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
            g_mqttServerPort);
}

static commandResult_t Cmd_MQTTServer_User(const void *context, const char *cmd,
                                           const char *args, int cmdFlags) {
  Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES);
  if (Tokenizer_GetArgsCount() == 0) {
    ADDLOG_INFO(LOG_FEATURE_GENERAL, "MQTTS user: %s", g_user);
  } else {
    strncpy(g_user, Tokenizer_GetArg(0), sizeof(g_user) - 1);
    g_user[sizeof(g_user) - 1] = 0;
    ADDLOG_INFO(LOG_FEATURE_GENERAL, "MQTTS user set to: %s", g_user);
  }
  return CMD_RES_OK;
}

static commandResult_t Cmd_MQTTServer_Pass(const void *context, const char *cmd,
                                           const char *args, int cmdFlags) {
  Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES);
  if (Tokenizer_GetArgsCount() == 0) {
    ADDLOG_INFO(LOG_FEATURE_GENERAL, "MQTTS pass: %s", g_password);
  } else {
    strncpy(g_password, Tokenizer_GetArg(0), sizeof(g_password) - 1);
    g_password[sizeof(g_password) - 1] = 0;
    ADDLOG_INFO(LOG_FEATURE_GENERAL, "MQTTS pass set");
  }
  return CMD_RES_OK;
}

static commandResult_t Cmd_MQTTServer_Port(const void *context, const char *cmd,
                                           const char *args, int cmdFlags) {
  Tokenizer_TokenizeString(args, 0);
  if (Tokenizer_GetArgsCount() == 0) {
    ADDLOG_INFO(LOG_FEATURE_GENERAL, "MQTTS port: %d", g_mqttServerPort);
    return CMD_RES_OK;
  }
  int newPort = Tokenizer_GetArgInteger(0);
  if (newPort < 1 || newPort > 65535) {
    ADDLOG_INFO(LOG_FEATURE_GENERAL, "MQTTS: invalid port %d", newPort);
    return CMD_RES_BAD_ARGUMENT;
  }
  g_mqttServerPort = newPort;
  if (g_listenSocket >= 0) {
    close(g_listenSocket);
    g_listenSocket = -1;
  }
  MQTTS_CreateListenSocket();
  return CMD_RES_OK;
}

static commandResult_t Cmd_MQTTServer_Publish(const void *context,
                                              const char *cmd, const char *args,
                                              int cmdFlags) {
  const char *topic;
  const char *payload;
  int topicLen, payloadLen;

  Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES);
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
                       payloadLen, NULL);

  return CMD_RES_OK;
}

void DRV_MQTTServer_AppendInformationToHTTPIndexPage(http_request_t *request,
                                                     int bPreState) {
  int cnt;
  if (bPreState)
    return;
  hprintf255(request,
             "<h5>MQTT Server (port %d, uptime %ds, %d fwd, %d devices)</h5>",
             g_mqttServerPort, g_mqttServer_secondsElapsed,
             g_totalPublishForwarded, MQTTS_ClientCount());
  cnt = 0;
  mqttClient_t *c;
  for (c = g_clientList; c; c = c->next) {
    cnt++;
    hprintf255(request, "<hr>");
    hprintf255(request,
               "<b>%s %s <a href='http://%s'>%s</a> In %d/%d Out %d/%d</b><br>",
               c->clientID[0] ? c->clientID : "(no id)",
               c->bConnected ? "Connected" : "TCP", c->ipAddr, c->ipAddr,
               c->bytesRecv, c->packetsRecv, c->bytesSent, c->packetsSent);
    if (c->subs) {
      hprintf255(request, "&nbsp;&nbsp;Subs: ");
      int first = 1;
      mqttSubscription_t *s;
      for (s = c->subs; s; s = s->next) {
        if (!first)
          hprintf255(request, ", ");
        hprintf255(request, "%s", s->topic ? s->topic : "?");
        first = 0;
      }
      hprintf255(request, "<br>");
    }
    // Extract per-device command topic from subscriptions like "cmnd/XXX/#"
    // Prefer topics containing '_' (device-specific e.g. tasmota_476739)
    // over group topics (e.g. tasmotas, bekens)
    {
      const char *bestTopic = NULL;
      int bestScore = -1;
      mqttSubscription_t *s;
      for (s = c->subs; s; s = s->next) {
        if (!s->topic || strncmp(s->topic, "cmnd/", 5) != 0)
          continue;
        const char *devStart = s->topic + 5;
        const char *slash = strchr(devStart, '/');
        if (!slash || slash == devStart)
          continue;
        int devLen = (int)(slash - devStart);
        // Score: higher = better. Prefer names with '_' (device-specific)
        int score = 1;
        if (memchr(devStart, '_', devLen))
          score += 10;
        // Deprioritize short generic group topics
        if (devLen <= 7)
          score -= 5;
        if (score > bestScore) {
          bestScore = score;
          bestTopic = devStart;
        }
      }
      char devName[64];
      devName[0] = 0;
      if (bestTopic) {
        const char *slash = strchr(bestTopic, '/');
        int devLen = (int)(slash - bestTopic);
        if (devLen > 63)
          devLen = 63;
        memcpy(devName, bestTopic, devLen);
        devName[devLen] = 0;
      } else if (c->clientID[0]) {
        strncpy(devName, c->clientID, 63);
        devName[63] = 0;
      }
      if (devName[0]) {
        hprintf255(request,
                   "<button "
                   "onclick=\"fetch('/cm?cmnd='+encodeURIComponent('ms_publish "
                   "cmnd/%s/POWER TOGGLE'))\">Send Toggle</button> ",
                   devName);
        hprintf255(request,
                   "<button "
                   "onclick=\"fetch('/cm?cmnd='+encodeURIComponent('ms_publish "
                   "cmnd/%s/POWER2 TOGGLE'))\">Send Toggle 2</button> ",
                   devName);
        hprintf255(request,
                   "<button "
                   "onclick=\"fetch('/cm?cmnd='+encodeURIComponent('ms_publish "
                   "cmnd/%s/Color FF0000'))\">Send Color Red</button> ",
                   devName);
        hprintf255(request,
                   "<button "
                   "onclick=\"fetch('/cm?cmnd='+encodeURIComponent('ms_publish "
                   "cmnd/%s/Color 00FF00'))\">Send Color Green</button>",
                   devName);
        hprintf255(request,
                   "<br><i>Development test commands, may not work on some "
                   "device types</i><br>");
      }
    }
  }
  if (cnt == 0) {
    hprintf255(request, "No clients connected.<br>");
  }
}

void DRV_MQTTServer_Init() {
  strcpy(g_password,
         "ma1oovoo0pooTie7koa8Eiwae9vohth1vool8ekaej8Voohi7beif5uMuph9Diex");
  strcpy(g_user, "homeassistant");

  g_clientList = NULL;
  g_mqttServer_secondsElapsed = 0;
  g_totalPublishForwarded = 0;
  MQTTS_CreateListenSocket();
  // cmddetail:{"name":"ms_publish","args":"[Topic][Payload]",
  // cmddetail:"descr":"Publish a message via the built-in MQTT server to all
  // subscribed clients.",
  // cmddetail:"fn":"Cmd_MQTTServer_Publish","file":"driver/drv_mqttServer.c","requires":"MQTTSERVER",
  // cmddetail:"examples":""}
  CMD_RegisterCommand("ms_publish", Cmd_MQTTServer_Publish, NULL);
  // cmddetail:{"name":"ms_user","args":"[Username]",
  // cmddetail:"descr":"Set or get the MQTT server username for client
  // authentication.",
  // cmddetail:"fn":"Cmd_MQTTServer_User","file":"driver/drv_mqttServer.c","requires":"MQTTSERVER",
  // cmddetail:"examples":""}
  CMD_RegisterCommand("ms_user", Cmd_MQTTServer_User, NULL);
  // cmddetail:{"name":"ms_pass","args":"[Password]",
  // cmddetail:"descr":"Set or get the MQTT server password for client
  // authentication.",
  // cmddetail:"fn":"Cmd_MQTTServer_Pass","file":"driver/drv_mqttServer.c","requires":"MQTTSERVER",
  // cmddetail:"examples":""}
  CMD_RegisterCommand("ms_pass", Cmd_MQTTServer_Pass, NULL);
  // cmddetail:{"name":"ms_port","args":"[PortNumber]",
  // cmddetail:"descr":"Set or get the MQTT server listen port. Setting a new
  // port closes and recreates the listener.",
  // cmddetail:"fn":"Cmd_MQTTServer_Port","file":"driver/drv_mqttServer.c","requires":"MQTTSERVER",
  // cmddetail:"examples":""}
  CMD_RegisterCommand("ms_port", Cmd_MQTTServer_Port, NULL);
  addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL,
            "DRV_MQTTServer_Init: MQTT Server driver started");
}

void DRV_MQTTServer_RunQuickTick() {
  if (g_listenSocket < 0)
    return;

  // Accept new connections (non-blocking)
  struct sockaddr_in clientAddr;
  socklen_t addrLen = sizeof(clientAddr);
  int newSock =
      accept(g_listenSocket, (struct sockaddr *)&clientAddr, &addrLen);
  if (newSock >= 0) {
    if (MQTTS_ClientCount() >= MQTT_MAX_CLIENTS) {
      addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL,
                "MQTTS: max clients reached, rejecting");
      close(newSock);
    } else {
      lwip_fcntl(newSock, F_SETFL, O_NONBLOCK);
      mqttClient_t *c = (mqttClient_t *)malloc(sizeof(mqttClient_t));
      if (!c) {
        addLogAdv(LOG_ERROR, LOG_FEATURE_GENERAL,
                  "MQTTS: malloc failed for client");
        close(newSock);
      } else {
        memset(c, 0, sizeof(mqttClient_t));
        c->socket = newSock;
        c->recvBuf = (byte *)malloc(MQTT_RECV_BUF_INITIAL);
        c->recvBufCap = c->recvBuf ? MQTT_RECV_BUF_INITIAL : 0;
        strncpy(c->ipAddr, inet_ntoa(clientAddr.sin_addr),
                sizeof(c->ipAddr) - 1);
        c->ipAddr[sizeof(c->ipAddr) - 1] = 0;
        // prepend to list
        c->next = g_clientList;
        g_clientList = c;
        addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL,
                  "MQTTS: new TCP connection from %s", c->ipAddr);
      }
    }
  }

  // Poll each connected client for data
  // Use safe iteration since HandlePacket/FreeClient can remove from list
  mqttClient_t *c = g_clientList;
  while (c) {
    mqttClient_t *next = c->next; // save next before possible free
    if (!c->recvBuf || c->recvBufCap == 0) {
      c = next;
      continue;
    }
    int space = c->recvBufCap - c->recvBufUsed;
    if (space <= 0) {
      // Buffer full with no complete packet — drop data
      c->recvBufUsed = 0;
      space = c->recvBufCap;
    }
    int nbytes =
        recv(c->socket, (char *)(c->recvBuf + c->recvBufUsed), space, 0);
    if (nbytes > 0) {
      c->bytesRecv += nbytes;
      c->recvBufUsed += nbytes;
      int total = c->recvBufUsed;
      int offset = 0;
      while (offset < total) {
        if (total - offset < 2)
          break;
        int lenBytes = 0;
        int remainLen = MQTTS_DecodeRemainingLength(
            c->recvBuf + offset + 1, total - offset - 1, &lenBytes);
        if (remainLen < 0)
          break; // incomplete remaining-length encoding
        int pktTotalLen = 1 + lenBytes + remainLen;
        if (offset + pktTotalLen > total) {
          // Need more space? Grow buffer if current capacity is too small
          if (pktTotalLen > c->recvBufCap &&
              pktTotalLen <= MQTT_RECV_BUF_SIZE) {
            int newCap = c->recvBufCap;
            while (newCap < pktTotalLen)
              newCap *= 2;
            if (newCap > MQTT_RECV_BUF_SIZE)
              newCap = MQTT_RECV_BUF_SIZE;
            byte *nb = (byte *)realloc(c->recvBuf, newCap);
            if (nb) {
              c->recvBuf = nb;
              c->recvBufCap = newCap;
            }
          }
          break; // incomplete packet, wait for more data
        }
        MQTTS_HandlePacket(c, c->recvBuf + offset, pktTotalLen);
        offset += pktTotalLen;
      }
      // Shift unconsumed bytes to front of buffer
      if (offset > 0 && offset < total) {
        memmove(c->recvBuf, c->recvBuf + offset, total - offset);
        c->recvBufUsed = total - offset;
      } else if (offset >= total) {
        c->recvBufUsed = 0;
      }
    } else if (nbytes == 0) {
      addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL, "MQTTS: client '%s' TCP closed",
                c->clientID);
      MQTTS_FreeClient(c);
    }
    // nbytes < 0 means EWOULDBLOCK, ignore
    c = next;
  }
}

void DRV_MQTTServer_RunEverySecond() { g_mqttServer_secondsElapsed++; }

void DRV_MQTTServer_Stop() {
#if ENABLE_OBK_BERRY
  MQTTS_Berry_Shutdown();
#endif
  while (g_clientList) {
    MQTTS_FreeClient(g_clientList);
  }
  if (g_listenSocket >= 0) {
    close(g_listenSocket);
    g_listenSocket = -1;
  }
  addLogAdv(LOG_INFO, LOG_FEATURE_GENERAL,
            "DRV_MQTTServer_Stop: MQTT Server driver stopped");
}

#endif // ENABLE_DRIVER_MQTTSERVER
