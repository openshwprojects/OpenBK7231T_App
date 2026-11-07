#if PLATFORM_RTL8720D || PLATFORM_RTL87X0C

#include "../../../new_common.h"
#include "../../../new_pins.h"
#include "../../../new_cfg.h"
#include "../../hal_bt_proxy.h"
#include "../../../logging/logging.h"
#include "../../../driver/drv_esphome_api.h"

#if ENABLE_BT_PROXY

#include <string.h>
#include <stdio.h>
#include <os_sched.h>
#include "wifi_conf.h"
#include "trace_app.h"
#include "gap.h"
#include "gap_adv.h"
#include "gap_bond_le.h"
#include "gap_callback_le.h"
#include "gap_config.h"
#include "gap_conn_le.h"
#include "gap_le.h"
#include "gap_le_types.h"
#include "gap_msg.h"
#include "gap_privacy.h"
#include "gap_scan.h"
#include "gap_storage_le.h"
#include "os_task.h"
#include "os_msg.h"
#include "os_mem.h"
#include "app_msg.h"
#include <wifi/wifi_conf.h>
#include "rtk_coex.h"
#include "bte.h"
#if ENABLE_BT_PROXY_GATT
#include "profile_client.h"
#include "gcs_client.h"
#endif

#define DEFAULT_SCAN_INTERVAL     0x520	// 820ms
#define DEFAULT_SCAN_WINDOW       0x520	// 820ms

typedef struct
{
	bool init_done;
	bool stack_ready;
	bool scan_active;		// user wants scanning (BTStartScan)
	bool scan_running;		// controller is actually scanning
	bool scan_paused;		// temporarily stopped while establishing a link
	int scan_total_packets;
	int scan_dropped_packets;
	int scan_last_total;
	int scan_idle_seconds;

	TaskHandle_t task_handle;
} hal_bt_state_t;

extern int bt_get_mac_address(uint8_t* mac);
extern void WiFI_GetMacAddress(char* mac);
extern unsigned int g_timeMs;
static hal_bt_state_t g_bt_proxy = { 0 };
static void* _evtQueueHandle;
static void* _ioQueueHandle;
static bool btq_task_running = true;
static uint16_t scan_interval = DEFAULT_SCAN_INTERVAL;
static uint16_t scan_window = DEFAULT_SCAN_WINDOW;
static uint8_t scan_mode = GAP_SCAN_MODE_PASSIVE;

static void hal_bt_scan_apply(void);

#if ENABLE_BT_PROXY_GATT
// ===========================================================================
// Active connections
//
// Threading model: every piece of GATT state below is owned by the BT app task
// (hal_btq_task). Other threads (ESPHome API server, main loop) only allocate a
// gatt_cmd_t and post it to the BT task's IO queue; the stack's GAP messages and
// GCS client callbacks are delivered in that same task.
// ===========================================================================

#define IO_MSG_TYPE_OBK_GATT		0x0F00
#define LINK_QUEUE_LEN				8
#define DISC_MAX_SERVICES			16
#define DISC_MAX_CHARS				32
#define DISC_MAX_DESCS				40
#define GCS_DISCOV_TABLE_SIZE		64

#define CONNECT_TIMEOUT_MS			20000
#define MTU_WAIT_MS					2000
#define OP_TIMEOUT_MS				10000

#define OBK_ATT_ERR_UNLIKELY			0x0e

typedef enum
{
	GATT_CMD_CONNECT = 1,
	GATT_CMD_DISCONNECT,
	GATT_CMD_DISCONNECT_ALL,
	GATT_CMD_SERVICES,
	GATT_CMD_READ,
	GATT_CMD_WRITE,
	GATT_CMD_NOTIFY,
	GATT_CMD_TICK,
} gatt_cmd_type_t;

typedef struct
{
	uint8_t type;
	uint8_t addr_type;
	bool response;
	uint64_t address;
	uint16_t handle;
	uint16_t len;
	uint8_t data[];
} gatt_cmd_t;

typedef enum
{
	LINK_FREE = 0,
	LINK_CONNECTING,
	LINK_WAIT_MTU,
	LINK_CONNECTED,
	LINK_DISCONNECTING,
} link_state_t;

typedef struct
{
	link_state_t state;
	uint64_t address;
	uint8_t bd[6];
	uint8_t bd_type;
	uint8_t conn_id;		// 0xFF until the stack assigns one
	bool user_disconnect;
	uint16_t mtu;
	uint32_t state_ms;
	gatt_cmd_t* queue[LINK_QUEUE_LEN];
	uint8_t q_head;
	uint8_t q_count;
	gatt_cmd_t* active;
	uint32_t active_ms;
} gatt_link_t;

typedef enum
{
	DISC_SRV = 0,
	DISC_CHAR,
	DISC_DESC,
} disc_phase_t;

typedef struct
{
	uint8_t uuid[16];
	uint16_t start;
	uint16_t end;
} disc_service_t;

typedef struct
{
	int link;				// index into g_links, -1 = workspace free
	disc_phase_t phase;
	uint8_t n_svcs;
	uint8_t cur;
	disc_service_t svcs[DISC_MAX_SERVICES];
	uint8_t n_chars;
	bt_gatt_char_t chars[DISC_MAX_CHARS];
	uint8_t n_descs;
	bt_gatt_desc_t descs[DISC_MAX_DESCS];
} disc_ws_t;

static gatt_link_t g_links[BT_PROXY_GATT_MAX_LINKS];
static disc_ws_t g_disc = { .link = -1 };
static T_CLIENT_ID g_gcs_client_id = CLIENT_PROFILE_GENERAL_ID;
static volatile int g_link_led_state = 0;

static const uint8_t BT_BASE_UUID[16] = {
	0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static void uuid_from16(uint8_t* out, uint16_t u16)
{
	memcpy(out, BT_BASE_UUID, 16);
	out[12] = u16 & 0xFF;
	out[13] = u16 >> 8;
}

static void addr_to_bd(uint64_t address, uint8_t* bd)
{
	for(int i = 0; i < 6; i++) bd[i] = (address >> (8 * i)) & 0xFF;
}

static int link_index(gatt_link_t* l)
{
	return (int)(l - g_links);
}

static gatt_link_t* link_by_addr(uint64_t address)
{
	for(int i = 0; i < BT_PROXY_GATT_MAX_LINKS; i++)
	{
		if(g_links[i].state != LINK_FREE && g_links[i].address == address) return &g_links[i];
	}
	return NULL;
}

static gatt_link_t* link_by_conn(uint8_t conn_id)
{
	for(int i = 0; i < BT_PROXY_GATT_MAX_LINKS; i++)
	{
		if(g_links[i].state != LINK_FREE && g_links[i].conn_id == conn_id) return &g_links[i];
	}
	return NULL;
}

static gatt_link_t* link_connecting_unassigned(void)
{
	for(int i = 0; i < BT_PROXY_GATT_MAX_LINKS; i++)
	{
		if(g_links[i].state == LINK_CONNECTING && g_links[i].conn_id == 0xFF) return &g_links[i];
	}
	return NULL;
}

static void link_update_led(void)
{
	int s = 0;
	for(int i = 0; i < BT_PROXY_GATT_MAX_LINKS; i++)
	{
		gatt_link_t* l = &g_links[i];
		if(l->state == LINK_CONNECTED && s < 2) s = (g_disc.link == i) ? 1 : 2;
		else if(l->state == LINK_CONNECTING || l->state == LINK_WAIT_MTU) s = 1;
	}
	g_link_led_state = s;
}

static void cmd_free(gatt_cmd_t* c)
{
	if(c) os_mem_free(c);
}

static void link_flush(gatt_link_t* l)
{
	cmd_free(l->active);
	l->active = NULL;
	while(l->q_count)
	{
		cmd_free(l->queue[l->q_head]);
		l->q_head = (l->q_head + 1) % LINK_QUEUE_LEN;
		l->q_count--;
	}
	l->q_head = 0;
}

static void link_reset(gatt_link_t* l)
{
	link_flush(l);
	if(g_disc.link == link_index(l)) g_disc.link = -1;
	memset(l, 0, sizeof(*l));
	l->state = LINK_FREE;
	l->conn_id = 0xFF;
}

static bool link_push(gatt_link_t* l, gatt_cmd_t* c)
{
	if(l->q_count >= LINK_QUEUE_LEN) return false;
	l->queue[(l->q_head + l->q_count) % LINK_QUEUE_LEN] = c;
	l->q_count++;
	return true;
}

static void link_push_front(gatt_link_t* l, gatt_cmd_t* c)
{
	l->q_head = (l->q_head + LINK_QUEUE_LEN - 1) % LINK_QUEUE_LEN;
	l->queue[l->q_head] = c;
	l->q_count++;
}

static gatt_cmd_t* link_pop(gatt_link_t* l)
{
	if(!l->q_count) return NULL;
	gatt_cmd_t* c = l->queue[l->q_head];
	l->q_head = (l->q_head + 1) % LINK_QUEUE_LEN;
	l->q_count--;
	return c;
}

static bool scan_any_link_connecting(void)
{
	for(int i = 0; i < BT_PROXY_GATT_MAX_LINKS; i++)
	{
		if(g_links[i].state == LINK_CONNECTING) return true;
	}
	return false;
}

static void link_kick(gatt_link_t* l);
static void link_kick_all(void)
{
	for(int i = 0; i < BT_PROXY_GATT_MAX_LINKS; i++) link_kick(&g_links[i]);
}

static void link_op_done(gatt_link_t* l)
{
	cmd_free(l->active);
	l->active = NULL;
	link_kick(l);
}

// ---------------------------------------------------------------------------
// Service discovery state machine
// ---------------------------------------------------------------------------
static void disc_finish(gatt_link_t* l, bool ok)
{
	if(ok) ESPHome_API_BT_OnServicesDone(l->address);
	else ESPHome_API_BT_OnError(l->address, 0, OBK_ATT_ERR_UNLIKELY);
	g_disc.link = -1;
	link_update_led();
	link_op_done(l);
	link_kick_all(); // another link may be waiting for the workspace
}

static void disc_emit_current(gatt_link_t* l)
{
	disc_service_t* s = &g_disc.svcs[g_disc.cur];
	bt_gatt_service_t svc;
	memcpy(svc.uuid, s->uuid, 16);
	svc.handle = s->start;
	svc.end_handle = s->end;
	svc.char_count = g_disc.n_chars;
	svc.chars = g_disc.chars;
	svc.descs = g_disc.descs;
	ESPHome_API_BT_OnService(l->address, &svc);
}

// Starts char discovery of the next service that needs it, emitting services
// that cannot be discovered further. Ends the procedure after the last one.
static void disc_next_service(gatt_link_t* l)
{
	while(g_disc.cur < g_disc.n_svcs)
	{
		disc_service_t* s = &g_disc.svcs[g_disc.cur];
		g_disc.phase = DISC_CHAR;
		g_disc.n_chars = 0;
		g_disc.n_descs = 0;
		if(gcs_all_char_discovery(l->conn_id, s->start, s->end) == GAP_CAUSE_SUCCESS)
		{
			return;
		}
		ADDLOG_ERROR(LOG_FEATURE_GENERAL, "BTProxy: char discovery failed for svc 0x%04X", s->start);
		disc_emit_current(l);
		g_disc.cur++;
	}
	disc_finish(l, true);
}

static void disc_start_descs(gatt_link_t* l)
{
	disc_service_t* s = &g_disc.svcs[g_disc.cur];
	if(g_disc.n_chars > 0)
	{
		uint16_t start = g_disc.chars[0].handle + 1;
		if(start <= s->end)
		{
			g_disc.phase = DISC_DESC;
			if(gcs_all_char_descriptor_discovery(l->conn_id, start, s->end) == GAP_CAUSE_SUCCESS)
			{
				return;
			}
		}
	}
	disc_emit_current(l);
	g_disc.cur++;
	disc_next_service(l);
}

static bool disc_is_char_attr(uint16_t h)
{
	for(int k = 0; k < g_disc.n_chars; k++)
	{
		if(g_disc.chars[k].decl_handle == h || g_disc.chars[k].handle == h) return true;
	}
	return false;
}

static void disc_on_result(gatt_link_t* l, T_GCS_DISCOVERY_RESULT* r)
{
	switch(g_disc.phase)
	{
		case DISC_SRV:
		{
			if(r->discov_type != GCS_ALL_PRIMARY_SRV_DISCOV) return;
			if(!r->is_success || r->result_num == 0)
			{
				ADDLOG_ERROR(LOG_FEATURE_GENERAL, "BTProxy: primary service discovery failed");
				disc_finish(l, false);
				return;
			}
			g_disc.n_svcs = 0;
			for(int i = 0; i < r->result_num && g_disc.n_svcs < DISC_MAX_SERVICES; i++)
			{
				T_GCS_DISCOV_RESULT* e = &r->p_result_table[i];
				disc_service_t* s = &g_disc.svcs[g_disc.n_svcs];
				if(e->result_type == DISC_RESULT_ALL_SRV_UUID16)
				{
					uuid_from16(s->uuid, e->result_data.srv_uuid16_disc_data.uuid16);
					s->start = e->result_data.srv_uuid16_disc_data.att_handle;
					s->end = e->result_data.srv_uuid16_disc_data.end_group_handle;
				}
				else if(e->result_type == DISC_RESULT_ALL_SRV_UUID128)
				{
					memcpy(s->uuid, e->result_data.srv_uuid128_disc_data.uuid128, 16);
					s->start = e->result_data.srv_uuid128_disc_data.att_handle;
					s->end = e->result_data.srv_uuid128_disc_data.end_group_handle;
				}
				else
				{
					continue;
				}
				g_disc.n_svcs++;
			}
			ADDLOG_INFO(LOG_FEATURE_GENERAL, "BTProxy: %d services", g_disc.n_svcs);
			g_disc.cur = 0;
			disc_next_service(l);
			break;
		}
		case DISC_CHAR:
		{
			if(r->discov_type != GCS_ALL_CHAR_DISCOV) return;
			g_disc.n_chars = 0;
			for(int i = 0; r->is_success && i < r->result_num && g_disc.n_chars < DISC_MAX_CHARS; i++)
			{
				T_GCS_DISCOV_RESULT* e = &r->p_result_table[i];
				bt_gatt_char_t* c = &g_disc.chars[g_disc.n_chars];
				memset(c, 0, sizeof(*c));
				if(e->result_type == DISC_RESULT_CHAR_UUID16)
				{
					uuid_from16(c->uuid, e->result_data.char_uuid16_disc_data.uuid16);
					c->decl_handle = e->result_data.char_uuid16_disc_data.decl_handle;
					c->handle = e->result_data.char_uuid16_disc_data.value_handle;
					c->properties = e->result_data.char_uuid16_disc_data.properties & 0xFF;
				}
				else if(e->result_type == DISC_RESULT_CHAR_UUID128)
				{
					memcpy(c->uuid, e->result_data.char_uuid128_disc_data.uuid128, 16);
					c->decl_handle = e->result_data.char_uuid128_disc_data.decl_handle;
					c->handle = e->result_data.char_uuid128_disc_data.value_handle;
					c->properties = e->result_data.char_uuid128_disc_data.properties & 0xFF;
				}
				else
				{
					continue;
				}
				g_disc.n_chars++;
			}
			disc_start_descs(l);
			break;
		}
		case DISC_DESC:
		{
			if(r->discov_type != GCS_ALL_CHAR_DESC_DISCOV) return;
			g_disc.n_descs = 0;
			// Find Information returns every attribute in the range, sorted by handle.
			// Skip characteristic declarations/values and assign the rest to the
			// characteristic whose value handle precedes them.
			for(int i = 0; r->is_success && i < r->result_num && g_disc.n_descs < DISC_MAX_DESCS; i++)
			{
				T_GCS_DISCOV_RESULT* e = &r->p_result_table[i];
				bt_gatt_desc_t* d = &g_disc.descs[g_disc.n_descs];
				if(e->result_type == DISC_RESULT_CHAR_DESC_UUID16)
				{
					uint16_t u = e->result_data.char_desc_uuid16_disc_data.uuid16;
					if(u >= 0x2800 && u <= 0x2803) continue;
					uuid_from16(d->uuid, u);
					d->handle = e->result_data.char_desc_uuid16_disc_data.handle;
				}
				else if(e->result_type == DISC_RESULT_CHAR_DESC_UUID128)
				{
					memcpy(d->uuid, e->result_data.char_desc_uuid128_disc_data.uuid128, 16);
					d->handle = e->result_data.char_desc_uuid128_disc_data.handle;
				}
				else
				{
					continue;
				}
				if(disc_is_char_attr(d->handle)) continue;
				int owner = -1;
				for(int k = 0; k < g_disc.n_chars; k++)
				{
					if(g_disc.chars[k].handle < d->handle) owner = k;
				}
				if(owner < 0) continue;
				bt_gatt_char_t* c = &g_disc.chars[owner];
				if(c->desc_count == 0) c->desc_first = g_disc.n_descs;
				else if(c->desc_first + c->desc_count != g_disc.n_descs) continue; // keep ranges contiguous
				c->desc_count++;
				g_disc.n_descs++;
			}
			disc_emit_current(l);
			g_disc.cur++;
			disc_next_service(l);
			break;
		}
	}
}

// ---------------------------------------------------------------------------
// Per-link operation queue (one outstanding ATT request per link)
// ---------------------------------------------------------------------------
static bool cause_is_busy(T_GAP_CAUSE cause)
{
	return cause == GAP_CAUSE_ALREADY_IN_REQ || cause == GAP_CAUSE_ERROR_CREDITS || cause == GAP_CAUSE_NO_RESOURCE;
}

static void link_kick(gatt_link_t* l)
{
	while(l->state == LINK_CONNECTED && l->active == NULL && l->q_count > 0)
	{
		gatt_cmd_t* c = link_pop(l);
		T_GAP_CAUSE cause = GAP_CAUSE_SUCCESS;
		switch(c->type)
		{
			case GATT_CMD_SERVICES:
				if(g_disc.link != -1)
				{
					// workspace used by another link; retried when it frees up
					link_push_front(l, c);
					return;
				}
				g_disc.link = link_index(l);
				g_disc.phase = DISC_SRV;
				g_disc.n_svcs = 0;
				cause = gcs_all_primary_srv_discovery(l->conn_id);
				if(cause != GAP_CAUSE_SUCCESS) g_disc.link = -1;
				break;
			case GATT_CMD_READ:
				cause = gcs_attr_read(l->conn_id, c->handle);
				break;
			case GATT_CMD_WRITE:
				cause = gcs_attr_write(l->conn_id, c->response ? GATT_WRITE_TYPE_REQ : GATT_WRITE_TYPE_CMD,
					c->handle, c->len, c->data);
				ADDLOG_EXTRADEBUG(LOG_FEATURE_GENERAL, "BTProxy: write h=%d len=%d %s -> cause %d",
					c->handle, c->len, c->response ? "req" : "cmd", cause);
				if(cause == GAP_CAUSE_SUCCESS && !c->response)
				{
					// Write Command: no ATT response, HA does not wait either
					cmd_free(c);
					continue;
				}
				break;
			default:
				cmd_free(c);
				continue;
		}
		if(cause == GAP_CAUSE_SUCCESS)
		{
			l->active = c;
			l->active_ms = g_timeMs;
			link_update_led();
			return;
		}
		if(cause_is_busy(cause))
		{
			link_push_front(l, c);
			return; // retried on next tick
		}
		ADDLOG_ERROR(LOG_FEATURE_GENERAL, "BTProxy: GATT op %d on 0x%04X failed, cause %d", c->type, c->handle, cause);
		ESPHome_API_BT_OnError(l->address, c->handle, OBK_ATT_ERR_UNLIKELY);
		cmd_free(c);
	}
}

static T_APP_RESULT hal_bt_gcs_callback(T_CLIENT_ID client_id, uint8_t conn_id, void* p_data)
{
	if(client_id != g_gcs_client_id) return APP_RESULT_SUCCESS;
	gatt_link_t* l = link_by_conn(conn_id);
	if(!l) return APP_RESULT_SUCCESS;
	T_GCS_CLIENT_CB_DATA* cb = (T_GCS_CLIENT_CB_DATA*)p_data;

	switch(cb->cb_type)
	{
		case GCS_CLIENT_CB_TYPE_DISC_RESULT:
			if(g_disc.link == link_index(l) && l->active && l->active->type == GATT_CMD_SERVICES)
			{
				disc_on_result(l, &cb->cb_content.discov_result);
			}
			break;
		case GCS_CLIENT_CB_TYPE_READ_RESULT:
		{
			T_GCS_READ_RESULT* r = &cb->cb_content.read_result;
			ADDLOG_EXTRADEBUG(LOG_FEATURE_GENERAL, "BTProxy: read result h=%d cause 0x%x len=%d", r->handle, r->cause, r->value_size);
			if(!l->active || l->active->type != GATT_CMD_READ) break;
			if(r->cause == GAP_SUCCESS) ESPHome_API_BT_OnRead(l->address, l->active->handle, r->p_value, r->value_size);
			else ESPHome_API_BT_OnError(l->address, l->active->handle, r->cause & 0xFF);
			link_op_done(l);
			break;
		}
		case GCS_CLIENT_CB_TYPE_WRITE_RESULT:
		{
			T_GCS_WRITE_RESULT* w = &cb->cb_content.write_result;
			ADDLOG_EXTRADEBUG(LOG_FEATURE_GENERAL, "BTProxy: write result h=%d type %d cause 0x%x", w->handle, w->type, w->cause);
			if(w->type != GATT_WRITE_TYPE_REQ) break; // write commands complete on send
			if(!l->active || l->active->type != GATT_CMD_WRITE) break;
			if(w->cause == GAP_SUCCESS) ESPHome_API_BT_OnWrite(l->address, l->active->handle);
			else ESPHome_API_BT_OnError(l->address, l->active->handle, w->cause & 0xFF);
			link_op_done(l);
			break;
		}
		case GCS_CLIENT_CB_TYPE_NOTIF_IND:
		{
			T_GCS_NOTIF_IND* n = &cb->cb_content.notif_ind;
			ADDLOG_EXTRADEBUG(LOG_FEATURE_GENERAL, "BTProxy: %s h=%d len=%d", n->notify ? "notify" : "indicate", n->handle, n->value_size);
			ESPHome_API_BT_OnNotifyData(l->address, n->handle, n->p_value, n->value_size);
			break;
		}
		default:
			break;
	}
	return APP_RESULT_SUCCESS;
}

// ---------------------------------------------------------------------------
// Connection management
// ---------------------------------------------------------------------------
static void link_report_connected(gatt_link_t* l)
{
	l->state = LINK_CONNECTED;
	l->state_ms = g_timeMs;
	link_update_led();
	ESPHome_API_BT_OnConnection(l->address, true, l->mtu, 0);
	link_kick(l);
}

static void gatt_on_conn_state(uint8_t conn_id, uint8_t new_state, uint16_t disc_cause)
{
	gatt_link_t* l = link_by_conn(conn_id);
	if(!l && (new_state == GAP_CONN_STATE_CONNECTING || new_state == GAP_CONN_STATE_CONNECTED))
	{
		l = link_connecting_unassigned();
		if(l) l->conn_id = conn_id;
	}
	ADDLOG_INFO(LOG_FEATURE_GENERAL, "BTProxy: conn %d state %d cause 0x%x%s", conn_id, new_state, disc_cause, l ? "" : " (untracked)");
	if(!l)
	{
		if(new_state == GAP_CONN_STATE_CONNECTED) le_disconnect(conn_id);
		return;
	}

	switch(new_state)
	{
		case GAP_CONN_STATE_CONNECTING:
			if(l->user_disconnect) le_disconnect(conn_id); // cancelled before we had an id
			break;
		case GAP_CONN_STATE_CONNECTED:
			if(l->user_disconnect)
			{
				le_disconnect(conn_id);
				break;
			}
			l->state = LINK_WAIT_MTU;
			l->state_ms = g_timeMs;
			l->mtu = 23;
			hal_bt_scan_apply();
			link_update_led();
			break;
		case GAP_CONN_STATE_DISCONNECTED:
		{
			uint64_t address = l->address;
			int err = l->user_disconnect ? 0 : (disc_cause & 0xFF);
			if(!l->user_disconnect && err == 0) err = 0x08; // connection timeout / failed
			link_reset(l);
			link_update_led();
			ESPHome_API_BT_OnConnection(address, false, 0, err);
			ESPHome_API_BT_OnConnectionsFreeChanged();
			hal_bt_scan_apply();
			link_kick_all();
			break;
		}
		default:
			break;
	}
}

static void gatt_on_mtu(uint8_t conn_id, uint16_t mtu)
{
	gatt_link_t* l = link_by_conn(conn_id);
	if(!l) return;
	l->mtu = mtu;
	ADDLOG_INFO(LOG_FEATURE_GENERAL, "BTProxy: conn %d mtu %d", conn_id, mtu);
	if(l->state == LINK_WAIT_MTU) link_report_connected(l);
}

static void gatt_do_connect(gatt_cmd_t* c)
{
	gatt_link_t* l = link_by_addr(c->address);
	if(l)
	{
		if(l->state == LINK_CONNECTED)
		{
			ESPHome_API_BT_OnConnection(l->address, true, l->mtu, 0);
		}
		// already connecting: the pending result will be reported
		return;
	}
	if(scan_any_link_connecting())
	{
		ESPHome_API_BT_OnConnection(c->address, false, 0, GAP_CAUSE_ALREADY_IN_REQ);
		return;
	}
	for(int i = 0; i < BT_PROXY_GATT_MAX_LINKS && !l; i++)
	{
		if(g_links[i].state == LINK_FREE) l = &g_links[i];
	}
	if(!l)
	{
		ESPHome_API_BT_OnConnection(c->address, false, 0, GAP_CAUSE_CONN_LIMIT);
		return;
	}

	link_reset(l);
	l->address = c->address;
	addr_to_bd(c->address, l->bd);
	l->bd_type = c->addr_type ? GAP_REMOTE_ADDR_LE_RANDOM : GAP_REMOTE_ADDR_LE_PUBLIC;
	l->state = LINK_CONNECTING;
	l->state_ms = g_timeMs;

	// Scanning and initiating compete for the same radio time; pause the scan.
	hal_bt_scan_apply();

	T_GAP_LE_CONN_REQ_PARAM p;
	p.scan_interval = 0x60;		// 60 ms
	p.scan_window = 0x50;		// 50 ms
	p.conn_interval_min = 0x18;	// 30 ms
	p.conn_interval_max = 0x28;	// 50 ms
	p.conn_latency = 0;
	p.supv_tout = 500;			// 5 s
	p.ce_len_min = 2 * (p.conn_interval_min - 1);
	p.ce_len_max = 2 * (p.conn_interval_max - 1);
	le_set_conn_param(GAP_CONN_PARAM_1M, &p);

	T_GAP_CAUSE cause = le_connect(GAP_PHYS_CONN_INIT_1M_BIT, l->bd, (T_GAP_REMOTE_ADDR_TYPE)l->bd_type,
		GAP_LOCAL_ADDR_LE_PUBLIC, 1000);
	ADDLOG_INFO(LOG_FEATURE_GENERAL, "BTProxy: connect %02X:%02X:%02X:%02X:%02X:%02X type %d -> cause %d",
		l->bd[5], l->bd[4], l->bd[3], l->bd[2], l->bd[1], l->bd[0], l->bd_type, cause);
	if(cause != GAP_CAUSE_SUCCESS)
	{
		uint64_t address = l->address;
		link_reset(l);
		hal_bt_scan_apply();
		ESPHome_API_BT_OnConnection(address, false, 0, cause ? cause : 0xFF);
	}
	link_update_led();
	ESPHome_API_BT_OnConnectionsFreeChanged();
}

static void gatt_do_disconnect(gatt_link_t* l)
{
	l->user_disconnect = true;
	if(l->conn_id != 0xFF && l->state != LINK_DISCONNECTING)
	{
		l->state = LINK_DISCONNECTING;
		l->state_ms = g_timeMs;
		le_disconnect(l->conn_id);
	}
}

static volatile uint32_t g_last_tick_ms = 0;
static volatile uint32_t g_ticks_posted = 0;

static void gatt_on_tick(void)
{
	g_last_tick_ms = g_timeMs;
	for(int i = 0; i < BT_PROXY_GATT_MAX_LINKS; i++)
	{
		gatt_link_t* l = &g_links[i];
		uint32_t age = g_timeMs - l->state_ms;
		switch(l->state)
		{
			case LINK_CONNECTING:
				if(age > CONNECT_TIMEOUT_MS)
				{
					ADDLOG_ERROR(LOG_FEATURE_GENERAL, "BTProxy: connect timeout");
					if(l->conn_id != 0xFF)
					{
						l->user_disconnect = false;
						le_disconnect(l->conn_id);
						l->state_ms = g_timeMs; // wait for DISCONNECTED
					}
					else
					{
						uint64_t address = l->address;
						link_reset(l);
						ESPHome_API_BT_OnConnection(address, false, 0, 0x08);
						ESPHome_API_BT_OnConnectionsFreeChanged();
						hal_bt_scan_apply();
					}
				}
				break;
			case LINK_WAIT_MTU:
				if(age > MTU_WAIT_MS)
				{
					uint16_t mtu = 23;
					if(le_get_conn_param(GAP_PARAM_CONN_MTU_SIZE, &mtu, l->conn_id) == GAP_CAUSE_SUCCESS && mtu >= 23)
					{
						l->mtu = mtu;
					}
					link_report_connected(l);
				}
				break;
			case LINK_CONNECTED:
				if(l->active && (g_timeMs - l->active_ms) > OP_TIMEOUT_MS)
				{
					ADDLOG_ERROR(LOG_FEATURE_GENERAL, "BTProxy: GATT op %d timed out", l->active->type);
					if(l->active->type == GATT_CMD_SERVICES)
					{
						g_disc.link = -1;
						ESPHome_API_BT_OnError(l->address, 0, OBK_ATT_ERR_UNLIKELY);
					}
					else
					{
						ESPHome_API_BT_OnError(l->address, l->active->handle, OBK_ATT_ERR_UNLIKELY);
					}
					cmd_free(l->active);
					l->active = NULL;
				}
				link_kick(l); // also retries ops that hit a busy stack
				break;
			case LINK_DISCONNECTING:
				if(age > CONNECT_TIMEOUT_MS)
				{
					uint64_t address = l->address;
					link_reset(l);
					ESPHome_API_BT_OnConnection(address, false, 0, 0);
					ESPHome_API_BT_OnConnectionsFreeChanged();
				}
				break;
			default:
				break;
		}
	}
	link_update_led();
	// Resuming the scan right after a link comes up can fail with
	// GAP_CAUSE_INVALID_STATE while the controller settles; retry every tick.
	hal_bt_scan_apply();
}

static void gatt_handle_cmd(gatt_cmd_t* c)
{
	bool keep = false;
	switch(c->type)
	{
		case GATT_CMD_CONNECT:
			gatt_do_connect(c);
			break;
		case GATT_CMD_DISCONNECT:
		{
			gatt_link_t* l = link_by_addr(c->address);
			if(l) gatt_do_disconnect(l);
			else ESPHome_API_BT_OnConnection(c->address, false, 0, 0);
			break;
		}
		case GATT_CMD_DISCONNECT_ALL:
			for(int i = 0; i < BT_PROXY_GATT_MAX_LINKS; i++)
			{
				if(g_links[i].state != LINK_FREE) gatt_do_disconnect(&g_links[i]);
			}
			break;
		case GATT_CMD_NOTIFY:
			// With REMOTE_CACHING, HA writes the CCCD itself; GCS forwards every
			// notification it receives, so there is nothing to register here.
			if(link_by_addr(c->address)) ESPHome_API_BT_OnNotifyAck(c->address, c->handle);
			else ESPHome_API_BT_OnError(c->address, c->handle, OBK_ATT_ERR_UNLIKELY);
			break;
		case GATT_CMD_SERVICES:
		case GATT_CMD_READ:
		case GATT_CMD_WRITE:
		{
			gatt_link_t* l = link_by_addr(c->address);
			if(!l || l->state != LINK_CONNECTED || !link_push(l, c))
			{
				ESPHome_API_BT_OnError(c->address, c->handle, OBK_ATT_ERR_UNLIKELY);
				break;
			}
			keep = true;
			link_kick(l);
			break;
		}
		case GATT_CMD_TICK:
			gatt_on_tick();
			break;
	}
	if(!keep) cmd_free(c);
}

static bool gatt_post(gatt_cmd_t* c)
{
	if(!c) return false;
	if(!g_bt_proxy.stack_ready || !_ioQueueHandle || !_evtQueueHandle)
	{
		cmd_free(c);
		return false;
	}
	T_IO_MSG msg;
	msg.type = IO_MSG_TYPE_OBK_GATT;
	msg.subtype = 0;
	msg.u.buf = c;
	if(os_msg_send(_ioQueueHandle, &msg, 0) == false)
	{
		cmd_free(c);
		return false;
	}
	uint8_t event = EVENT_IO_TO_APP;
	os_msg_send(_evtQueueHandle, &event, 0);
	return true;
}

static gatt_cmd_t* gatt_cmd_new(uint8_t type, uint64_t address, uint16_t handle, uint16_t len)
{
	gatt_cmd_t* c = os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(gatt_cmd_t) + len);
	if(!c) return NULL;
	c->type = type;
	c->address = address;
	c->handle = handle;
	c->len = len;
	return c;
}

bool HAL_BTProxy_GattConnect(uint64_t address, uint8_t address_type)
{
	gatt_cmd_t* c = gatt_cmd_new(GATT_CMD_CONNECT, address, 0, 0);
	if(c) c->addr_type = address_type;
	return gatt_post(c);
}

bool HAL_BTProxy_GattDisconnect(uint64_t address)
{
	return gatt_post(gatt_cmd_new(GATT_CMD_DISCONNECT, address, 0, 0));
}

bool HAL_BTProxy_GattGetServices(uint64_t address)
{
	return gatt_post(gatt_cmd_new(GATT_CMD_SERVICES, address, 0, 0));
}

bool HAL_BTProxy_GattRead(uint64_t address, uint16_t handle)
{
	return gatt_post(gatt_cmd_new(GATT_CMD_READ, address, handle, 0));
}

bool HAL_BTProxy_GattWrite(uint64_t address, uint16_t handle, bool response, const uint8_t* data, uint16_t len)
{
	if(len > 512) return false;
	gatt_cmd_t* c = gatt_cmd_new(GATT_CMD_WRITE, address, handle, len);
	if(!c) return false;
	c->response = response;
	if(len && data) memcpy(c->data, data, len);
	return gatt_post(c);
}

bool HAL_BTProxy_GattNotify(uint64_t address, uint16_t handle, bool enable)
{
	(void)enable;
	return gatt_post(gatt_cmd_new(GATT_CMD_NOTIFY, address, handle, 0));
}

void HAL_BTProxy_GattDisconnectAll(void)
{
	gatt_post(gatt_cmd_new(GATT_CMD_DISCONNECT_ALL, 0, 0, 0));
}

int HAL_BTProxy_GattGetAllocated(uint64_t* allocated, int max, int* limit)
{
	int n = 0;
	for(int i = 0; i < BT_PROXY_GATT_MAX_LINKS; i++)
	{
		if(g_links[i].state != LINK_FREE && n < max) allocated[n++] = g_links[i].address;
	}
	if(limit) *limit = BT_PROXY_GATT_MAX_LINKS;
	return n;
}

int HAL_BTProxy_GattGetLinkState(void)
{
	return g_link_led_state;
}

// Diagnostics (BTProxyStatus). Reads BT-task state without locking; values may
// be slightly stale but this never blocks.
void HAL_BTProxy_LogStatus(void)
{
	ADDLOG_INFO(LOG_FEATURE_GENERAL, "BTProxy: stack_ready %d scan want %d running %d paused %d pkts %d drop %d",
		g_bt_proxy.stack_ready, g_bt_proxy.scan_active, g_bt_proxy.scan_running, g_bt_proxy.scan_paused,
		g_bt_proxy.scan_total_packets, g_bt_proxy.scan_dropped_packets);
	ADDLOG_INFO(LOG_FEATURE_GENERAL, "BTProxy: BT task last tick %d ms ago (posted %d), disc ws %d",
		(int)(g_timeMs - g_last_tick_ms), (int)g_ticks_posted, g_disc.link);
	for(int i = 0; i < BT_PROXY_GATT_MAX_LINKS; i++)
	{
		gatt_link_t* l = &g_links[i];
		ADDLOG_INFO(LOG_FEATURE_GENERAL, "BTProxy: link %d state %d conn %d addr %08X%04X mtu %d q %d active %d age %d",
			i, l->state, l->conn_id, (unsigned)(l->address >> 16), (unsigned)(l->address & 0xFFFF), l->mtu,
			l->q_count, l->active ? l->active->type : 0, (int)(g_timeMs - l->state_ms));
	}
}

static void gatt_handle_gap_msg(T_IO_MSG* io)
{
	T_LE_GAP_MSG gap_msg;
	memcpy(&gap_msg, &io->u.param, sizeof(io->u.param));
	switch(io->subtype)
	{
		case GAP_MSG_LE_CONN_STATE_CHANGE:
			gatt_on_conn_state(gap_msg.msg_data.gap_conn_state_change.conn_id,
				gap_msg.msg_data.gap_conn_state_change.new_state,
				gap_msg.msg_data.gap_conn_state_change.disc_cause);
			break;
		case GAP_MSG_LE_CONN_MTU_INFO:
			gatt_on_mtu(gap_msg.msg_data.gap_conn_mtu_info.conn_id, gap_msg.msg_data.gap_conn_mtu_info.mtu_size);
			break;
		default:
			break;
	}
}
#endif // ENABLE_BT_PROXY_GATT

// ===========================================================================
// Scanning
// ===========================================================================

static T_APP_RESULT hal_bt_gap_callback(uint8_t cb_type, void* p_cb_data)
{
	T_LE_CB_DATA* p_data = (T_LE_CB_DATA*)p_cb_data;
	if(!p_data) return APP_RESULT_SUCCESS;

	if(cb_type == GAP_MSG_LE_SCAN_INFO && p_data->p_le_scan_info)
	{
		T_LE_SCAN_INFO* info = p_data->p_le_scan_info;

		g_bt_proxy.scan_total_packets++;
		if(!ESPHome_API_PassScanResult(info->bd_addr, info->rssi, info->remote_addr_type, info->data, info->data_len))
		{
			g_bt_proxy.scan_dropped_packets++;
		}
	}
	return APP_RESULT_SUCCESS;
}

// Brings the controller's scan state in line with what is wanted:
// scanning if the user asked for it and no link is being established.
static void hal_bt_scan_apply(void)
{
	if(!g_bt_proxy.stack_ready) return;
	bool want = g_bt_proxy.scan_active;
#if ENABLE_BT_PROXY_GATT
	g_bt_proxy.scan_paused = scan_any_link_connecting();
	if(g_bt_proxy.scan_paused) want = false;
#endif
	if(want && !g_bt_proxy.scan_running)
	{
		static T_GAP_CAUSE last_err = GAP_CAUSE_SUCCESS;
		T_GAP_CAUSE cause = le_scan_start();
		if(cause == GAP_CAUSE_SUCCESS)
		{
			if(last_err != GAP_CAUSE_SUCCESS) ADDLOG_INFO(LOG_FEATURE_GENERAL, "BTProxy: scan resumed");
			g_bt_proxy.scan_running = true;
		}
		else if(cause != last_err)
		{
			ADDLOG_INFO(LOG_FEATURE_GENERAL, "BTProxy: scan start error %d (will retry)", cause);
		}
		last_err = cause;
	}
	else if(!want && g_bt_proxy.scan_running)
	{
		le_scan_stop();
		g_bt_proxy.scan_running = false;
	}
}

static void hal_bt_on_dev_state(T_GAP_DEV_STATE new_state, uint16_t cause)
{
	if(new_state.gap_init_state == GAP_INIT_STATE_STACK_READY && !g_bt_proxy.stack_ready)
	{
		g_bt_proxy.stack_ready = true;
		ADDLOG_INFO(LOG_FEATURE_GENERAL, "BTProxy: stack ready");
	}
	// keep our view of the controller in sync (e.g. scan stopped by the stack)
	if(new_state.gap_scan_state == GAP_SCAN_STATE_IDLE) g_bt_proxy.scan_running = false;
	else if(new_state.gap_scan_state == GAP_SCAN_STATE_SCANNING) g_bt_proxy.scan_running = true;
	hal_bt_scan_apply();
}

void HAL_BTProxy_StartScan()
{
	if(!g_bt_proxy.init_done) return;
	g_bt_proxy.scan_active = true;
	g_bt_proxy.scan_idle_seconds = 0;
	// Not ready yet: the DEV_STATE_CHANGE handler starts it once the stack is up.
	hal_bt_scan_apply();
}

void HAL_BTProxy_StopScan()
{
	if(!g_bt_proxy.init_done) return;
	g_bt_proxy.scan_active = false;
	hal_bt_scan_apply();
}

static void hal_btq_task(void* p_param)
{
	(void)p_param;
	uint8_t event;
	os_msg_queue_create(&_ioQueueHandle, 0x20, sizeof(T_IO_MSG));
	os_msg_queue_create(&_evtQueueHandle, 0x40, sizeof(uint8_t));
	gap_start_bt_stack(_evtQueueHandle, _ioQueueHandle, 0x20);
	while(btq_task_running)
	{
		if(os_msg_recv(_evtQueueHandle, &event, 0xFFFFFFFF) == true)
		{
			if(event == EVENT_IO_TO_APP)
			{
				T_IO_MSG io_msg;
				if(os_msg_recv(_ioQueueHandle, &io_msg, 0) == true)
				{
					if(io_msg.type == IO_MSG_TYPE_BT_STATUS)
					{
						if(io_msg.subtype == GAP_MSG_LE_DEV_STATE_CHANGE)
						{
							T_LE_GAP_MSG gap_msg;
							memcpy(&gap_msg, &io_msg.u.param, sizeof(io_msg.u.param));
							hal_bt_on_dev_state(gap_msg.msg_data.gap_dev_state_change.new_state,
								gap_msg.msg_data.gap_dev_state_change.cause);
						}
#if ENABLE_BT_PROXY_GATT
						else
						{
							gatt_handle_gap_msg(&io_msg);
						}
#endif
					}
#if ENABLE_BT_PROXY_GATT
					else if(io_msg.type == IO_MSG_TYPE_OBK_GATT)
					{
						gatt_handle_cmd((gatt_cmd_t*)io_msg.u.buf);
					}
#endif
				}
			}
			else
			{
				gap_handle_msg(event);
			}
		}
	}
	vTaskDelete(NULL);
}

static void hal_bt_task(void* p_param)
{
	(void)p_param;

	while(!(wifi_is_up(RTW_STA_INTERFACE)))
	{
		delay_ms(100);
	}

	T_GAP_DEV_STATE new_state;
	bt_trace_init();
	le_get_gap_param(GAP_PARAM_DEV_STATE, &new_state);
	if(new_state.gap_init_state != GAP_INIT_STATE_STACK_READY)
	{
		bte_init();
		gap_config_max_le_link_num(2);
		le_gap_init(2);
	}

	char* device_name = (char*)CFG_GetDeviceName();
	uint16_t appearance = GAP_GATT_APPEARANCE_UNKNOWN;

	uint8_t  scan_filter_policy = GAP_SCAN_FILTER_ANY;
	uint8_t  scan_filter_duplicate = GAP_SCAN_FILTER_DUPLICATE_DISABLE;

	le_set_gap_param(GAP_PARAM_DEVICE_NAME, GAP_DEVICE_NAME_LEN, device_name);
	le_set_gap_param(GAP_PARAM_APPEARANCE, sizeof(appearance), &appearance);

	le_scan_set_param(GAP_PARAM_SCAN_INTERVAL, sizeof(scan_interval), &scan_interval);
	le_scan_set_param(GAP_PARAM_SCAN_WINDOW, sizeof(scan_window), &scan_window);
	le_scan_set_param(GAP_PARAM_SCAN_MODE, sizeof(scan_mode), &scan_mode);

	le_scan_set_param(GAP_PARAM_SCAN_FILTER_POLICY, sizeof(scan_filter_policy),
		&scan_filter_policy);
	le_scan_set_param(GAP_PARAM_SCAN_FILTER_DUPLICATES, sizeof(scan_filter_duplicate),
		&scan_filter_duplicate);

#if ENABLE_BT_PROXY_GATT
	static bool client_registered = false;
	if(!client_registered)
	{
		client_init(1);
		g_gcs_client_id = gcs_add_client(hal_bt_gcs_callback, BT_PROXY_GATT_MAX_LINKS, GCS_DISCOV_TABLE_SIZE);
		for(int i = 0; i < BT_PROXY_GATT_MAX_LINKS; i++)
		{
			memset(&g_links[i], 0, sizeof(g_links[i]));
			g_links[i].conn_id = 0xFF;
		}
		client_registered = true;
	}
#endif

	le_register_app_cb(hal_bt_gap_callback);

	// Larger stack than the scan-only build: GATT result encoding and the
	// advertisement forwarder both run in this task.
	xTaskCreate(hal_btq_task, "HAL_BTQ_Task", 2048, NULL, 1, &g_bt_proxy.task_handle);

	bt_coex_init();
	do
	{
		os_delay(100);
		le_get_gap_param(GAP_PARAM_DEV_STATE, &new_state);
	} while(new_state.gap_init_state != GAP_INIT_STATE_STACK_READY);
	wifi_btcoex_set_bt_on();
	delay_ms(50);
	vTaskDelete(NULL);
}

void HAL_BTProxy_Init(void)
{
	if(g_bt_proxy.init_done)
	{
		le_register_app_cb(hal_bt_gap_callback);
		return;
	}
	btq_task_running = true;
	xTaskCreate(hal_bt_task, "HAL_BT_Task", 1024, NULL, 1, NULL);


	g_bt_proxy.init_done = true;
}

void HAL_BTProxy_Deinit(void)
{
	if(!g_bt_proxy.init_done) return;

	HAL_BTProxy_StopScan();
#if ENABLE_BT_PROXY_GATT
	HAL_BTProxy_GattDisconnectAll();
#endif
	//btq_task_running = false;
	le_register_app_cb(NULL);

	//bte_deinit(); // causes crash
	//bt_trace_uninit();

	//g_bt_proxy.init_done = false;
}

void HAL_BTProxy_GetMAC(uint8_t* mac)
{
	bt_get_mac_address(mac);
	uint8_t zero_mac[6] = { 0 };
	uint8_t ff_mac[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
	if(memcmp(mac, zero_mac, 6) == 0 || memcmp(mac, ff_mac, 6) == 0)
	{
		WiFI_GetMacAddress((char*)mac);
	}
}

void HAL_BTProxy_SetScanMode(bool isActive)
{
	scan_mode = isActive == true ? GAP_SCAN_MODE_ACTIVE : GAP_SCAN_MODE_PASSIVE;
	// Before the stack is ready, just remember it: hal_bt_task applies scan_mode
	// during init. The new mode takes effect the next time scanning starts.
	if(g_bt_proxy.stack_ready)
	{
		bool was_running = g_bt_proxy.scan_running;
		if(was_running)
		{
			le_scan_stop();
			g_bt_proxy.scan_running = false;
		}
		le_scan_set_param(GAP_PARAM_SCAN_MODE, sizeof(scan_mode), &scan_mode);
		if(was_running) hal_bt_scan_apply();
	}
}

bool HAL_BTProxy_GetScanMode(void)
{
	return scan_mode == GAP_SCAN_MODE_ACTIVE;
}

void HAL_BTProxy_SetWindowInterval(uint16_t window, uint16_t interval)
{
	// convert from ms
	scan_interval = (uint16_t)((interval * 16) / 10);
	scan_window = (uint16_t)((window * 16) / 10);

	if(g_bt_proxy.init_done)
	{
		le_scan_set_param(GAP_PARAM_SCAN_INTERVAL, sizeof(scan_interval), &scan_interval);
		le_scan_set_param(GAP_PARAM_SCAN_WINDOW, sizeof(scan_window), &scan_window);
	}
}

bool HAL_BTProxy_IsInit(void)
{
	return g_bt_proxy.init_done;
}

void HAL_BTProxy_OnEverySecond(void)
{
	if(!g_bt_proxy.stack_ready) return;
#if ENABLE_BT_PROXY_GATT
	if(gatt_post(gatt_cmd_new(GATT_CMD_TICK, 0, 0, 0))) g_ticks_posted++;
#endif
	// Scan watchdog: the controller occasionally stops reporting; restart if
	// we've seen nothing for a minute while we should be scanning.
	if(g_bt_proxy.scan_active && !g_bt_proxy.scan_paused)
	{
		if(g_bt_proxy.scan_total_packets == g_bt_proxy.scan_last_total)
		{
			if(++g_bt_proxy.scan_idle_seconds >= 60)
			{
				ADDLOG_INFO(LOG_FEATURE_GENERAL, "BTProxy: no advertisements for 60s, restarting scan");
				g_bt_proxy.scan_idle_seconds = 0;
				le_scan_stop();
				g_bt_proxy.scan_running = false;
				hal_bt_scan_apply();
			}
		}
		else
		{
			g_bt_proxy.scan_idle_seconds = 0;
		}
	}
	g_bt_proxy.scan_last_total = g_bt_proxy.scan_total_packets;
}

int HAL_BTProxy_GetScanStats(int* init_done, int* scan_active, int* total_packets, int* dropped_packets)
{
	if(init_done) *init_done = g_bt_proxy.init_done;
	if(scan_active) *scan_active = g_bt_proxy.scan_running;
	if(total_packets) *total_packets = g_bt_proxy.scan_total_packets;
	if(dropped_packets) *dropped_packets = g_bt_proxy.scan_dropped_packets;
	return 1;
}

#endif

#endif
