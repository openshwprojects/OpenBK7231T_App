
/*****************************************************************************
**
**  Name:           wm_bt_storage.c
**
**  Description:    This file contains the  functions for mesh parameters storage
**
*****************************************************************************/
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#include "wm_bt_config.h"

#if (WM_MESH_INCLUDED == CFG_ON)

#include "mesh/mesh.h"
#include "mesh/glue.h"
#include "wm_flash_map.h"
#include "wm_internal_flash.h"
#include "wm_bt_def.h"
#include "wm_bt_util.h"
#include "syscfg/syscfg.h"
#include "host/ble_hs.h"
#include "wm_ble_mesh_store.h"

static uint32_t mesh_store_ptr_start = 0;
static uint32_t mesh_store_ptr_end = 0;
volatile static uint32_t mesh_store_ptr_offset;
volatile static uint32_t mesh_store_rd_ptr_offset;

static uint8_t mesh_store_area_valid;

/*All mesh parameters sotre in nvram with the format LTV
*
*Format:  Len|key'\0'|v|value
*
*  9'I''V''\0'13536373333 Means:  key=IV, valid=1,value=0x3536373333
*
*/

static bool mesh_key_find(const char *p_src_key, uint32_t *p_dest_ltv)
{
    bool found = false;
    uint8_t *p_key;
    uint8_t *store_area_start = (uint8_t *)mesh_store_ptr_start + 4;
    uint8_t *store_area_end = (uint8_t *)mesh_store_ptr_end;

    if(!mesh_store_area_valid) { return found; }

    //search all area to compare the key;
    while(store_area_start < store_area_end) {
        if(store_area_start[0] == 0x00) { return found; }

        p_key = store_area_start + 1;

        if(!strcmp((char *)p_key, p_src_key)) {
            *p_dest_ltv  = (uint32_t)store_area_start;
            //BT_DBG("Found %s in nvram", p_key);
            return true;
        }

        //move to the next ltv
        store_area_start += store_area_start[0] + 1;
    }

    return found;
}

int tls_mesh_store_update(const char *key, uint8_t valid, const uint8_t *value, int value_len)
{
    bool found = false;
    uint32_t dest_ltv_addr = 0;
    uint8_t *p_dest_ltv_addr, *p_value;
    uint8_t ltv_offset = 0;
    //poor effiency!!!
    found = mesh_key_find(key, &dest_ltv_addr);

    if(found) {
        p_dest_ltv_addr = (uint8_t *)dest_ltv_addr;
        //update value;
        //sanity check;
        //dest_value_len = p_dest_ltv_addr[0] - strlen((char *)(p_dest_ltv_addr + 1)) - 1 - 1;
        /**Clear the value, value_len is 0*/
        //assert(dest_value_len == value_len);
        //sanity check end;
        p_value = p_dest_ltv_addr + strlen((char *)(p_dest_ltv_addr + 1)) + 1 + 1;
        p_value[0] = valid;
        p_value++;

        if(valid) {
            memcpy(p_value, value, value_len);
            TLS_BT_APPL_TRACE_DEBUG("update flash with key:%s, value:%s, valid=%d\r\n", key, bt_hex(value,
                                    value_len), valid);
        } else {
            TLS_BT_APPL_TRACE_DEBUG("update flash with key:%s, value cleared, valid=%d\r\n", key, valid);
        }
    } else {
        if(valid == 0) {
            BT_DBG("!!!clear an inexist key:%s", key);
            return 0;
        }

        if(!mesh_store_area_valid) {
            p_value = (uint8_t *)mesh_store_ptr_start;
            *p_value++ = 0xDE;
            *p_value++ = 0xAD;
            *p_value++ = 0xBE;
            *p_value++ = 0xAF;
            mesh_store_area_valid = 1;
        }

        p_value = (uint8_t *)mesh_store_ptr_offset;

        if((mesh_store_ptr_offset + strlen(key) + 1 + 1 + value_len) > mesh_store_ptr_end) {
            BT_ERR("No space to save mesh param...\r\n");
            assert(0);
            return -1;
        }

        /*construct the ltv array*/
        p_value[ltv_offset] = strlen(key) + 1 + 1 + value_len;
        ltv_offset++;
        //key field
        memcpy(&p_value[ltv_offset], key, strlen(key));
        ltv_offset += strlen(key);
        // '\0' key ending tag;
        p_value[ltv_offset] = 0;
        ltv_offset++;
        // valid field
        p_value[ltv_offset] = valid;
        ltv_offset++;
        //value fileld;
        memcpy(&p_value[ltv_offset], value, value_len);
        ltv_offset += value_len;
        //clear the left area, means the end of parameter area;
        p_value[ltv_offset] = 0;
        mesh_store_ptr_offset = (uint32_t)&p_value[ltv_offset];
        TLS_BT_APPL_TRACE_DEBUG("store flash with key:%s, value:%s\r\n", key, bt_hex(value, value_len));
        assert(mesh_store_ptr_offset <= mesh_store_ptr_end);
    }

    return 0;
}

int tls_mesh_store_retrieve(char *key, uint8_t *value, int *value_len)
{
    uint8_t *p_flag = NULL, valid;
    uint32_t ltv_total_len = 0, key_length = 0, value_length = 0, ret = 0;

    //sanity check;
    if(key == NULL || value == NULL || value_len == NULL) { return -1; }

    if(!mesh_store_area_valid) { return -1; }

    //length field
    p_flag = (uint8_t *)mesh_store_rd_ptr_offset;
    ltv_total_len = p_flag[0];
    key_length = strlen((char *)&p_flag[1]);

    if(ltv_total_len) {
        mesh_store_rd_ptr_offset += (ltv_total_len + 1); //move to the next item;
        assert(mesh_store_rd_ptr_offset < mesh_store_ptr_end);
        valid = p_flag[key_length + 1 + 1];

        if(valid) {
            ret = sprintf(key, "%s", (char *)(p_flag + 1));
            assert(ret == key_length);
            value_length = ltv_total_len - key_length - 1 - 1;
            *value_len = value_length;
            memcpy(value, (void *)(p_flag + key_length + 1 + 1 + 1), value_length);
            TLS_BT_APPL_TRACE_DEBUG("Load: key:%s, value:%s\r\n", key, bt_hex(value, value_length));
        } else {
            TLS_BT_APPL_TRACE_DEBUG("Invalid: key=%s\r\n", (char *)&p_flag[1]);
            *value_len = 0; //an empty pos; invalid flag
        }

        return 1;
    } else {
        mesh_store_ptr_offset = mesh_store_rd_ptr_offset; // move store ptr to an empty pos;
        return 0;
    }

    assert(0);
    return 0;
}

int tls_mesh_store_flush(void)
{
    tls_fls_write(TLS_FLASH_MESH_PARAM_ADDR, (uint8_t*)mesh_store_ptr_start, 0x1000);
    return 0;
}
int tls_mesh_store_erase(void)
{
    return tls_fls_erase(TLS_FLASH_MESH_PARAM_ADDR / INSIDE_FLS_SECTOR_SIZE);
}
void tls_mesh_store_init(void)
{
    int ret;
    
    uint8_t *ptr = NULL;
    mesh_store_ptr_start = (uint32_t)tls_mem_alloc(0x1000) ;     //tls_param_get_bt_param_address(TLS_PARAM_ID_BT_REMOTE_DEVICE_1, 0) ;
    assert(mesh_store_ptr_start != 0);
    mesh_store_ptr_end = (uint32_t)(mesh_store_ptr_start+0x1000);//tls_param_get_bt_param_address(TLS_PARAM_ID_BT_REMOTE_DEVICE_5,0) + sizeof(bt_remote_device_t);

    ret = tls_fls_read(TLS_FLASH_MESH_PARAM_ADDR, (uint8_t*)mesh_store_ptr_start, 0x1000);
    if(ret != 0)
    {
        printf("Read flash mesh param failed\r\n");
    }

    mesh_store_ptr_offset = mesh_store_ptr_start + 4;
    mesh_store_rd_ptr_offset = mesh_store_ptr_start + 4;
    ptr = (uint8_t *)mesh_store_ptr_start;

    if(ptr[0] == 0xDE && ptr[1] == 0xAD && ptr[2] == 0xBE && ptr[3] == 0xAF) {
        mesh_store_area_valid = 1;
    } else {
        mesh_store_area_valid = 0;
    }

    TLS_BT_APPL_TRACE_DEBUG("mesh_store_ptr_start=0x%08x, mesh_store_ptr_end=0x%08x, valid=%d\r\n",mesh_store_ptr_start, mesh_store_ptr_end, mesh_store_area_valid);
}
void tls_mesh_store_deinit(void)
{
    if(mesh_store_ptr_start)
    {
        tls_mem_free((void*)mesh_store_ptr_start);
        mesh_store_ptr_start = 0;
    }
}


#endif
