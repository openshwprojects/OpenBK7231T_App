/*****************************************************************************
**
**  Name:           wm_bt_spp_server.c
**
**  Description:    This file contains the sample functions for bluetooth spp server application
**
*****************************************************************************/

#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#include "wm_bt_config.h"

#if (WM_BTA_SPPS_INCLUDED == CFG_ON)

#include "wm_bt_spp.h"
#include "wm_bt_util.h"
#include "wm_bt_spp_server.h"

#include "wm_osal.h"

#include "bt_utils.h"

/*
 * DEFINES
 ****************************************************************************************
 */

#define SPP_SERVER_NAME "WM_SPP_SERVER"

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

static const wm_spp_sec_t spp_sec_mask = WM_SPP_SEC_NONE;
//static const tls_spp_role_t spp_role_slave = WM_SPP_ROLE_SERVER;

static int g_send_freq = 0;
static int last_sys_time = 0;
static int g_recv_bytes = 0;

/*
 * LOCAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */

static void btspp_init_status_callback(uint8_t status)
{
    TLS_BT_APPL_TRACE_DEBUG("%s, status=%d\r\n", __FUNCTION__, status);

    if(status == 0) {
        tls_bt_spp_start_server(spp_sec_mask, WM_SPP_ROLE_SERVER, 0, SPP_SERVER_NAME);
    }
}

static void btspp_data_indication_callback(uint8_t status, uint32_t handle, uint8_t *p_data,
        uint16_t length)
{
    TLS_BT_APPL_TRACE_DEBUG("%s, status=%d\r\n", __FUNCTION__, status);
#if 0
	{
	    int i = 0;

	    printf("DATA:\r\n");

	    for(i = 0; i < length; i++) { printf("%02x ", p_data[i]); }

	    printf("\r\n");
	}
#endif
    g_recv_bytes += length;

    if((tls_os_get_time() - last_sys_time) > 500) {
        last_sys_time =  tls_os_get_time();
        TLS_BT_APPL_TRACE_ERROR("speed: %5.2f kbps/s\r\n", g_recv_bytes * 8 / 1000.0);
        g_recv_bytes = 0;
    }

    g_send_freq++;

    if(g_send_freq > 10) {
        //tls_bt_spp_write(handle, p_data, length);
        g_send_freq = 0;
    }
}
static void btspp_server_open_callback(uint8_t status, uint32_t handle)
{
    TLS_BT_APPL_TRACE_DEBUG("%s, status=%d, handle=%d\r\n", __FUNCTION__, status, handle);
}
static void btspp_server_start_callback(uint8_t status, uint32_t handle, uint8_t sec_id,
                                        bool use_co_rfc)
{
    TLS_BT_APPL_TRACE_DEBUG("%s, status=%d, handle=%d(0x%08x), sec_id=%d, use_co_rfc=%d\r\n",
                            __FUNCTION__,
                            status, handle, handle, sec_id, use_co_rfc);
}

static void wm_bt_spp_callback(tls_spp_event_t evt, tls_spp_msg_t *msg)
{
    TLS_BT_APPL_TRACE_DEBUG("%s, event=%s(%d)\r\n", __FUNCTION__, tls_spp_evt_2_str(evt), evt);

    switch(evt) {
        case WM_SPP_INIT_EVT:
            btspp_init_status_callback(msg->init_msg.status);
            break;

        case WM_SPP_DISCOVERY_COMP_EVT:
            break;

        case WM_SPP_OPEN_EVT:
            break;

        case WM_SPP_CLOSE_EVT:
            break;

        case WM_SPP_START_EVT:
            btspp_server_start_callback(msg->start_msg.status, msg->start_msg.handle, msg->start_msg.sec_id,
                                        msg->start_msg.use_co_rfc);
            break;

        case WM_SPP_CL_INIT_EVT:
            break;

        case WM_SPP_DATA_IND_EVT:
            btspp_data_indication_callback(msg->data_ind_msg.status, msg->data_ind_msg.handle,
                                           msg->data_ind_msg.data, msg->data_ind_msg.length);
            break;

        case WM_SPP_CONG_EVT:
            break;

        case WM_SPP_WRITE_EVT:
            break;

        case WM_SPP_SRV_OPEN_EVT:
            btspp_server_open_callback(msg->srv_open_msg.status, msg->srv_open_msg.handle);
            break;

        default:
            break;
    }
}

/*
 * EXPORTED FUNCTION DEFINITIONS
 ****************************************************************************************
 */

tls_bt_status_t tls_bt_enable_spp_server()
{
    tls_bt_status_t status;
    TLS_BT_APPL_TRACE_DEBUG("%s\r\n", __func__);
    status = tls_bt_spp_init(wm_bt_spp_callback);
	UNUSED(status);
    return tls_bt_spp_enable();
}

tls_bt_status_t tls_bt_disable_spp_server()
{
    tls_bt_status_t status;
    TLS_BT_APPL_TRACE_DEBUG("%s\r\n", __func__);
    tls_bt_spp_disable();
    status = tls_bt_spp_deinit();
    return status;
}

#endif

