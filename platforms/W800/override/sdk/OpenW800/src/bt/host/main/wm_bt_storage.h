#ifndef __WM_BT_STORAGE_H__
#define __WM_BT_STORAGE_H__

#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>

#include "bt_target.h"

#define BTIF_CFG_TYPE_INVALID   0
#define BTIF_CFG_TYPE_STR       1
#define BTIF_CFG_TYPE_INT      (1 << 1)
#define BTIF_CFG_TYPE_BIN      (1 << 2)
#define BTIF_CFG_TYPE_VOLATILE (1 << 15)


typedef enum {
    NV_LOCAL = 0,
    NV_LOCAL_ADAPTER,
    NV_LOCAL_ADAPTER_ADDRESS,
    NV_LOCAL_ADAPTER_NAME,
    NV_LOCAL_ADAPTER_CLASS,
    NV_LOCAL_ADAPTER_IO_CAP,
    NV_LOCAL_ADAPTER_DISCOVERABLE,
    NV_LOCAL_ADAPTER_CONNECTABLE,
    NV_LOCAL_ADAPTER_DISCOVERY_TIMEOUT,
    NV_LOCAL_ADAPTER_AUTH_REQ,
    NV_LOCAL_ADAPTER_MODE,
    NV_LOCAL_ADAPTER_BLE_AUTH_REQ,
    NV_LOCAL_ADAPTER_BLE_IR,
    NV_LOCAL_ADAPTER_BLE_IRK,
    NV_LOCAL_ADAPTER_BLE_DHK,
    NV_LOCAL_ADAPTER_BLE_ER,
    NV_LOCAL_ADAPTER_MAX_TAG,
    NV_REMOTE,
    NV_REMOTE_ADDRESS,
    NV_REMOTE_NAME,
    NV_REMOTE_CLASS,
    NV_REMOTE_SERVICE,
    NV_REMOTE_LINK_KEY,
    NV_REMOTE_KEY_TYPE,
    NV_REMOTE_IO_CAP,
    NV_REMOTE_PIN_LENGTH,
    NV_REMOTE_DEVICE_TYPE,
    NV_REMOTE_BLE_ADDRESS_TYPE,
    NV_REMOTE_BLE_KEY_PENC,
    NV_REMOTE_BLE_KEY_LENC,
    NV_REMOTE_BLE_KEY_PID,
    NV_REMOTE_BLE_KEY_LID,
    NV_REMOTE_BLE_KEY_PCSRK,
    NV_REMOTE_BLE_KEY_LCSRK,
    NV_REMOTE_RECONNECT,
    NV_REMOTE_MANU,
    NV_REMOTE_LMPVER,
    NV_REMOTE_LMPSUBVER,
    NV_REMOTE_MAX_TAG
} nv_tag_t;

/*Adapter valid bit mask*/
#define NV_LOCAL_TAG            0
#define NV_REMOTE_TAG           1

#define ADAPTER_BD_ADDRESS_VALID_BIT          (1<<0)
#define ADAPTER_NAME_VALID_BIT                (1<<1)
#define ADAPTER_CLASS_VALID_BIT               (1<<2)
#define ADAPTER_IOCAP_VALID_BIT               (1<<3)
#define ADAPTER_DISCOVER_VALID_BIT            (1<<4)
#define ADAPTER_CONNECT_VALID_BIT             (1<<5)
#define ADAPTER_BT_AUTH_REQ_VALID_BIT         (1<<6)
#define ADAPTER_BLE_WORK_MODE_VALID_BIT       (1<<7)
#define ADAPTER_BLE_AUTH_REQ_VALID_BIT        (1<<8)
#define ADAPTER_BLE_IR_VALID_BIT              (1<<9)
#define ADAPTER_BLE_IRK_VALID_BIT             (1<<10)
#define ADAPTER_BLE_DHK_VALID_BIT             (1<<11)
#define ADAPTER_BLE_ER_VALID_BIT              (1<<12)



/*BT device valid bit mask*/
#define DEVICE_BD_ADDRESS_VALID_BIT             (1<<0)
#define DEVICE_NAME_VALID_BIT                   (1<<1)
#define DEVICE_CLASS_VALID_BIT                  (1<<2)
#define DEVICE_SERVICE_VALID_BIT                (1<<3)
#define DEVICE_LINK_KEY_VALID_BIT               (1<<4)
#define DEVICE_KEY_TYPE_VALID_BIT               (1<<5)
#define DEVICE_IOCAP_VALID_BIT                  (1<<6)
#define DEVICE_PIN_LENGTH_VALID_BIT             (1<<7)
#define DEVICE_TYPE_VALID_BIT                   (1<<8)
#define DEVICE_RECONNECT_VALID_BIT              (1<<9)
#define DEVICE_BLE_ADDR_TYPE_VALID_BIT          (1<<10)
#define DEVICE_BLE_KEY_PENC_VALID_BIT           (1<<11)
#define DEVICE_BLE_KEY_LENC_VALID_BIT           (1<<12)
#define DEVICE_BLE_KEY_PID_VALID_BIT            (1<<13)
#define DEVICE_BLE_KEY_LID_VALID_BIT            (1<<14)
#define DEVICE_BLE_KEY_PCSRK_VALID_BIT          (1<<15)
#define DEVICE_BLE_KEY_LCSRK_VALID_BIT          (1<<16)
#define DEVICE_RMT_MANU_VALID_BIT                         (1<<17)
#define DEVICE_RMT_LMP_VERSION_VALID_BIT                  (1<<18)
#define DEVICE_RMT_LMP_SUB_VERSION_VALID_BIT              (1<<19)


extern int btif_wm_config_get_remote_device(int index, void *ptr, int from_flash);

extern int btif_config_get_int(const char *section, const char *key, const char *name, int *value);
extern int btif_config_set_int(const char *section, const char *key, const char *name, int value);
extern int btif_config_get_str(const char *section, const char *key, const char *name, char *value,
                               int *bytes);
extern int btif_config_set_str(const char *section, const char *key, const char *name,
                               const char *value);

extern int btif_config_get(const char *section, const char *key, const char *name, char *value,
                           int *bytes, int *type);
extern int btif_config_set(const char *section, const char *key, const char *name,
                           const char  *value, int bytes, int type);

extern int btif_config_remove(const char *section, const char *key, const char *name);
extern int btif_config_filter_remove(const char *section, const char *filter[], int filter_count,
                                     int max_allowed);
extern int btif_config_exist(const char *section, const char *key, const char *name);
extern int btif_config_flush(int force);
extern int btif_config_save();
extern void btif_clear_remote_all();
extern int btif_wm_config_update_remote_device(const char *key);
#endif

