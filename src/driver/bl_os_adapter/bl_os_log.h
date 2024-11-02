/****************************************************************************
 * components/platform/soc/bl602/bl602_os_adapter/bl602_os_adapter/include/bl_os_adapter/bl_os_log.h
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

#ifndef _BL_OS_LOG_H_
#define _BL_OS_LOG_H_

#include <bl_os_adapter/bl_os_adapter.h>

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************************
 * Definition
 ****************************************************************************/

typedef enum _bl_os_log_leve
{
    LOG_LEVEL_ALL = 0,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_ASSERT,
    LOG_LEVEL_NEVER,
} bl_os_log_level_t;

#define bl_os_log_printf g_bl_ops_funcs._log_write

#define bl_os_log_debug(M, ...) \
    bl_os_log_printf(LOG_LEVEL_DEBUG, NULL, __FILENAME__, __LINE__, M, ##__VA_ARGS__);

#define bl_os_log_info(M, ...) \
    bl_os_log_printf(LOG_LEVEL_INFO, NULL, __FILENAME__, __LINE__, M, ##__VA_ARGS__);

#define bl_os_log_warn(M, ...) \
    bl_os_log_printf(LOG_LEVEL_WARN, NULL, __FILENAME__, __LINE__, M, ##__VA_ARGS__);

#define bl_os_log_error(M, ...) \
    bl_os_log_printf(LOG_LEVEL_ERROR, NULL, __FILENAME__, __LINE__, M, ##__VA_ARGS__);

/****************************************************************************
 * Private Types
 ****************************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* _BL_OS_LOG_H_ */