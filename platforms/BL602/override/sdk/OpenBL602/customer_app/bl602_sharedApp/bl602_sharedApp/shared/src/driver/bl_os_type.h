/****************************************************************************
 * components/platform/soc/bl602/bl602_os_adapter/bl602_os_adapter/include/bl_os_adapter/bl_os_type.h
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


#ifndef _BL_OS_TYPE_H_
#define _BL_OS_TYPE_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************************
 * Definition
 ****************************************************************************/

typedef void* BL_Timer_t;
typedef void* BL_TaskHandle_t;
typedef void* BL_Sem_t;
typedef void* BL_Mutex_t;
typedef void* BL_MessageQueue_t;
typedef void* BL_EventGroup_t;
typedef void* BL_TimeOut_t;
typedef uint32_t BL_TickType_t;

#ifdef __cplusplus
}
#endif

#endif /* _BL_OS_TYPE_H_ */
