/******************************************************************************
 *
 *  Copyright (C) 2001-2012 Broadcom Corporation
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

#define LOG_TAG "bt_bte"
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
extern void hci_dbg_msg(const char *format, ...);

void LogMsg(uint32_t level, const char *fmt_str, ...)
{
    char buffer[512] = {0};

    if(1) {
        va_list ap;
        va_start(ap, fmt_str);
        vsprintf(&buffer[0], fmt_str, ap);
        va_end(ap);
        hci_dbg_msg("[WM]:%s\r\n", buffer);
    } else {
        return;
    }
}
