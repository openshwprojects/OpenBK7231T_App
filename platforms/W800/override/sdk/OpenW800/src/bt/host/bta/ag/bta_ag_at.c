/******************************************************************************
 *
 *  Copyright (C) 2004-2012 Broadcom Corporation
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

/******************************************************************************
 *
 *  BTA AG AT command interpreter.
 *
 ******************************************************************************/

#include <string.h>
#include "bt_target.h"
#if defined(BTA_HFP_HSP_AG_INCLUDED) && (BTA_HFP_HSP_AG_INCLUDED == TRUE)
#include "bt_common.h"
#include "bta_ag_at.h"
#include "utl.h"
#include "gki.h"

/*****************************************************************************
**  Constants
*****************************************************************************/

/******************************************************************************
**
** Function         bta_ag_at_init
**
** Description      Initialize the AT command parser control block.
**
**
** Returns          void
**
******************************************************************************/
void bta_ag_at_init(tBTA_AG_AT_CB *p_cb)
{
    p_cb->p_cmd_buf = NULL;
    p_cb->cmd_pos = 0;
}

/******************************************************************************
**
** Function         bta_ag_at_reinit
**
** Description      Re-initialize the AT command parser control block.  This
**                  function resets the AT command parser state and frees
**                  any GKI buffer.
**
**
** Returns          void
**
******************************************************************************/
void bta_ag_at_reinit(tBTA_AG_AT_CB *p_cb)
{
    GKI_free_and_reset_buf((void **)&p_cb->p_cmd_buf);
    p_cb->cmd_pos = 0;
}

/******************************************************************************
**
** Function         bta_ag_process_at
**
** Description      Parse AT commands.  This function will take the input
**                  character string and parse it for AT commands according to
**                  the AT command table passed in the control block.
**
**
** Returns          void
**
******************************************************************************/
void bta_ag_process_at(tBTA_AG_AT_CB *p_cb)
{
    uint16_t      idx;
    uint8_t       arg_type;
    char        *p_arg;
    int16_t       int_arg = 0;

    /* loop through at command table looking for match */
    for(idx = 0; p_cb->p_at_tbl[idx].p_cmd[0] != 0; idx++) {
        if(!utl_strucmp(p_cb->p_at_tbl[idx].p_cmd, p_cb->p_cmd_buf)) {
            break;
        }
    }

    /* if there is a match; verify argument type */
    if(p_cb->p_at_tbl[idx].p_cmd[0] != 0) {
        /* start of argument is p + strlen matching command */
        p_arg = p_cb->p_cmd_buf + strlen(p_cb->p_at_tbl[idx].p_cmd);

        /* if no argument */
        if(p_arg[0] == 0) {
            arg_type = BTA_AG_AT_NONE;
        }
        /* else if arg is '?' and it is last character */
        else if(p_arg[0] == '?' && p_arg[1] == 0) {
            /* we have a read */
            arg_type = BTA_AG_AT_READ;
        }
        /* else if arg is '=' */
        else if(p_arg[0] == '=' && p_arg[1] != 0) {
            if(p_arg[1] == '?' && p_arg[2] == 0) {
                /* we have a test */
                arg_type = BTA_AG_AT_TEST;
            } else {
                /* we have a set */
                arg_type = BTA_AG_AT_SET;
                /* skip past '=' */
                p_arg++;
            }
        } else
            /* else it is freeform argument */
        {
            arg_type = BTA_AG_AT_FREE;
        }

        /* if arguments match command capabilities */
        if((arg_type & p_cb->p_at_tbl[idx].arg_type) != 0) {
            /* if it's a set integer check max, min range */
            if(arg_type == BTA_AG_AT_SET &&
                    p_cb->p_at_tbl[idx].fmt == BTA_AG_AT_INT) {
                int_arg = utl_str2int(p_arg);

                if(int_arg < (int16_t) p_cb->p_at_tbl[idx].min ||
                        int_arg > (int16_t) p_cb->p_at_tbl[idx].max) {
                    /* arg out of range; error */
                    (*p_cb->p_err_cback)(p_cb->p_user, FALSE, NULL);
                } else {
                    (*p_cb->p_cmd_cback)(p_cb->p_user, idx, arg_type, p_arg, int_arg);
                }
            } else {
                (*p_cb->p_cmd_cback)(p_cb->p_user, idx, arg_type, p_arg, int_arg);
            }
        }
        /* else error */
        else {
            (*p_cb->p_err_cback)(p_cb->p_user, FALSE, NULL);
        }
    }
    /* else no match call error callback */
    else {
        (*p_cb->p_err_cback)(p_cb->p_user, TRUE, p_cb->p_cmd_buf);
    }
}

/******************************************************************************
**
** Function         bta_ag_at_parse
**
** Description      Parse AT commands.  This function will take the input
**                  character string and parse it for AT commands according to
**                  the AT command table passed in the control block.
**
**
** Returns          void
**
******************************************************************************/
void bta_ag_at_parse(tBTA_AG_AT_CB *p_cb, char *p_buf, uint16_t len)
{
    int i = 0;
    char *p_save;

    if(p_cb->p_cmd_buf == NULL) {
        p_cb->p_cmd_buf = (char *)GKI_getbuf(p_cb->cmd_max_len);
        p_cb->cmd_pos = 0;
    }

    for(i = 0; i < len;) {
        while(p_cb->cmd_pos < p_cb->cmd_max_len - 1 && i < len) {
            /* Skip null characters between AT commands. */
            if((p_cb->cmd_pos == 0) && (p_buf[i] == 0)) {
                i++;
                continue;
            }

            p_cb->p_cmd_buf[p_cb->cmd_pos] = p_buf[i++];

            if(p_cb->p_cmd_buf[p_cb->cmd_pos] == '\r' || p_cb->p_cmd_buf[p_cb->cmd_pos] == '\n') {
                p_cb->p_cmd_buf[p_cb->cmd_pos] = 0;

                if((p_cb->cmd_pos > 2)                                      &&
                        (p_cb->p_cmd_buf[0] == 'A' || p_cb->p_cmd_buf[0] == 'a') &&
                        (p_cb->p_cmd_buf[1] == 'T' || p_cb->p_cmd_buf[1] == 't')) {
                    p_save = p_cb->p_cmd_buf;
                    p_cb->p_cmd_buf += 2;
                    bta_ag_process_at(p_cb);
                    p_cb->p_cmd_buf = p_save;
                }

                p_cb->cmd_pos = 0;
            } else if(p_cb->p_cmd_buf[p_cb->cmd_pos] == 0x1A || p_cb->p_cmd_buf[p_cb->cmd_pos] == 0x1B) {
                p_cb->p_cmd_buf[++p_cb->cmd_pos] = 0;
                (*p_cb->p_err_cback)(p_cb->p_user, TRUE, p_cb->p_cmd_buf);
                p_cb->cmd_pos = 0;
            } else {
                ++p_cb->cmd_pos;
            }
        }

        if(i < len) {
            p_cb->cmd_pos = 0;
        }
    }
}
#endif
