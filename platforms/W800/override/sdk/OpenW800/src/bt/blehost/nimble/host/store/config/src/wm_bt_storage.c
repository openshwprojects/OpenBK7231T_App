
/*****************************************************************************
**
**  Name:           wm_bt_storage.c
**
**  Description:    This file contains the  functions for bluetooth parameters storage
**
*****************************************************************************/
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#include "wm_bt_config.h"

#if (WM_NIMBLE_INCLUDED == CFG_ON)

#include "wm_params.h"
#include "wm_param.h"
#include "wm_bt_def.h"
#include "syscfg/syscfg.h"
#include "host/ble_hs.h"
#include "store/config/wm_bt_storage.h"
#include "ble_store_config_priv.h"


#define WM_BT_STORAGE_DEBUG_TAG 0
//our param area support max bond device count is 5
#define BTM_SEC_MAX_BLE_DEVICE_RECORDS MYNEWT_VAL(BLE_STORE_MAX_BONDS)

uint8_t btstorage_trace_level = 0;

#if WM_BT_STORAGE_DEBUG_TAG == 1
static char *nv_tag_2_str(uint8_t tag);

#define BTSTORAGE_TRACE_ERROR(...) printf("%s(L%d): " fmt, __FUNCTION__, __LINE__,  ## __VA_ARGS__);
#define BTSTORAGE_TRACE_WARNING(...)
#define BTSTORAGE_TRACE_API(...)
#define BTSTORAGE_TRACE_EVENT(...)
#define BTSTORAGE_TRACE_DEBUG(...) printf("%s(L%d): " fmt, __FUNCTION__, __LINE__,  ## __VA_ARGS__);
#define BTSTORAGE_TRACE_VERBOSE(...)

#else

#define BTSTORAGE_TRACE_ERROR(...)
#define BTSTORAGE_TRACE_WARNING(...)
#define BTSTORAGE_TRACE_API(...)
#define BTSTORAGE_TRACE_EVENT(...)
#define BTSTORAGE_TRACE_DEBUG(...)
#define BTSTORAGE_TRACE_VERBOSE(...)
#endif



static uint8_t string_to_bdaddr(const char *string, tls_bt_addr_t *addr)
{
    assert(string != NULL);
    assert(addr != NULL);
    tls_bt_addr_t new_addr;
    uint8_t *ptr = new_addr.address;
    uint8_t ret = sscanf(string, "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx",
                         &ptr[0], &ptr[1], &ptr[2], &ptr[3], &ptr[4], &ptr[5]) == 6;

    if(ret) {
        memcpy(addr, &new_addr, sizeof(tls_bt_addr_t));
    }

    return ret;
}

static const char *bd_to_string(const uint8_t *addr, char *string, size_t size)
{
    assert(addr != NULL);
    assert(string != NULL);

    if(size < 18) {
        return NULL;
    }

    const uint8_t *ptr = addr;
    sprintf(string, "%02x:%02x:%02x:%02x:%02x:%02x",
            ptr[0], ptr[1], ptr[2],
            ptr[3], ptr[4], ptr[5]);
    return string;
}

/*Only 16 bytes, the spec definition is 248 bytes*/
#define WM_BD_NAME_LEN 16

int btif_wm_config_get_remote_device(int index, void *ptr, int from_flash)
{
    assert(ptr != NULL);
    /*always from sram*/
    return tls_param_get(TLS_PARAM_ID_BT_REMOTE_DEVICE_1 + index, ptr, 0);
}

int btif_wm_config_set_local(const nv_tag_t section, const nv_tag_t key, const nv_tag_t name,
                             const char  *value, int bytes, int type, bool dummy_wr)
{
    int ret = TRUE;
    int err = 0;
    bt_adapter_t adapter;
    tls_bt_addr_t address;
    BTSTORAGE_TRACE_DEBUG("btif_wm_config_set_local [%s->%s->%s]\r\n", nv_tag_2_str(section),
                          nv_tag_2_str(key), nv_tag_2_str(name));

    if(section == NV_LOCAL) {
        tls_param_get(TLS_PARAM_ID_BT_ADAPTER, &adapter, 0);

        if(!dummy_wr) {
            if(adapter.valid_tag != 0xdeadbeaf) {
                adapter.valid_tag = 0xdeadbeaf;
            }
        } else {
            if(adapter.valid_tag != 0xdeadbeaf) {
                BTSTORAGE_TRACE_DEBUG("Clear local adapter information, but it is an empty pos!!!")
                return ret;
            }
        }

        switch((uint8_t)name) {
            case NV_LOCAL_ADAPTER_ADDRESS:
                BTSTORAGE_TRACE_DEBUG("Save local address:%s\r\n", value);

                if(!dummy_wr) {
                    adapter.valid_bit |= ADAPTER_BD_ADDRESS_VALID_BIT;
                    string_to_bdaddr(value, &address);
                    memcpy(adapter.bd_addr, address.address, 6);
                } else {
                    adapter.valid_bit &= ~ADAPTER_BD_ADDRESS_VALID_BIT;
                }

                break;

            case NV_LOCAL_ADAPTER_NAME:
                BTSTORAGE_TRACE_DEBUG("Save local name:%s\r\n", value);

                if((bytes <= WM_BD_NAME_LEN) && (!dummy_wr)) {
                    if(bytes > WM_BD_NAME_LEN) {
                        bytes = WM_BD_NAME_LEN;
                    }

                    adapter.valid_bit |= ADAPTER_NAME_VALID_BIT;
                    memcpy(adapter.name, value, bytes);
                    adapter.name_len = bytes;
                } else {
                    /*Invalid name parameter*/
                    adapter.valid_bit &= ~ADAPTER_NAME_VALID_BIT;
                    adapter.name_len = 0;

                    if(!dummy_wr) {
                        ret = FALSE;
                    }
                }

                break;

            case NV_LOCAL_ADAPTER_CLASS:
                if(!dummy_wr) {
                    adapter.class_of_device = *((uint32_t *)value);
                    adapter.valid_bit |= ADAPTER_CLASS_VALID_BIT;
                } else {
                    adapter.valid_bit &= ~ADAPTER_CLASS_VALID_BIT;
                }

                BTSTORAGE_TRACE_DEBUG("Save local device class:%04x\r\n", adapter.class_of_device);
                break;

            case NV_LOCAL_ADAPTER_IO_CAP:
                if(!dummy_wr) {
                    adapter.io_cap = *((uint8_t *)value);
                    adapter.valid_bit |= ADAPTER_IOCAP_VALID_BIT;
                } else {
                    adapter.valid_bit &= ~ADAPTER_IOCAP_VALID_BIT;
                }

                BTSTORAGE_TRACE_DEBUG("Save local iocap:%d\r\n", adapter.io_cap);
                break;

            case NV_LOCAL_ADAPTER_DISCOVERABLE:
                if(!dummy_wr) {
                    adapter.discoverable = *((uint8_t *)value);
                    adapter.valid_bit |= ADAPTER_DISCOVER_VALID_BIT;
                } else {
                    adapter.valid_bit &= ~ADAPTER_DISCOVER_VALID_BIT;
                }

                BTSTORAGE_TRACE_DEBUG("Save local discoverable:%d\r\n", adapter.discoverable);
                break;

            case NV_LOCAL_ADAPTER_CONNECTABLE:
                if(!dummy_wr) {
                    adapter.connectable = *((uint8_t *)value) ;
                    adapter.valid_bit |= ADAPTER_CONNECT_VALID_BIT;
                } else {
                    adapter.valid_bit &= ~ADAPTER_CONNECT_VALID_BIT;
                }

                BTSTORAGE_TRACE_DEBUG("Save local connectable:%d\r\n", adapter.connectable);
                break;

            case NV_LOCAL_ADAPTER_AUTH_REQ:
                if(!dummy_wr) {
                    adapter.bt_auth_req = *((uint8_t *)value);
                    adapter.valid_bit |= ADAPTER_BT_AUTH_REQ_VALID_BIT;
                } else {
                    adapter.valid_bit &= ~ADAPTER_BT_AUTH_REQ_VALID_BIT;
                }

                BTSTORAGE_TRACE_DEBUG("Save local auth:0x%02x\r\n", adapter.bt_auth_req);
                break;

            case NV_LOCAL_ADAPTER_MODE:
                if(!dummy_wr) {
                    adapter.mode = *((uint8_t *)value) ;
                    adapter.valid_bit |= ADAPTER_BLE_WORK_MODE_VALID_BIT;
                } else {
                    adapter.valid_bit &= ~ADAPTER_BLE_WORK_MODE_VALID_BIT;
                }

                BTSTORAGE_TRACE_DEBUG("Save local adapter mode:0x%02x\r\n", adapter.mode);
                break;
#if ( BTA_GATT_INCLUDED == TRUE && BLE_INCLUDED == TRUE)

            case NV_LOCAL_ADAPTER_BLE_AUTH_REQ:
                if(!dummy_wr) {
                    adapter.ble_auth_req = *((uint8_t *)value) ;
                    adapter.valid_bit |= ADAPTER_BLE_AUTH_REQ_VALID_BIT;
                } else {
                    adapter.valid_bit &= ~ADAPTER_BLE_AUTH_REQ_VALID_BIT;
                }

                break;

            case NV_LOCAL_ADAPTER_BLE_IR:
                if(!dummy_wr) {
                    adapter.valid_bit |= ADAPTER_BLE_IR_VALID_BIT;
                    memcpy(adapter.ir, value, 16);
                } else {
                    adapter.valid_bit &= ~ADAPTER_BLE_IR_VALID_BIT;
                }

                BTSTORAGE_TRACE_DEBUG("Save local ble ir type=%d", type);
                break;

            case NV_LOCAL_ADAPTER_BLE_ER:
                if(!dummy_wr) {
                    //assert(type == BTIF_CFG_TYPE_BIN);
                    adapter.valid_bit |= ADAPTER_BLE_ER_VALID_BIT;
                    memcpy(adapter.er, value, 16);
                } else {
                    adapter.valid_bit &= ~ADAPTER_BLE_ER_VALID_BIT;
                }

                BTSTORAGE_TRACE_DEBUG("Save local ble er type=%d", type);
                break;

            case NV_LOCAL_ADAPTER_BLE_IRK:
                if(!dummy_wr) {
                    //assert(type == BTIF_CFG_TYPE_BIN);
                    adapter.valid_bit |= ADAPTER_BLE_IRK_VALID_BIT;
                    memcpy(adapter.irk, value, 16);
                } else {
                    adapter.valid_bit &= ~ADAPTER_BLE_IRK_VALID_BIT;
                }

                BTSTORAGE_TRACE_DEBUG("Save local ble irk type=%d", type);
                break;

            case NV_LOCAL_ADAPTER_BLE_DHK:
                if(!dummy_wr) {
                    //assert(type == BTIF_CFG_TYPE_BIN);
                    adapter.valid_bit |= ADAPTER_BLE_DHK_VALID_BIT;
                    memcpy(adapter.dhk, value, 16);
                } else {
                    adapter.valid_bit &= ~ADAPTER_BLE_DHK_VALID_BIT;
                }

                BTSTORAGE_TRACE_DEBUG("Save local ble dhk type=%d", type);
                break;
#endif

            default:
                BTSTORAGE_TRACE_WARNING("ERROR ERROR ERROR, unknown tag index to save local parameter\r\n");
                ret = FALSE;
                break;
        }

        err = tls_param_set(TLS_PARAM_ID_BT_ADAPTER, &adapter, 0);

        if(err == TLS_PARAM_STATUS_OK) {
            ret = TRUE;
        } else {
            ret = FALSE;
        }
    } else {
        ret = FALSE;
    }

    return ret;
}
int btif_wm_config_get_local(const nv_tag_t section, const nv_tag_t key, const nv_tag_t name,
                             char *value, int *bytes, int *type, bool dummy_rd)
{
    int ret = TRUE;
    bt_adapter_t adapter;

    if(section == NV_LOCAL) {
        if(key == NV_LOCAL_ADAPTER) {
            tls_param_get(TLS_PARAM_ID_BT_ADAPTER, &adapter, 0);

            if(adapter.valid_tag != 0xdeadbeaf) {
                /*Flash content invalid, do nothing*/
                ret = FALSE;
                return ret;
            }

            switch((uint8_t)name) {
                case NV_LOCAL_ADAPTER_ADDRESS:
                    if(adapter.valid_bit & ADAPTER_BD_ADDRESS_VALID_BIT) {
                        if(!dummy_rd) {
                            bd_to_string(adapter.bd_addr, value, 18);
                            *bytes = 6;
                            *type = BTIF_CFG_TYPE_STR;
                        }
                    } else {
                        ret = FALSE;
                    }

                    break;

                case NV_LOCAL_ADAPTER_NAME:
                    if(adapter.valid_bit & ADAPTER_NAME_VALID_BIT) {
                        if(!dummy_rd) {
                            memcpy(value, adapter.name, adapter.name_len);
                            *bytes = adapter.name_len;
                            *type = BTIF_CFG_TYPE_STR;
                        }
                    } else {
                        /*Invalid name parameter*/
                        ret = FALSE;
                    }

                    break;

                case NV_LOCAL_ADAPTER_CLASS:
                    if(adapter.valid_bit & ADAPTER_CLASS_VALID_BIT) {
                        if(!dummy_rd) {
                            *((uint32_t *)value) = adapter.class_of_device;
                        }
                    } else {
                        ret = FALSE;
                    }

                    break;

                case NV_LOCAL_ADAPTER_IO_CAP:
                    if(adapter.valid_bit & ADAPTER_IOCAP_VALID_BIT) {
                        if(!dummy_rd) {
                            *((uint8_t *)value) = adapter.io_cap;
                        }
                    } else {
                        ret = FALSE;
                    }

                    break;

                case NV_LOCAL_ADAPTER_DISCOVERABLE:
                    if(adapter.valid_bit & ADAPTER_DISCOVER_VALID_BIT) {
                        if(!dummy_rd) {
                            *((uint8_t *)value) = adapter.discoverable;
                        }
                    } else {
                        ret = FALSE;
                    }

                    break;

                case NV_LOCAL_ADAPTER_CONNECTABLE:
                    if(adapter.valid_bit & ADAPTER_CONNECT_VALID_BIT) {
                        if(!dummy_rd) {
                            *((uint8_t *)value) = adapter.connectable;
                        }
                    } else {
                        ret = FALSE;
                    }

                    break;

                case NV_LOCAL_ADAPTER_AUTH_REQ:
                    if(adapter.valid_bit & ADAPTER_BT_AUTH_REQ_VALID_BIT) {
                        if(!dummy_rd) {
                            *((uint8_t *)value) = adapter.bt_auth_req;
                        }
                    } else {
                        ret = FALSE;
                    }

                    break;

                case NV_LOCAL_ADAPTER_MODE:
                    if(adapter.valid_bit & ADAPTER_BLE_WORK_MODE_VALID_BIT) {
                        if(!dummy_rd) {
                            *((uint8_t *)value) = adapter.mode;
                        }
                    } else {
                        ret = FALSE;
                    }

                    break;
#if ( BTA_GATT_INCLUDED == TRUE && BLE_INCLUDED == TRUE)

                case NV_LOCAL_ADAPTER_BLE_AUTH_REQ:
                    if(adapter.valid_bit & ADAPTER_BLE_AUTH_REQ_VALID_BIT) {
                        if(!dummy_rd) {
                            *((uint8_t *)value) = adapter.ble_auth_req;
                        }
                    } else {
                        ret = FALSE;
                    }

                    break;

                case NV_LOCAL_ADAPTER_BLE_IR:
                    assert(*type == BTIF_CFG_TYPE_BIN);

                    if(adapter.valid_bit & ADAPTER_BLE_IR_VALID_BIT) {
                        if(!dummy_rd) {
                            memcpy(value, adapter.ir, 16);
                            *bytes = 16;
                        }
                    } else {
                        ret = FALSE;
                    }

                    break;

                case NV_LOCAL_ADAPTER_BLE_ER:
                    assert(*type == BTIF_CFG_TYPE_BIN);

                    if(adapter.valid_bit & ADAPTER_BLE_ER_VALID_BIT) {
                        if(!dummy_rd) {
                            memcpy(value, adapter.er, 16);
                            *bytes = 16;
                        }
                    } else {
                        ret = FALSE;
                    }

                    break;

                case NV_LOCAL_ADAPTER_BLE_IRK:
                    assert(*type == BTIF_CFG_TYPE_BIN);

                    if(adapter.valid_bit & ADAPTER_BLE_IRK_VALID_BIT) {
                        if(!dummy_rd) {
                            memcpy(value, adapter.irk, 16);
                            *bytes = 16;
                        }
                    } else {
                        ret = FALSE;
                    }

                    break;

                case NV_LOCAL_ADAPTER_BLE_DHK:
                    assert(*type == BTIF_CFG_TYPE_BIN);

                    if(adapter.valid_bit & ADAPTER_BLE_DHK_VALID_BIT) {
                        if(!dummy_rd) {
                            memcpy(value, adapter.dhk, 16);
                            *bytes = 16;
                        }
                    } else {
                        ret = FALSE;
                    }

                    break;
#endif

                default:
                    ret = FALSE;
                    break;
            }
        } else {
            ret = FALSE;
        }
    } else {
        ret = FALSE;
    }

    if(ret) {
        BTSTORAGE_TRACE_DEBUG("btif_wm_config_got_local [%s->%s->%s]\r\n", nv_tag_2_str(section),
                              nv_tag_2_str(key), nv_tag_2_str(name));
    } else {
        BTSTORAGE_TRACE_DEBUG("There is no local adapter information:[%s->%s->%s]\r\n",
                              nv_tag_2_str(section), nv_tag_2_str(key), nv_tag_2_str(name));
    }

    return ret;
}
int btif_wm_config_get_local_int(const nv_tag_t section, const nv_tag_t key, const nv_tag_t name,
                                 int *value)
{
    int size = sizeof(*value);
    int type = BTIF_CFG_TYPE_INT;
    return btif_wm_config_get_local(section, key, name, (char *)value, &size, &type, false);
}
int btif_wm_config_set_local_int(const nv_tag_t section, const nv_tag_t key, const nv_tag_t name,
                                 int value)
{
    return btif_wm_config_set_local(section, key, name, (char *)&value, sizeof(value),
                                    BTIF_CFG_TYPE_INT, false);
}
int btif_wm_config_get_local_str(const nv_tag_t section, const nv_tag_t key, const nv_tag_t name,
                                 char *value, int *size)
{
    int type = BTIF_CFG_TYPE_STR;

    if(value) {
        *value = 0;
    }

    return btif_wm_config_get_local(section, key, name, value, size, &type, false);
}
int btif_wm_config_set_local_str(const nv_tag_t section, const nv_tag_t key, const nv_tag_t name,
                                 const char *value)
{
    value = value ? value : "";
    return btif_wm_config_set_local(section, key, name, value, strlen(value) + 1, BTIF_CFG_TYPE_STR,
                                    false);
}
int btif_wm_config_remove_local(const nv_tag_t section, const nv_tag_t key, const nv_tag_t name)
{
    return btif_wm_config_set_local(section, key, name, NULL, 0, 0, true);
}
int btif_wm_config_exist_local(const nv_tag_t section, const nv_tag_t key, const nv_tag_t name)
{
    return btif_wm_config_get_local(section, key, name, NULL, NULL, NULL, true);
}
int btif_wm_config_filter_remove_local(const char *section, const char *filter[], int filter_count,
                                       int max_allowed)
{
    return FALSE;
}



/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool btif_wm_config_find_by_key(uint8_t *bd_addr, int *index, int add)
{
    uint8_t i = 0;
    bool found = false;
    bt_remote_device_t device;

    for(i = 0; i < BTM_SEC_MAX_BLE_DEVICE_RECORDS; i++) {
        tls_param_get(TLS_PARAM_ID_BT_REMOTE_DEVICE_1 + i, (void *)&device, 0);

        if((device.valid_tag == 0xdeadbeaf) && device.in_use && (memcmp(device.bd_addr, bd_addr, 6) == 0)) {
            found = true;
            *index = i;
            return found;
        }
    }

    /*find an empty pos and insert one*/
    if((found == false) && add) {
        /*Try to find an empty pos*/
        for(i = 0; i < BTM_SEC_MAX_BLE_DEVICE_RECORDS; i++) {
            tls_param_get(TLS_PARAM_ID_BT_REMOTE_DEVICE_1 + i, (void *)&device, 0);

            /*For a fresh flash, valid tag must be not equal to 0xdeadbeaf*/
            if((device.valid_tag != 0xdeadbeaf) || (device.in_use == 0)) {
                found = true;
                *index = i;
                memcpy(device.bd_addr, bd_addr, 6);
                device.in_use = 1;
                device.valid_tag = 0xdeadbeaf;
                tls_param_set(TLS_PARAM_ID_BT_REMOTE_DEVICE_1 + i, (void *)&device, 0);
                break;
            }
        }
    }

    if((found == false) && (add)) {
        BTSTORAGE_TRACE_WARNING("No position to fit the bonding key, we try to erase an old one !!!\r\n");
        //TODO find the old one position;
        //the last pos must be the oldest one
        //the pos[0~(BTM_SEC_MAX_BLE_DEVICE_RECORDS-1)] will be updated once is paried with phone;
        found = true;
        *index = (BTM_SEC_MAX_BLE_DEVICE_RECORDS - 1);
        memcpy(device.bd_addr, bd_addr, 6);
        device.in_use = 1;
        device.valid_tag = 0xdeadbeaf;
        //the last pos will be overwitten;
        tls_param_set(TLS_PARAM_ID_BT_REMOTE_DEVICE_1 + (BTM_SEC_MAX_BLE_DEVICE_RECORDS - 1),
                      (void *)&device, 0);
    }

    return found;
}

int btif_wm_config_update_remote_device(const char *key)
{
    int ret = FALSE;
    tls_bt_addr_t addr;
    int index = -1;
    bt_remote_device_t update_device;
    bt_remote_device_t tmp_device;
    //assert(section == NV_REMOTE);
    string_to_bdaddr(key, &addr);

    //scan the total records
    if(btif_wm_config_find_by_key(addr.address, &index, 0)) {
        /*if index == 0, already the first pos, do nothing*/
        if(index != 0) {
            //for there is only five devices, so we sort it by hand;
            tls_param_get(TLS_PARAM_ID_BT_REMOTE_DEVICE_1 + index, (void *)&update_device, 0);

            if(index == 1) {
                tls_param_get(TLS_PARAM_ID_BT_REMOTE_DEVICE_1 + 0, (void *)&tmp_device, 0);      //read 0
                tls_param_set(TLS_PARAM_ID_BT_REMOTE_DEVICE_1 + index, (void *)&tmp_device, 0);  //0 write to 1
                tls_param_set(TLS_PARAM_ID_BT_REMOTE_DEVICE_1 + 0, (void *)&update_device, 0);
                ret = TRUE;
            } else if(index == 2) {
                tls_param_get(TLS_PARAM_ID_BT_REMOTE_DEVICE_1 + 1, (void *)&tmp_device, 0); //read 1
                tls_param_set(TLS_PARAM_ID_BT_REMOTE_DEVICE_1 + index, (void *)&tmp_device, 0);  //1->2
                tls_param_get(TLS_PARAM_ID_BT_REMOTE_DEVICE_1 + 0, (void *)&tmp_device, 0);      //read 0
                tls_param_set(TLS_PARAM_ID_BT_REMOTE_DEVICE_1 + 1, (void *)&tmp_device, 0);  //0 write to 1
                tls_param_set(TLS_PARAM_ID_BT_REMOTE_DEVICE_1 + 0, (void *)&update_device, 0);
                ret = TRUE;
            } else if(index == 3) {
                tls_param_get(TLS_PARAM_ID_BT_REMOTE_DEVICE_1 + 2, (void *)&tmp_device, 0); //read 2
                tls_param_set(TLS_PARAM_ID_BT_REMOTE_DEVICE_1 + index, (void *)&tmp_device, 0);  //2->3
                tls_param_get(TLS_PARAM_ID_BT_REMOTE_DEVICE_1 + 1, (void *)&tmp_device, 0); //read 1
                tls_param_set(TLS_PARAM_ID_BT_REMOTE_DEVICE_1 + 2, (void *)&tmp_device, 0);  //1->2
                tls_param_get(TLS_PARAM_ID_BT_REMOTE_DEVICE_1 + 0, (void *)&tmp_device, 0);      //read 0
                tls_param_set(TLS_PARAM_ID_BT_REMOTE_DEVICE_1 + 1, (void *)&tmp_device, 0);  //0 write to 1
                tls_param_set(TLS_PARAM_ID_BT_REMOTE_DEVICE_1 + 0, (void *)&update_device, 0);
                ret = TRUE;
            } else if(index == 4) {
                tls_param_get(TLS_PARAM_ID_BT_REMOTE_DEVICE_1 + 3, (void *)&tmp_device, 0); //read 3
                tls_param_set(TLS_PARAM_ID_BT_REMOTE_DEVICE_1 + index, (void *)&tmp_device, 0);  //3->4
                tls_param_get(TLS_PARAM_ID_BT_REMOTE_DEVICE_1 + 2, (void *)&tmp_device, 0); //read 2
                tls_param_set(TLS_PARAM_ID_BT_REMOTE_DEVICE_1 + 3, (void *)&tmp_device, 0);  //2->3
                tls_param_get(TLS_PARAM_ID_BT_REMOTE_DEVICE_1 + 1, (void *)&tmp_device, 0); //read 1
                tls_param_set(TLS_PARAM_ID_BT_REMOTE_DEVICE_1 + 2, (void *)&tmp_device, 0);  //1->2
                tls_param_get(TLS_PARAM_ID_BT_REMOTE_DEVICE_1 + 0, (void *)&tmp_device, 0);      //read 0
                tls_param_set(TLS_PARAM_ID_BT_REMOTE_DEVICE_1 + 1, (void *)&tmp_device, 0);  //0 write to 1
                tls_param_set(TLS_PARAM_ID_BT_REMOTE_DEVICE_1 + 0, (void *)&update_device, 0);
                ret = TRUE;
            } else {
                BTSTORAGE_TRACE_WARNING("We support only 5 remote devices by now!!!\r\n");
            }
        } else {
            BTSTORAGE_TRACE_DEBUG("Already the first pos, do nothing\r\n");
        }
    } else {
        BTSTORAGE_TRACE_WARNING("Warning, device[%s] is not found when updating\r\n", key);
    }

    if(ret) {
        btif_config_save();
    }

    return ret;
}


/********************************************************************************************************
*  FUNCTION: btif_wm_config_set_remote
*  DESCRIPTION:  dummy_wr 1 clear, 0 write the parameters
*
*
*********************************************************************************************************/

int btif_wm_config_set_remote(const nv_tag_t section, const char *key, const nv_tag_t name,
                              const char  *value, int bytes, int type, bool dummy_wr)
{
    return FALSE;
}
int btif_wm_config_get_remote(const nv_tag_t section, const char *key, const nv_tag_t name,
                              char *value, int *bytes, int *type, bool dummy_rd)
{
    return FALSE;
}

int btif_wm_config_filter_remove_remote(const char *section, const char *filter[], int filter_count,
                                        int max_allowed)
{
    return FALSE;
}

int local_name_str_to_index(const char *tag_name)
{
    const char *name[] = {"Address", "Name", "DevClass", "IOCAP", "Discoverable", "Connectable", "DiscoveryTimeout", "AuthReq", "ScanMode", "BleAuthReq",  "LE_LOCAL_KEY_IR", "LE_LOCAL_KEY_IRK", "LE_LOCAL_KEY_DHK", "LE_LOCAL_KEY_ER", NULL};
    int i = 0;

    do {
        if(strncmp(name[i], tag_name, strlen(tag_name)) == 0) {
            return (i + (int)NV_LOCAL_ADAPTER_ADDRESS);
        }

        i++;
    } while(name[i] != NULL);

    BTSTORAGE_TRACE_ERROR("ERROR ERROR ERROR , unindexed local tag string(%s)\r\n", tag_name);
    assert(0);
    return -1;
}

int remote_name_str_to_index(const char *tag_name)
{
    const char *name[] = {"Address", "Name", "DevClass", "Service", "LinkKey", "LinkKeyType", "IOCAP", "PinLength", "DevType", "AddrType", "LE_KEY_PENC", "LE_KEY_LENC", "LE_KEY_PID", "LE_KEY_LID", "LE_KEY_PCSRK", "LE_KEY_LCSRK", "Reconnect", "Manufacturer", "LmpVer", "LmpSubVer", NULL};
    int i = 0;

    do {
        if(strncmp(name[i], tag_name, strlen(tag_name)) == 0) {
            return (i + (int)NV_REMOTE_ADDRESS);
        }

        i++;
    } while(name[i] != NULL);

    BTSTORAGE_TRACE_ERROR("ERROR ERROR ERROR , unindexed remote tag string(%s)\r\n", tag_name);
    assert(0);
    return -1;
}


int btif_config_get_int(const char *section, const char *key, const char *name, int *value)
{
    BTSTORAGE_TRACE_EVENT("btif_config_get_int section:%s, key:%s, name:%s\r\n", section, key, name);
    int index = -1;

    if((strncmp(section, "Local", 5) == 0) && (strncmp(key, "Adapter", 7) == 0)) {
        index = local_name_str_to_index(name);

        if(index > 0) {
            return btif_wm_config_get_local_int(NV_LOCAL, NV_LOCAL_ADAPTER, index, value);
        }
    } else {
    }

    return FALSE;
}



int btif_config_set_int(const char *section, const char *key, const char *name, int value)
{
    BTSTORAGE_TRACE_EVENT("btif_config_set_int section:%s, key:%s, name:%s\r\n", section, key, name);
    int index = -1;

    if((strncmp(section, "Local", 5) == 0) && (strncmp(key, "Adapter", 7) == 0)) {
        index = local_name_str_to_index(name);

        if(index > 0) {
            return btif_wm_config_set_local_int(NV_LOCAL, NV_LOCAL_ADAPTER, index, value);
        }
    } else {
    }

    return FALSE;
}
int btif_config_get_str(const char *section, const char *key, const char *name, char *value,
                        int *size)
{
    BTSTORAGE_TRACE_EVENT("btif_config_get_str section:%s, key:%s, name:%s\r\n", section, key, name);
    int index = -1;

    if((strncmp(section, "Local", 5) == 0) && (strncmp(key, "Adapter", 7) == 0)) {
        index = local_name_str_to_index(name);

        if(index > 0) {
            return btif_wm_config_get_local_str(NV_LOCAL, NV_LOCAL_ADAPTER, index, value, size);
        }
    } else {
    }

    return FALSE;
}
int btif_config_set_str(const char *section, const char *key, const char *name, const char *value)
{
    BTSTORAGE_TRACE_EVENT("btif_config_set_str section:%s, key:%s, name:%s\r\n", section, key, name);
    int index = -1;

    if((strncmp(section, "Local", 5) == 0) && (strncmp(key, "Adapter", 7) == 0)) {
        index = local_name_str_to_index(name);

        if(index > 0) {
            return btif_wm_config_set_local_str(NV_LOCAL, NV_LOCAL_ADAPTER, index, value);
        }
    } else {
    }

    return FALSE;
}

int btif_config_get(const char *section, const char *key, const char *name, char *value, int *bytes,
                    int *type)
{
    BTSTORAGE_TRACE_EVENT("btif_config_get section:%s, key:%s, name:%s\r\n", section, key, name);
    int index = -1;

    if((strncmp(section, "Local", 5) == 0) && (strncmp(key, "Adapter", 7) == 0)) {
        index = local_name_str_to_index(name);

        if(index > 0) {
            return btif_wm_config_get_local(NV_LOCAL, NV_LOCAL_ADAPTER, index, value, bytes, type, false);
        }
    } else {
    }

    return FALSE;
}
int btif_config_set(const char *section, const char *key, const char *name, const char  *value,
                    int bytes, int type)
{
    BTSTORAGE_TRACE_EVENT("btif_config_set section:%s, key:%s, name:%s\r\n", section, key, name);
    int index = -1;

    if((strncmp(section, "Local", 5) == 0) && (strncmp(key, "Adapter", 7) == 0)) {
        index = local_name_str_to_index(name);

        if(index > 0) {
            return btif_wm_config_set_local(NV_LOCAL, NV_LOCAL_ADAPTER, index, value, bytes, type, false);
        }
    } else {
    }

    return FALSE;
}

int btif_config_remove(const char *section, const char *key, const char *name)
{
    BTSTORAGE_TRACE_EVENT("btif_config_remove section:%s, key:%s, name:%s\r\n", section, key, name);
    int index = -1;

    if((strncmp(section, "Local", 5) == 0) && (strncmp(key, "Adapter", 7) == 0)) {
        index = local_name_str_to_index(name);

        if(index > 0) {
            return btif_wm_config_remove_local(NV_LOCAL, NV_LOCAL_ADAPTER, index);
        }
    } else {
    }

    return FALSE;
}
int btif_config_filter_remove(const char *section, const char *filter[], int filter_count,
                              int max_allowed)
{
    BTSTORAGE_TRACE_EVENT("btif_config_filter_remove ...section:[%s]filter_count:[%d]\r\n", section,
                          filter_count);
    return FALSE;
}
int btif_config_exist(const char *section, const char *key, const char *name)
{
    BTSTORAGE_TRACE_DEBUG("btif_config_exist ...section:[%s] key:[%s] name:[%s]\r\n", section, key,
                          name);
    int index = -1;

    if((strncmp(section, "Local", 5) == 0) && (strncmp(key, "Adapter", 7) == 0)) {
        index = local_name_str_to_index(name);

        if(index > 0) {
            return btif_wm_config_exist_local(NV_LOCAL, NV_LOCAL_ADAPTER, index);
        }
    } else {
    }

    return FALSE;
}

int btif_config_remove_remote(const char *key)
{
    int ret = TRUE;
    return ret;
}

int btif_config_save()
{
    //tls_param_to_flash(TLS_PARAM_ID_ALL);
    return TRUE;
}
int btif_config_flush(int force)
{
    if(force) { tls_param_to_flash(TLS_PARAM_ID_ALL); }

    return TRUE;
}
/*debug function */
void btif_clear_remote_all()
{
    tls_param_to_flash(TLS_PARAM_ID_ALL);
}

#define NVRAM_ADDR_PAYLOAD_OFFSET 12
#define NVRAM_OUR_SEC_PAYLOAD_OFFSET   (NVRAM_ADDR_PAYLOAD_OFFSET+7)
#define NVRAM_PEER_SEC_PAYLOAD_OFFSET  (NVRAM_OUR_SEC_PAYLOAD_OFFSET+72)
#define NVRAM_CCCD_SEC_PAYLOAD_OFFSET  (NVRAM_PEER_SEC_PAYLOAD_OFFSET+72)

int btif_config_get_sec_index(void *addr, uint8_t *found)
{
    int i = 0;
    bt_remote_device_t device;
    ble_addr_t *addr_offset;
    uint8_t *ptr_offset = (uint8_t *)&device;

    for(i = 0; i < BTM_SEC_MAX_BLE_DEVICE_RECORDS; i++) {
        tls_param_get(TLS_PARAM_ID_BT_REMOTE_DEVICE_1 + i, (void *)&device, 0);

        if((device.valid_tag == 0xdeadbeae)) {
            addr_offset = (ble_addr_t *)(ptr_offset + NVRAM_ADDR_PAYLOAD_OFFSET);

            if(ble_addr_cmp((ble_addr_t *)addr, addr_offset) == 0) {
                *found = 1;
                return i;
            }
        }
    }

    for(i = 0; i < BTM_SEC_MAX_BLE_DEVICE_RECORDS; i++) {
        tls_param_get(TLS_PARAM_ID_BT_REMOTE_DEVICE_1 + i, (void *)&device, 0);

        if(device.valid_tag != 0xdeadbeae) {
            return i;
        }
    }

    return -1;
}


void dump_bt_device_info(bt_remote_device_t *device)
{
    uint8_t *ptr = (uint8_t *)device;
    int i = 0, j = 0;
    printf("dump device info valid_tag:%08x,valid_bit:0x%08x\r\n", device->valid_tag,
           device->valid_bit);

    for(i = 0; i < sizeof(bt_remote_device_t); i++) {
        printf("%02x ", ptr[i]);
        j++;

        if(j == 16) {
            j = 0;
            printf("\r\n");
        }
    }

    printf("\r\n");
}

int btif_config_store_cccd(int idx, void *addr, int count, void *payload, int length)
{
    bt_remote_device_t device;
    uint8_t *ptr_offset = (uint8_t *)&device;
    assert(idx < BTM_SEC_MAX_BLE_DEVICE_RECORDS);
    memset(&device, 0, sizeof(device));
    tls_param_get(TLS_PARAM_ID_BT_REMOTE_DEVICE_1 + idx, (void *)&device, 0);

    if((device.valid_tag == 0xdeadbeae)) {
        device.valid_bit = (count) << 2 | (device.valid_bit & 0x03);
        ptr_offset += NVRAM_CCCD_SEC_PAYLOAD_OFFSET;
        memcpy(ptr_offset, payload, length);
    } else {
        device.valid_tag = 0xdeadbeae;
        device.valid_bit = (count) << 2;
        ptr_offset += NVRAM_ADDR_PAYLOAD_OFFSET;
        memcpy(ptr_offset, addr, sizeof(ble_addr_t));
        //ptr_offset += sizeof(ble_addr_t);
        ptr_offset = (uint8_t *)NVRAM_CCCD_SEC_PAYLOAD_OFFSET;
        memcpy(ptr_offset, payload, length);
    }

    tls_param_set(TLS_PARAM_ID_BT_REMOTE_DEVICE_1 + idx, (void *)&device, 0);
    //dump_bt_device_info(&device);
    return TRUE;
}
int btif_config_store_our_sec(int idx, void *addr, uint8_t *payload, int length)
{
    bt_remote_device_t device;
    uint8_t *ptr_offset = (uint8_t *)&device;
    assert(idx < BTM_SEC_MAX_BLE_DEVICE_RECORDS);
    memset(&device, 0, sizeof(device));
    tls_param_get(TLS_PARAM_ID_BT_REMOTE_DEVICE_1 + idx, (void *)&device, 0);

    if((device.valid_tag == 0xdeadbeae)) {
        device.valid_bit |= 0x01;
        ptr_offset += NVRAM_OUR_SEC_PAYLOAD_OFFSET;
        memcpy(ptr_offset, payload, length);
    } else {
        device.valid_tag = 0xdeadbeae;
        device.valid_bit = 0x01;
        ptr_offset += NVRAM_ADDR_PAYLOAD_OFFSET;
        memcpy(ptr_offset, addr, sizeof(ble_addr_t));
        ptr_offset += sizeof(ble_addr_t);
        memcpy(ptr_offset, payload, length);
    }

    tls_param_set(TLS_PARAM_ID_BT_REMOTE_DEVICE_1 + idx, (void *)&device, 0);
    //dump_bt_device_info(&device);
    return 0;
}

int btif_config_store_peer_sec(int idx, void *addr, uint8_t *payload, int length)
{
    bt_remote_device_t device;
    uint8_t *ptr_offset = (uint8_t *)&device;
    assert(idx < BTM_SEC_MAX_BLE_DEVICE_RECORDS);
    memset(&device, 0, sizeof(device));
    tls_param_get(TLS_PARAM_ID_BT_REMOTE_DEVICE_1 + idx, (void *)&device, 0);

    if((device.valid_tag == 0xdeadbeae)) {
        device.valid_bit |= 0x02;
        ptr_offset += NVRAM_PEER_SEC_PAYLOAD_OFFSET;
        memcpy(ptr_offset, payload, length);
    } else {
        device.valid_tag = 0xdeadbeae;
        device.valid_bit = 0x02;
        ptr_offset += NVRAM_ADDR_PAYLOAD_OFFSET;
        memcpy(ptr_offset, addr, sizeof(ble_addr_t));
        //ptr_offset += sizeof(ble_addr_t);
        ptr_offset = (uint8_t *)NVRAM_PEER_SEC_PAYLOAD_OFFSET;
        memcpy(ptr_offset, payload, length);
    }

    tls_param_set(TLS_PARAM_ID_BT_REMOTE_DEVICE_1 + idx, (void *)&device, 0);
    //dump_bt_device_info(&device);
    return 0;
}

uint32_t btif_config_get_sec_cccd_item(int idx, void *addr, void *our_sec, int our_sec_size,
                                       void *peer_sec, int peer_sec_size, void *cccd_info, int cccd_info_size)
{
    bt_remote_device_t device;
    uint8_t *ptr_offset = (uint8_t *)&device;
    uint32_t valid_bit = 0;
    assert(idx < BTM_SEC_MAX_BLE_DEVICE_RECORDS);
    memset(&device, 0, sizeof(device));
    tls_param_get(TLS_PARAM_ID_BT_REMOTE_DEVICE_1 + idx, (void *)&device, 0);

    if((device.valid_tag == 0xdeadbeae)) {
        valid_bit = device.valid_bit;
        memcpy(addr, ptr_offset + NVRAM_ADDR_PAYLOAD_OFFSET, 7);
        memcpy(our_sec, ptr_offset + NVRAM_OUR_SEC_PAYLOAD_OFFSET, our_sec_size);
        memcpy(peer_sec, ptr_offset + NVRAM_PEER_SEC_PAYLOAD_OFFSET, peer_sec_size);
        memcpy(cccd_info, ptr_offset + NVRAM_CCCD_SEC_PAYLOAD_OFFSET, cccd_info_size);
        //dump_bt_device_info(&device);
    }

    return valid_bit;
}

int btif_config_delete_our_sec(int idx)
{
    bt_remote_device_t device;
    memset(&device, 0, sizeof(device));
    assert(idx < BTM_SEC_MAX_BLE_DEVICE_RECORDS);
    tls_param_get(TLS_PARAM_ID_BT_REMOTE_DEVICE_1 + idx, (void *)&device, 0);

    if((device.valid_tag == 0xdeadbeae)) {
        if(device.valid_bit & 0x01) {
            device.valid_bit &= (~0x01);

            if((device.valid_bit & 0x03) == 0x00) {
                device.valid_tag = 0x88888888; //invalid tag!!!
            }

            tls_param_set(TLS_PARAM_ID_BT_REMOTE_DEVICE_1 + idx, (void *)&device, 0);
        }
    }

    return 0;
}
int btif_config_delete_cccd(int idx)
{
    bt_remote_device_t device;
    memset(&device, 0, sizeof(device));
    assert(idx < BTM_SEC_MAX_BLE_DEVICE_RECORDS);
    tls_param_get(TLS_PARAM_ID_BT_REMOTE_DEVICE_1 + idx, (void *)&device, 0);

    if((device.valid_tag == 0xdeadbeae)) {
        if(device.valid_bit & 0xFFFFFFFC) {
            device.valid_bit &= (0x03);

            if((device.valid_bit & 0x03) == 0x00) {
                device.valid_tag = 0x88888888; //invalid tag!!!
            }

            tls_param_set(TLS_PARAM_ID_BT_REMOTE_DEVICE_1 + idx, (void *)&device, 0);
        }
    }

    return 0;
}

int btif_config_delete_peer_sec(int idx)
{
    bt_remote_device_t device;
    memset(&device, 0, sizeof(device));
    assert(idx < BTM_SEC_MAX_BLE_DEVICE_RECORDS);
    tls_param_get(TLS_PARAM_ID_BT_REMOTE_DEVICE_1 + idx, (void *)&device, 0);

    if((device.valid_tag == 0xdeadbeae)) {
        if(device.valid_bit & 0x02) {
            device.valid_bit &= (~0x02);

            if((device.valid_bit & 0x03) == 0x00) { //omit the cccd counter;
                device.valid_tag = 0x88888888; //invalid tag!!!
            }

            tls_param_set(TLS_PARAM_ID_BT_REMOTE_DEVICE_1 + idx, (void *)&device, 0);
        }
    }

    return 0;
}
int btif_config_delete_all()
{
    int idx = 0;
    bt_remote_device_t device;
    
    device.valid_tag = 0x88888888;
    device.valid_bit = 0;

    for(idx = 0; idx < BTM_SEC_MAX_BLE_DEVICE_RECORDS; idx++)
    {
        tls_param_set(TLS_PARAM_ID_BT_REMOTE_DEVICE_1 + idx, (void *)&device, 0);
    }

    return 0;
}

int btif_config_store_key_map(const uint8_t *map_info, int length, bool force_flush)
{
    bt_adapter_t adapter;
    tls_param_get(TLS_PARAM_ID_BT_ADAPTER, &adapter, 0);

    if(adapter.valid_tag != 0xdeadbeaf) {
        adapter.valid_tag = 0xdeadbeaf;
    }

    adapter.valid_bit |=
                    ADAPTER_BLE_IR_VALID_BIT; //reuse the ADAPTER_BLE_IR_VALID_BIT to mark the map key value;
    memcpy(&adapter.ir[0], map_info, length);
    tls_param_set(TLS_PARAM_ID_BT_ADAPTER, (void *)&adapter, 0);

    if(force_flush) {
        btif_config_flush(1);
    } else {
        ble_store_config_persist_flush();
    }

    return 0;
}

int btif_config_load_key_map(uint8_t *map_info, int length)
{
    bt_adapter_t adapter;
    memset(&adapter, 0, sizeof(adapter));
    tls_param_get(TLS_PARAM_ID_BT_ADAPTER, &adapter, 0);

    if((adapter.valid_tag == 0xdeadbeaf) && (adapter.valid_bit & ADAPTER_BLE_IR_VALID_BIT)) {
        memcpy(map_info, &adapter.ir[0], length);
        return 0;
    } else {
        return -1;
    }
}


#if WM_BT_STORAGE_DEBUG_TAG == 1

#ifndef CASE_RETURN_STR
#define CASE_RETURN_STR(const) case const: return #const;
#endif

static char *nv_tag_2_str(uint8_t state)
{
    switch(state) {
            CASE_RETURN_STR(NV_LOCAL)
            CASE_RETURN_STR(NV_LOCAL_ADAPTER)
            CASE_RETURN_STR(NV_LOCAL_ADAPTER_ADDRESS)
            CASE_RETURN_STR(NV_LOCAL_ADAPTER_NAME)
            CASE_RETURN_STR(NV_LOCAL_ADAPTER_CLASS)
            CASE_RETURN_STR(NV_LOCAL_ADAPTER_IO_CAP)
            CASE_RETURN_STR(NV_LOCAL_ADAPTER_DISCOVERABLE)
            CASE_RETURN_STR(NV_LOCAL_ADAPTER_CONNECTABLE)
            CASE_RETURN_STR(NV_LOCAL_ADAPTER_DISCOVERY_TIMEOUT)
            CASE_RETURN_STR(NV_LOCAL_ADAPTER_AUTH_REQ)
            CASE_RETURN_STR(NV_LOCAL_ADAPTER_MODE)
            CASE_RETURN_STR(NV_LOCAL_ADAPTER_BLE_AUTH_REQ)
            CASE_RETURN_STR(NV_LOCAL_ADAPTER_BLE_IR)
            CASE_RETURN_STR(NV_LOCAL_ADAPTER_BLE_IRK)
            CASE_RETURN_STR(NV_LOCAL_ADAPTER_BLE_DHK)
            CASE_RETURN_STR(NV_LOCAL_ADAPTER_BLE_ER)
            CASE_RETURN_STR(NV_LOCAL_ADAPTER_MAX_TAG)
            CASE_RETURN_STR(NV_REMOTE)
            CASE_RETURN_STR(NV_REMOTE_ADDRESS)
            CASE_RETURN_STR(NV_REMOTE_NAME)
            CASE_RETURN_STR(NV_REMOTE_CLASS)
            CASE_RETURN_STR(NV_REMOTE_SERVICE)
            CASE_RETURN_STR(NV_REMOTE_LINK_KEY)
            CASE_RETURN_STR(NV_REMOTE_KEY_TYPE)
            CASE_RETURN_STR(NV_REMOTE_IO_CAP)
            CASE_RETURN_STR(NV_REMOTE_PIN_LENGTH)
            CASE_RETURN_STR(NV_REMOTE_DEVICE_TYPE)
            CASE_RETURN_STR(NV_REMOTE_BLE_ADDRESS_TYPE)
            CASE_RETURN_STR(NV_REMOTE_BLE_KEY_PENC)
            CASE_RETURN_STR(NV_REMOTE_BLE_KEY_LENC)
            CASE_RETURN_STR(NV_REMOTE_BLE_KEY_PID)
            CASE_RETURN_STR(NV_REMOTE_BLE_KEY_LID)
            CASE_RETURN_STR(NV_REMOTE_BLE_KEY_PCSRK)
            CASE_RETURN_STR(NV_REMOTE_BLE_KEY_LCSRK)
            CASE_RETURN_STR(NV_REMOTE_MANU)
            CASE_RETURN_STR(NV_REMOTE_LMPVER)
            CASE_RETURN_STR(NV_REMOTE_LMPSUBVER)
            CASE_RETURN_STR(NV_REMOTE_MAX_TAG)

        default:
            return "~~~~~~~~~~~~~~~~!!!Unknown nv tag ID!!!~~~~~~~~~~~~~~~~~";
    }
}
#endif

#endif
