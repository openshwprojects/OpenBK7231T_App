#include <stdio.h>
#include <stdlib.h>
#include "../new_cfg.h"


#define MAX_DZ_INDEX 50


// typedef struct Domoticz_s {
// 	int idx[CHANNEL_MAX];
// 	int update_timer;
// 	int nvalue;
// } Domoticz_t;

// extern Domoticz_t g_dz;

int Dz_GetNumberOfChannels();
int Dz_GetChannelFromIndex(int idx);
void send_domoticz_message(uint16_t ch, int nvalue, char* svalue, int Battery, int RSSI);
int parse_json(char* json_str, char* dtype, int* nvalue_ptr);
void Dz_PublishAll();
void Dz_PublishEnergy();