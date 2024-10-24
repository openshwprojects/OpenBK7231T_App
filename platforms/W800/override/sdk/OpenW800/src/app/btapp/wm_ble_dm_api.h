#ifndef __WM_BLE_DM_API_H__
#define __WM_BLE_DM_API_H__

/*****************************************************************************
**  External Function Declarations
*****************************************************************************/
#ifdef __cplusplus
extern "C"
{
#endif

/*******************************************************************************
 **
 ** Function         tls_dm_get_timer_id
 **
 ** Description      Allocate a timer from btif system;
 **
 ** Parameters       None;
 **
 ** Returns          >=0 success; <0 failed;
 **
 *******************************************************************************/

int8_t tls_dm_get_timer_id(void);

/*******************************************************************************
 **
 ** Function         tls_dm_free_timer_id
 **
 ** Description      free a timer to btif system;
 **
 ** Parameters       timer_id;
 **
 ** Returns          None;
 **
 *******************************************************************************/

void tls_dm_free_timer_id(uint8_t timer_id);

/*******************************************************************************
 **
 ** Function         tls_dm_stop_timer
 **
 ** Description      stop a specific timer ;
 **
 ** Parameters       timer_id;
 **
 ** Returns          None;
 **
 *******************************************************************************/

tls_bt_status_t tls_dm_stop_timer(uint8_t timer_id);
/*******************************************************************************
 **
 ** Function         tls_dm_start_timer
 **
 ** Description      enable a timer ;
 **
 ** Parameters       timer_id;
 **                  tls_ble_dm_timer_callback_t; timer expired callback function
 **
 ** Returns          TLS_BT_STATUS_SUCCESS;
 **               TLS_BT_STATUS_NOMEM

 **
 *******************************************************************************/

tls_bt_status_t tls_dm_start_timer(uint8_t timer_id, uint32_t timeout_ms,
                                   tls_ble_dm_timer_callback_t callback);

#ifdef __cplusplus
}
#endif



#endif
