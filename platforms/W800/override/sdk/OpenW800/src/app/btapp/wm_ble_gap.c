#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>


#include "wm_bt_config.h"

#if (WM_BLE_INCLUDED == CFG_ON)

#include "wm_mem.h"
#include "list.h"
#include "wm_bt_def.h"
#include "wm_ble_gap.h"
#include "wm_bt_util.h"
#include "wm_ble.h"

/*
 * STRUCTURE DEFINITIONS
 ****************************************************************************************
 */

typedef struct {
    struct dl_list list;
    tls_ble_dm_evt_t evt;
    tls_ble_dm_callback_t reg_func_ptr;
} report_evt_t;
/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

static int g_force_privacy = 0;
static report_evt_t report_evt_list;

/*
 * DEFINES
 ****************************************************************************************
 */

#define PTR_TO_UINT(p) ((unsigned int) ((uintptr_t) (p)))
#define UINT_TO_PTR(u) ((void *) ((uintptr_t) (u)))
#define PTR_TO_INT(p) ((int) ((intptr_t) (p)))
#define INT_TO_PTR(i) ((void *) ((intptr_t) (i)))

/*
 * LOCAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */

static void tls_ble_dm_event_handler(tls_ble_dm_evt_t evt, tls_ble_dm_msg_t *msg)
{
    //TLS_BT_APPL_TRACE_EVENT("%s, event:%s,%d\r\n", __FUNCTION__, tls_dm_evt_2_str(evt), evt);
    uint32_t cpu_sr;

    switch(evt) {
        case WM_BLE_DM_TIMER_EXPIRED_EVT: {
            void *ptr = INT_TO_PTR(msg->dm_timer_expired.func_ptr);
            ((int (*)(int))ptr)(msg->dm_timer_expired.id);
            break;
        }

        case WM_BLE_DM_TRIGER_EVT: {
            void *ptr = INT_TO_PTR(msg->dm_evt_trigered.func_ptr);
            ((int (*)(int))ptr)(msg->dm_evt_trigered.id);
            break;
        }

        case WM_BLE_DM_SET_ADV_DATA_CMPL_EVT:
        case WM_BLE_DM_SCAN_RES_EVT:
        case WM_BLE_DM_REPORT_RSSI_EVT:
        case WM_BLE_DM_SCAN_RES_CMPL_EVT:
        case WM_BLE_DM_SET_SCAN_PARAM_CMPL_EVT: {
            report_evt_t *report_evt = NULL;
            report_evt_t *report_evt_next = NULL;
            cpu_sr = tls_os_set_critical();

            if(!dl_list_empty(&report_evt_list.list)) {
                dl_list_for_each_safe(report_evt, report_evt_next, &report_evt_list.list, report_evt_t, list) {
                    tls_os_release_critical(cpu_sr);

                    if((report_evt) && (report_evt->evt & evt) && (report_evt->reg_func_ptr)) {
                        report_evt->reg_func_ptr(evt, msg);
                    }

                    cpu_sr = tls_os_set_critical();
                }
            }

            tls_os_release_critical(cpu_sr);
            break;
        }

        default:
            TLS_BT_APPL_TRACE_WARNING("Warning , unhandled ble dm evt=%d\r\n", evt);
            break;
    }
}

int tls_ble_gap_init()
{
    dl_list_init(&report_evt_list.list);
    return tls_ble_dm_init(tls_ble_dm_event_handler);
}

int tls_ble_gap_deinit()
{
    uint32_t cpu_sr;
    report_evt_t *evt = NULL;
    report_evt_t *evt_next = NULL;

    if(dl_list_empty(&report_evt_list.list))
    { return TLS_BT_STATUS_SUCCESS; }

    cpu_sr = tls_os_set_critical();
    dl_list_for_each_safe(evt, evt_next, &report_evt_list.list, report_evt_t, list) {
        dl_list_del(&evt->list);
        tls_mem_free(evt);
    }
    tls_os_release_critical(cpu_sr);
    return tls_ble_dm_deinit();
}

void update_ble_privacy()
{
    g_force_privacy = 1;
}

int tls_ble_register_report_evt(tls_ble_dm_evt_t rpt_evt,  tls_ble_dm_callback_t rpt_callback)
{
    uint32_t cpu_sr;
    report_evt_t *evt = NULL;
    cpu_sr = tls_os_set_critical();
    dl_list_for_each(evt, &report_evt_list.list, report_evt_t, list) {
        if(evt->reg_func_ptr == rpt_callback) {
            if(evt->evt & rpt_evt) {
                /*Already in the list, do nothing*/
                tls_os_release_critical(cpu_sr);
                return TLS_BT_STATUS_SUCCESS;
            } else {
                /*Appending this evt to monitor list*/
                tls_os_release_critical(cpu_sr);
                evt->evt |= rpt_evt;
                return TLS_BT_STATUS_SUCCESS;
            }
        }
    }
    tls_os_release_critical(cpu_sr);
    evt = tls_mem_alloc(sizeof(report_evt_t));

    if(evt == NULL) {
        return TLS_BT_STATUS_NOMEM;
    }

    memset(evt, 0, sizeof(report_evt_t));
    evt->reg_func_ptr = rpt_callback;
    evt->evt = rpt_evt;
    cpu_sr = tls_os_set_critical();
    dl_list_add_tail(&report_evt_list.list, &evt->list);
    tls_os_release_critical(cpu_sr);
    return TLS_BT_STATUS_SUCCESS;
}
int tls_ble_deregister_report_evt(tls_ble_dm_evt_t rpt_evt,  tls_ble_dm_callback_t rpt_callback)
{
    uint32_t cpu_sr;
    report_evt_t *evt = NULL;
    report_evt_t *evt_next = NULL;
    cpu_sr = tls_os_set_critical();

    if(!dl_list_empty(&report_evt_list.list)) {
        dl_list_for_each_safe(evt, evt_next, &report_evt_list.list, report_evt_t, list) {
            tls_os_release_critical(cpu_sr);

            if((evt->reg_func_ptr == rpt_callback)) {
                evt->evt &= ~rpt_evt; //clear monitor bit;

                if(evt->evt == 0) {  //no evt left;
                    dl_list_del(&evt->list);
                    tls_mem_free(evt);
                    evt = NULL;
                }
            }

            cpu_sr = tls_os_set_critical();
        }
    }

    tls_os_release_critical(cpu_sr);
    return TLS_BT_STATUS_SUCCESS;
}

int tls_ble_gap_set_name(const char *dev_name, uint8_t update_flash)
{
    tls_bt_status_t ret;
    tls_bt_property_t prop;

    if(dev_name == NULL) {
        return TLS_BT_STATUS_PARM_INVALID;
    }

    prop.type = WM_BT_PROPERTY_BDNAME;
    prop.len = strlen(dev_name);     ////name length;
    prop.val = (void *)dev_name; ////name value;
    ret = tls_bt_set_adapter_property(&prop, update_flash);
    return ret;
}
#endif
