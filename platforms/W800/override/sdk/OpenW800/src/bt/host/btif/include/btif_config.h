/******************************************************************************
 *
 *  Copyright (C) 2009-2012 Broadcom Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/

/*******************************************************************************
 *
 *  Filename:      btif_config.h
 *
 *  Description:   Bluetooth configuration Interface
 *
 *******************************************************************************/

#ifndef BTIF_CONFIG_H
#define BTIF_CONFIG_H

#ifdef __cplusplus
#include <stdint.h>
extern "C" {
#endif

/*******************************************************************************
** Constants & Macros
********************************************************************************/

#define BTIF_CFG_TYPE_INVALID   0
#define BTIF_CFG_TYPE_STR       1
#define BTIF_CFG_TYPE_INT      (1 << 1)
#define BTIF_CFG_TYPE_BIN      (1 << 2)
#define BTIF_CFG_TYPE_VOLATILE (1 << 15)


/*******************************************************************************
**  Functions
********************************************************************************/

int btif_config_init();

int btif_config_exist(const char *section, const char *key, const char *name);
int btif_config_get_int(const char *section, const char *key, const char *name, int *value);
int btif_config_set_int(const char *section, const char *key, const char *name, int value);
int btif_config_get_str(const char *section, const char *key, const char *name, char *value,
                        int *bytes);
int btif_config_set_str(const char *section, const char *key, const char *name, const char *value);

int btif_config_get(const char *section, const char *key, const char *name, char *value, int *bytes,
                    int *type);
int btif_config_set(const char *section, const char *key, const char *name, const char  *value,
                    int bytes, int type);

int btif_config_remove(const char *section, const char *key, const char *name);
int btif_config_filter_remove(const char *section, const char *filter[], int filter_count,
                              int max_allowed);

short btif_config_next_key(short current_key_pos, const char *section, char *key_name,
                           int *key_name_bytes);
short btif_config_next_value(short pos, const char *section, const char *key, char *value_name,
                             int *value_name_bytes);

typedef void (*btif_config_enum_callback)(void *user_data, const char *section, const char *key,
        const char *name,
        const char  *value, int bytes, int type);
int btif_config_enum(btif_config_enum_callback cb, void *user_data);

int btif_config_save();
void btif_config_flush(int force);

#ifdef __cplusplus
}
#endif

#endif
