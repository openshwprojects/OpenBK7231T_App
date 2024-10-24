/*
*
* Licensed to the Apache Software Foundation (ASF) under one
* or more contributor license agreements.  See the NOTICE file
* distributed with this work for additional information
* regarding copyright ownership.  The ASF licenses this file
* to you under the Apache License, Version 2.0 (the
* "License"); you may not use this file except in compliance
* with the License.  You may obtain a copy of the License at
*
*  http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing,
* software distributed under the License is distributed on an
* "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
* KIND, either express or implied.  See the License for the
* specific language governing permissions and limitations
* under the License.
*/


#include "syscfg/syscfg.h"

#if MYNEWT_VAL(BLE_STORE_CONFIG_PERSIST)

#include <string.h>
#include "sysinit/sysinit.h"
#include "host/ble_hs.h"
#include "nimble/nimble_port.h"
#include "store/config/ble_store_config.h"
#include "ble_store_config_priv.h"
#include "store/config/wm_bt_storage.h"

#define NIMBLE_NVS_STR_NAME_MAX_LEN              16


typedef struct ble_store_value_noaddr_cccd {
    uint16_t chr_val_handle;
    uint16_t flags;
    unsigned value_changed: 1;
} ble_store_value_t;


typedef struct cccd_nvram {
    ble_addr_t addr;
    uint8_t offset;
    ble_store_value_t value[MYNEWT_VAL(BLE_STORE_MAX_CCCDS)];
} cccd_nvram_t;


/*****************************************************************************
 * $ NVS                                                                     *
 *****************************************************************************/

/* Gets the database in RAM filled up with keys stored in NVS. The sequence of
 * the keys in database may get lost.
 */
static void
ble_hs_log_flat_buf_tmp(const void *data, int len)
{
    const uint8_t *u8ptr;
    int i;
    u8ptr = data;

    for(i = 0; i < len; i++) {
        BLE_HS_LOG(ERROR, "0x%02x ", u8ptr[i]);
    }
}


static void
ble_store_nvram_print_value_sec(const struct ble_store_value_sec *sec)
{
    BLE_HS_LOG(ERROR, "addr=");
    ble_hs_log_flat_buf_tmp(sec->peer_addr.val, 6);
    BLE_HS_LOG(ERROR, " ");
    BLE_HS_LOG(ERROR, "peer_addr_type=%d ", sec->peer_addr.type);

    if(sec->ltk_present) {
        BLE_HS_LOG(ERROR, "ediv=%u rand=%llu authenticated=%d ltk=",
                   sec->ediv, sec->rand_num, sec->authenticated);
        ble_hs_log_flat_buf_tmp(sec->ltk, 16);
        BLE_HS_LOG(ERROR, " ");
    }

    if(sec->irk_present) {
        BLE_HS_LOG(ERROR, "irk=");
        ble_hs_log_flat_buf_tmp(sec->irk, 16);
        BLE_HS_LOG(ERROR, " ");
    }

    if(sec->csrk_present) {
        BLE_HS_LOG(ERROR, "csrk=");
        ble_hs_log_flat_buf_tmp(sec->csrk, 16);
        BLE_HS_LOG(ERROR, " ");
    }

    BLE_HS_LOG(ERROR, "\n");
}
static void
ble_store_nvram_print_value_cccd(const struct ble_store_value_cccd *sec)
{
    BLE_HS_LOG(ERROR, "addr=");
    ble_hs_log_flat_buf_tmp(sec->peer_addr.val, 6);
    BLE_HS_LOG(ERROR, " ");
    BLE_HS_LOG(ERROR, "peer_addr_type=%d ", sec->peer_addr.type);
    BLE_HS_LOG(ERROR, "chr_val_handle=%d(0x%04x) ", sec->chr_val_handle, sec->chr_val_handle);
    BLE_HS_LOG(ERROR, "flags=%d(0x%04x) ", sec->flags, sec->flags);
    BLE_HS_LOG(ERROR, "value_changed=%d(0x%04x) ", sec->value_changed, sec->value_changed);
    BLE_HS_LOG(ERROR, "\n");
}

static int
ble_nvs_restore_sec_keys()
{
    int i = 0;
    int restore_count = 0;
    uint8_t cccd_info[sizeof(ble_store_value_t)*MYNEWT_VAL(BLE_STORE_MAX_CCCDS)];
    struct ble_store_value_sec peer_sec;
    struct ble_store_value_sec our_sec;
    struct ble_store_value_cccd cccd;
    ble_addr_t addr;
    uint32_t nv_tag_valid;
    uint32_t cccd_count;
#define OUR_SEC_VALID_BIT_MASK    (0x01<<0)
#define PEER_SEC_VALID_BIT_MASK   (0x01<<1)
#define CCCD_VALID_BITS_MASK      (0xFFFFFFFC)

    for(i = 0; i < MYNEWT_VAL(BLE_STORE_MAX_BONDS); i++) {
        nv_tag_valid = btif_config_get_sec_cccd_item(i, &addr, &our_sec, sizeof(our_sec), &peer_sec,
                       sizeof(peer_sec), &cccd_info[0], sizeof(cccd_info));

        if(nv_tag_valid) {
            if(nv_tag_valid & OUR_SEC_VALID_BIT_MASK) {
                BLE_HS_LOG(ERROR, "load our  sec; ");
                ble_store_nvram_print_value_sec(&our_sec);
                ble_store_config_our_secs[ble_store_config_num_our_secs] = our_sec;
                ble_store_config_num_our_secs++;
                restore_count++;
            }

            if(nv_tag_valid & PEER_SEC_VALID_BIT_MASK) {
                BLE_HS_LOG(ERROR, "load peer sec; ");
                ble_store_nvram_print_value_sec(&peer_sec);
                ble_store_config_peer_secs[ble_store_config_num_peer_secs] = peer_sec;
                ble_store_config_num_peer_secs++;
                restore_count++;
            }

            if(nv_tag_valid & CCCD_VALID_BITS_MASK) {
                int j = 0;
                ble_store_value_t *ptr_value = (ble_store_value_t *)&cccd_info[0];
                cccd_count = nv_tag_valid >> 2;
                //nvdump_hexstring("load raw cccd info", cccd_info, sizeof(ble_store_value_t)*MYNEWT_VAL(BLE_STORE_MAX_CCCDS));
                assert(cccd_count <= 6);

                for(j = 0; j < cccd_count; j++) {
                    BLE_HS_LOG(ERROR, "load our cccd; ");
                    memcpy(&cccd.peer_addr, &addr, sizeof(ble_addr_t));
                    cccd.chr_val_handle = ptr_value->chr_val_handle;
                    cccd.flags = ptr_value->flags;
                    cccd.value_changed = ptr_value->value_changed;
                    ble_store_nvram_print_value_cccd(&cccd);
                    ble_store_config_cccds[ble_store_config_num_cccds] = cccd;
                    ble_store_config_num_cccds++;
                    ptr_value++;
                }

                restore_count++;
            }
        }
    }

    return restore_count;
}

static cccd_nvram_t cccd_nvram_array[MYNEWT_VAL(BLE_STORE_MAX_BONDS)];

int ble_store_config_persist_cccds(bool flush)
{
    int i = 0, j = 0, rc = 0;
    int nv_idx = 0;
    int prefer_index = 0;
    uint8_t *ptr_byte = NULL;
    uint8_t found = 0;

    for(i = 0; i < ble_store_config_num_cccds; i++) {
        //BLE_HS_LOG(ERROR, "save our cccd; ");
        //ble_store_nvram_print_value_cccd(&ble_store_config_cccds[i]);
        cccd_nvram_array[i].offset = 0;
    }

    /*prepare the nvram information*/
    for(i = 0; i < ble_store_config_num_cccds; i++) {
        ptr_byte = (uint8_t *)&ble_store_config_cccds[i];
        ptr_byte += 8; //value offset; dword alignment
        found = 0;

        for(j = 0; j < prefer_index; j++) {
            rc = ble_addr_cmp(&ble_store_config_cccds[i].peer_addr, &cccd_nvram_array[j].addr);

            if(!rc) {
                memcpy(&cccd_nvram_array[j].value[cccd_nvram_array[j].offset], ptr_byte, sizeof(ble_store_value_t));
                cccd_nvram_array[j].offset++;
                found = 1;
                break;
            }
        }

        if(found) { continue; }

        if(j == prefer_index) {
            memcpy(&cccd_nvram_array[j].addr, &ble_store_config_cccds[i].peer_addr, sizeof(ble_addr_t));
            memcpy(&cccd_nvram_array[j].value[cccd_nvram_array[j].offset], ptr_byte, sizeof(ble_store_value_t));
            cccd_nvram_array[j].offset++;
            prefer_index++;
        }
    }

    BLE_HS_LOG(DEBUG, "prefer_index=%d sizeof(ble_store_value_t)=%d\r\n", prefer_index,
               sizeof(ble_store_value_t));
    i = 0;

    for(i = 0; i < prefer_index; i++) {
        nv_idx  = btif_config_get_sec_index(&cccd_nvram_array[i].addr, &found);

        if(nv_idx < 0) {
            BLE_HS_LOG(DEBUG, "CCCD Full, impossible\r\n");
            return -1;
        }
        if(found) nv_idx = i;

        ptr_byte = (uint8_t *)&cccd_nvram_array[i].value[0];
        //nvdump_hexstring("write raw cccd info", ptr_byte, sizeof(ble_store_value_t)*MYNEWT_VAL(BLE_STORE_MAX_CCCDS));
        btif_config_store_cccd(nv_idx, &cccd_nvram_array[i].addr, cccd_nvram_array[i].offset, ptr_byte,
                               sizeof(ble_store_value_t)*MYNEWT_VAL(BLE_STORE_MAX_CCCDS));
    }

    for(; i < MYNEWT_VAL(BLE_STORE_MAX_BONDS); i++) {
        btif_config_delete_cccd(i);
    }

    if(flush)ble_store_config_persist_flush();
    return 0;
}

int ble_store_config_persist_peer_secs(bool flush)
{
    int i = 0;
    int nv_idx = 0;
    uint8_t found = 0;

    for(i = 0; i < ble_store_config_num_peer_secs; i++) {
        nv_idx  = btif_config_get_sec_index(&ble_store_config_peer_secs[i].peer_addr, &found);

        if(nv_idx < 0) {
            BLE_HS_LOG(ERROR, "PEER SEC Full, impossible[i=%d][%d]\r\n",i, ble_store_config_num_peer_secs);
            return -1;
        }
        if(found) nv_idx = i;

        //printf("ble_store_config_persist_peer_secs,i=%d,found =%d, nv_idx=%d, addr=%s\r\n", i, found, nv_idx, bt_hex(ble_store_config_peer_secs[i].peer_addr.val, 6));
        btif_config_store_peer_sec(nv_idx, &ble_store_config_peer_secs[i].peer_addr,
                                   (void *)&ble_store_config_peer_secs[i], sizeof(ble_store_config_peer_secs[i]));
    }

    for(; i < MYNEWT_VAL(BLE_STORE_MAX_BONDS); i++) {
        btif_config_delete_peer_sec(i);
    }

    if(flush)ble_store_config_persist_flush();
    return 0;
}

int ble_store_config_persist_our_secs(bool flush)
{
    int i = 0;
    int nv_idx = 0;
    uint8_t found = 0;

    for(i = 0; i < ble_store_config_num_our_secs; i++) {
        nv_idx  = btif_config_get_sec_index((void *)&ble_store_config_our_secs[i].peer_addr, &found);

        if(nv_idx < 0) {
            BLE_HS_LOG(ERROR, "OUR SEC Full, impossible[i=%d][%d]\r\n",i, ble_store_config_num_our_secs);
            return -1;
        }
        if(found) nv_idx = i;
        
        //printf("ble_store_config_persist_our_secs, i=%d,found =%d, nv_idx=%d,addr=%s\r\n",i, found, nv_idx, bt_hex(ble_store_config_our_secs[i].peer_addr.val,6));
        btif_config_store_our_sec(nv_idx, &ble_store_config_our_secs[i].peer_addr,
                                  (void *)&ble_store_config_our_secs[i], sizeof(ble_store_config_our_secs[i]));
    }

    for(; i < MYNEWT_VAL(BLE_STORE_MAX_BONDS); i++) {
        btif_config_delete_our_sec(i);
    }

    if(flush)ble_store_config_persist_flush();
    return 0;
}
void ble_store_config_persist_flush()
{
    //printf(">>>>>>>>>>>>>>>>>flush flash...\r\n");
    btif_config_flush(1);

}
int ble_store_config_delete_all()
{
    btif_config_delete_all();
}

void ble_store_config_conf_init()
{
    memset(cccd_nvram_array, 0, sizeof(cccd_nvram_t)*MYNEWT_VAL(BLE_STORE_MAX_BONDS));
    ble_nvs_restore_sec_keys();
    return;
}
void ble_store_config_conf_deinit()
{
}

/***************************************************************************************/
#endif /* MYNEWT_VAL(BLE_STORE_CONFIG_PERSIST) */
