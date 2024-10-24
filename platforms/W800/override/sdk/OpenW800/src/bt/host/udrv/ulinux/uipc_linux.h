/******************************************************************************
 *
 *  Copyright (C) 2007-2012 Broadcom Corporation
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
#ifndef _UIPC_LINUX_H_
#define _UIPC_LINUX_H_

typedef int SOCKET;
#define INVALID_SOCKET  (SOCKET)(-1)
#define SOCKET_ERROR            (-1)

/* tcp/ip socket configuration */
typedef struct {
    char            *p_address;
    unsigned int    port;
} tUIPC_LINUX_CFG_TCP ;


#ifdef __cplusplus
extern "C" {
#endif

/* Socket configuration for GLGPS interface */
extern tUIPC_LINUX_CFG_TCP uipc_linux_cfg_glgps;

#ifdef __cplusplus
}
#endif
#endif /* _UIPC_LINUX_H_ */
