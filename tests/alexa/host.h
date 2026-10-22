#ifndef ALEXA_HOST_H
#define ALEXA_HOST_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdbool.h>
#include <stdarg.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "src/httpserver/new_http.h"
#include "src/jsmn/jsmn_h.h"
#define CHANNEL_MAX 64
#define ChType_Toggle 1
#ifndef ENABLE_LED_BASIC
#define ENABLE_LED_BASIC 0
#endif
#define LOG_INFO 0
#define LOG_ALL 0
#define LOG_EXTRADEBUG 0
#define LOG_FEATURE_HTTP 0
#define addLogAdv(...) ((void)0)
extern int running, values[CHANNEL_MAX], outputs[CHANNEL_MAX];
bool DRV_IsRunning(const char *name);
const char *HAL_GetMyIPString(void);
void WiFI_GetMacAddress(char *mac);
const char *CFG_GetDeviceName(void);
bool CHANNEL_HasLabel(int ch);
const char *CHANNEL_GetLabel(int ch);
int CHANNEL_Get(int ch);
int CHANNEL_GetType(int ch);
int h_isChannelRelay(int ch);
void CHANNEL_Set(int ch, int value, int flags);
void DRV_HUE_Send_Advert_To(struct sockaddr_in *addr);
void DRV_WEMO_Send_Advert_To(int mode, struct sockaddr_in *addr);
void DRV_SSDP_Send_Advert_To(struct sockaddr_in *addr);
void DRV_SSDP_SendReply(struct sockaddr_in *addr, const char *data);
int HTTP_RegisterCallback(const char *url, int method, int (*fn)(http_request_t*), int auth);
int LED_IsLEDRunning(void);
int LED_GetEnableAll(void);
void CMD_ExecuteCommand(const char *cmd, int flags);
#endif
