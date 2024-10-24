#ifndef __WM_BLE_WIFI_PROF_H__
#define __WM_BLE_WIFI_PROF_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*op_exec_write_callback)(int exec);
typedef int (*op_write_callback)(int offset, uint8_t *ptr, int length, bool b_prep);
typedef int (*op_read_callback)(int offset);
typedef int (*op_disconnected_callback)(int status);
typedef int (*op_connected_callback)(int status);
typedef int (*op_indication_callback)(int status);
typedef int (*op_service_enabled_callback)(int status);
typedef int (*op_mtu_changed_callback)(int mtu);


typedef struct {
    size_t size;

    op_service_enabled_callback enabled_cb;

    op_connected_callback connected_cb;

    op_disconnected_callback disconnected_cb;

    op_read_callback read_cb;

    op_write_callback write_cb;

    op_exec_write_callback exec_write_cb;

    op_indication_callback indication_cb;

    op_mtu_changed_callback mtu_changed_cb;

} wm_ble_wifi_prof_callbacks_t;

int tls_ble_wifi_prof_init(wm_ble_wifi_prof_callbacks_t *callback);
int tls_ble_wifi_prof_deinit(int reason);
int tls_ble_wifi_prof_disconnect(int status);
int tls_ble_wifi_prof_send_msg(uint8_t *ptr, int length);
int tls_ble_wifi_prof_disable(int status);
int tls_ble_wifi_adv(bool enable);

#ifdef __cplusplus
}
#endif


#endif

