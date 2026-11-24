#include "../new_common.h"
#if ENABLE_DRIVER_TAILSCALE
#include "drv_tailscale.h"
#include "tsc_system.h"
#include "../new_cfg.h"
#include "../hal/hal_wifi.h"
#include "../cmnds/cmd_public.h"
#include "../driver/drv_deviceclock.h"
#include "../logging/logging.h"
#include "../littlefs/our_lfs.h"
#include "easyflash.h"
#include "tailscalec/client.h"
#include "tailscalec/socket.h"
#include "tailscalec/stun.h"
#include "tailscalec/crypto.h"
#include "lwip/netif.h"
#include "lwip/netifapi.h"
#include "lwip/tcpip.h"
#include "lwip/pbuf.h"
#include "lwip/ip4_addr.h"

extern int Main_HasWiFiConnected(void);

#define TX_QUEUE_SIZE 2
/* Hardware join/map/WireGuard peak was 3552 bytes on BK7231N. */
#define TAILSCALE_STACK_SIZE 6144
#define IDENTITY_MAGIC 0x54534331u
typedef struct {
    uint32_t magic;
    uint32_t version;
    uint64_t boot;
    tsc_identity identity;
} saved_identity;
typedef struct {
    tsc_client client;
    tsc_socket outer;
    tsc_endpoint endpoint;
    tsc_endpoint advertised[2];
    uint8_t advertised_types[2];
    size_t advertised_count;
    saved_identity saved;
    char url[256], auth[256], hostname[64], ca_path[64];
    char stun_host[128];
    uint16_t stun_port;
    char *ca;
    uint8_t server_key[32];
    int pinned;
    uint8_t packet[TSC_MTU];
} worker_state;

static beken_mutex_t mutex;
static int initialized, running, stopping, netif_ready;
static xTaskHandle thread;
static struct netif tunnel;
static struct pbuf *queue[TX_QUEUE_SIZE];
static unsigned queue_head, queue_count;
static unsigned long rx_packets, tx_packets, drops;
static unsigned worker_stack_free;
static char status[96] = "stopped";

static void set_status(const char *text)
{
    rtos_lock_mutex(&mutex);
    snprintf(status, sizeof(status), "%s", text);
    rtos_unlock_mutex(&mutex);
    ADDLOG_INFO(LOG_FEATURE_DRV, "Tailscale: %s", text);
}
static int should_stop(void)
{
    rtos_lock_mutex(&mutex);
    int result = stopping;
    rtos_unlock_mutex(&mutex);
    return result;
}
static void sample_stack(void)
{
    unsigned available = uxTaskGetStackHighWaterMark(NULL) * sizeof(StackType_t);
    rtos_lock_mutex(&mutex);
    worker_stack_free = available;
    rtos_unlock_mutex(&mutex);
}
static void pause_retry(void)
{
    for (unsigned i = 0; i < 50 && !should_stop(); ++i) rtos_delay_milliseconds(100);
}

/* Called by native lwIP while holding its core lock. Copy, rather than retain
 * the caller's pbuf: TCP may reuse its headers after this callback returns. */
static err_t tunnel_output(struct netif *netif, struct pbuf *p, const ip4_addr_t *dst)
{
    (void)netif; (void)dst;
    if (p->tot_len > TSC_MTU) return ERR_BUF;
    rtos_lock_mutex(&mutex);
    if (!netif_ready || queue_count == TX_QUEUE_SIZE) {
        ++drops;
        rtos_unlock_mutex(&mutex);
        return ERR_MEM;
    }
    struct pbuf *copy = pbuf_alloc(PBUF_RAW, p->tot_len, PBUF_RAM);
    if (!copy || pbuf_copy(copy, p) != ERR_OK) {
        if (copy) pbuf_free(copy);
        ++drops;
        rtos_unlock_mutex(&mutex);
        return ERR_MEM;
    }
    queue[(queue_head + queue_count) % TX_QUEUE_SIZE] = copy;
    ++queue_count;
    rtos_unlock_mutex(&mutex);
    return ERR_OK;
}
static err_t tunnel_init(struct netif *n)
{
    n->name[0] = 't'; n->name[1] = 's';
    n->mtu = TSC_MTU;
    n->output = tunnel_output;
    n->flags = 0; /* Layer 3: no Ethernet framing, ARP or broadcast. */
    return ERR_OK;
}
static int attach_netif(const uint8_t address[4])
{
    ip4_addr_t ip, mask, gateway;
    memcpy(&ip.addr, address, 4);
    IP4_ADDR(&mask, 255, 192, 0, 0);
    ip4_addr_set_zero(&gateway);
    err_t result = netifapi_netif_add(&tunnel, &ip, &mask, &gateway, NULL, tunnel_init, tcpip_input);
    if (result == ERR_OK) {
        rtos_lock_mutex(&mutex);
        netif_ready = 1;
        rtos_unlock_mutex(&mutex);
        while (netifapi_netif_set_link_up(&tunnel) == ERR_MEM) rtos_delay_milliseconds(10);
        while (netifapi_netif_set_up(&tunnel) == ERR_MEM) rtos_delay_milliseconds(10);
    }
    return result == ERR_OK ? TSC_OK : TSC_ERR_IO;
}
static void detach_netif(void)
{
    rtos_lock_mutex(&mutex);
    int attached = netif_ready;
    netif_ready = 0;
    rtos_unlock_mutex(&mutex);
    if (attached) {
        while (netifapi_netif_set_down(&tunnel) == ERR_MEM) rtos_delay_milliseconds(10);
        while (netifapi_netif_set_link_down(&tunnel) == ERR_MEM) rtos_delay_milliseconds(10);
        while (netifapi_netif_remove(&tunnel) == ERR_MEM) rtos_delay_milliseconds(10);
    }
    rtos_lock_mutex(&mutex);
    while (queue_count) {
        pbuf_free(queue[queue_head]);
        queue_head = (queue_head + 1) % TX_QUEUE_SIZE;
        --queue_count;
    }
    rtos_unlock_mutex(&mutex);
}
static void receive_ip(void *user, const void *packet, size_t n)
{
    (void)user;
    struct pbuf *p = pbuf_alloc(PBUF_RAW, (u16_t)n, PBUF_RAM);
    if (!p) return;
    if (pbuf_take(p, packet, n) != ERR_OK || tcpip_input(p, &tunnel) != ERR_OK) {
        pbuf_free(p);
        return;
    }
    rtos_lock_mutex(&mutex);
    ++rx_packets;
    rtos_unlock_mutex(&mutex);
}

static int hex_key(const char *s, uint8_t out[32])
{
    if (strlen(s) != 64) return 0;
    for (unsigned i = 0; i < 32; ++i) {
        unsigned byte = 0;
        for (unsigned j = 0; j < 2; ++j) {
            unsigned char c = (unsigned char)*s++;
            unsigned v = c >= '0' && c <= '9' ? c - '0' :
                c >= 'a' && c <= 'f' ? c - 'a' + 10 :
                c >= 'A' && c <= 'F' ? c - 'A' + 10 : 16;
            if (v > 15) return 0;
            byte = (byte << 4) | v;
        }
        out[i] = (uint8_t)byte;
    }
    return 1;
}
static char *read_bounded_file(const char *path, size_t maximum)
{
    struct lfs_info info;
    if (!lfs_present() || lfs_stat(&lfs, path, &info) ||
        info.type != LFS_TYPE_REG || info.size > maximum) return NULL;
    return (char *)LFS_ReadFile(path);
}
static int read_stun(worker_state *w, char *value)
{
    char *port = strrchr(value, ':');
    w->stun_port = 3478;
    if (port) {
        *port++ = 0;
        char *end;
        unsigned long number = strtoul(port, &end, 10);
        if (end == port || *end || !number || number > 65535) return 0;
        w->stun_port = (uint16_t)number;
    }
    if (!*value || strlen(value) >= sizeof(w->stun_host)) return 0;
    strcpy(w->stun_host, value);
    return 1;
}
static int read_config(worker_state *w)
{
    char *text = read_bounded_file("tailscale.conf", 1024);
    if (!text) return 0;
    size_t text_length = strlen(text);
    int good = 1;
    snprintf(w->hostname, sizeof(w->hostname), "%s", CFG_GetShortDeviceName());
    for (char *line = text; line && *line;) {
        char *next = strchr(line, '\n');
        if (next) *next++ = 0;
        size_t len = strlen(line);
        if (len && line[len - 1] == '\r') line[len - 1] = 0;
        if (*line && *line != '#') {
            char *value = strchr(line, '=');
            if (!value) { good = 0; break; }
            *value++ = 0;
            char *dest = NULL;
            size_t capacity = 0;
            if (!strcmp(line, "url")) { dest = w->url; capacity = sizeof(w->url); }
            else if (!strcmp(line, "auth_key")) { dest = w->auth; capacity = sizeof(w->auth); }
            else if (!strcmp(line, "hostname")) { dest = w->hostname; capacity = sizeof(w->hostname); }
            else if (!strcmp(line, "ca")) { dest = w->ca_path; capacity = sizeof(w->ca_path); }
            else if (!strcmp(line, "stun")) {
                if (!read_stun(w, value)) { good = 0; break; }
            }
            else if (!strcmp(line, "server_key")) {
                w->pinned = hex_key(value, w->server_key);
                if (!w->pinned) { good = 0; break; }
            } else { good = 0; break; }
            if (dest) {
                if (strlen(value) >= capacity) { good = 0; break; }
                strcpy(dest, value);
            }
        }
        line = next;
    }
    tsc_crypto_zero(text, text_length);
    free(text);
    if (!good || !w->url[0] || !w->hostname[0]) return 0;
    if (!strncmp(w->url, "http://", 7)) return w->pinned;
    if (strncmp(w->url, "https://", 8) || !TSC_ENABLE_TLS || !w->ca_path[0]) return 0;
    w->ca = read_bounded_file(w->ca_path, 8192);
    return w->ca != NULL;
}

static int load_identity(worker_state *w)
{
    size_t saved = 0;
    size_t got = ef_get_env_blob("tsc_identity", &w->saved, sizeof(w->saved), &saved);
    if (!saved) {
        memset(&w->saved, 0, sizeof(w->saved));
        w->saved.magic = IDENTITY_MAGIC;
        w->saved.version = 1;
        if (tsc_identity_generate(&w->saved.identity)) return 0;
    } else if (got != sizeof(w->saved) || saved != sizeof(w->saved) ||
               w->saved.magic != IDENTITY_MAGIC || w->saved.version != 1) return 0;
    if (w->saved.boot == UINT64_MAX) return 0;
    ++w->saved.boot;
    if (ef_set_env_blob("tsc_identity", &w->saved, sizeof(w->saved)) != EF_NO_ERR) return 0;
    saved_identity check;
    got = ef_get_env_blob("tsc_identity", &check, sizeof(check), &saved);
    int equal = got == sizeof(check) && saved == sizeof(check) &&
        tsc_crypto_equal(&check, &w->saved, sizeof(check));
    tsc_crypto_zero(&check, sizeof(check));
    if (equal) Tailscale_SystemSetBoot(w->saved.boot);
    return equal;
}

static int connected(worker_state *w)
{
    ip4_addr_t ip;
    if (!Main_HasWiFiConnected() || !ip4addr_aton(HAL_GetMyIPString(), &ip)) return 0;
    return !memcmp(&ip.addr, w->endpoint.address, 4);
}
static void discover_public_endpoint(worker_state *w)
{
    w->advertised[0] = w->endpoint;
    w->advertised_types[0] = 1;
    w->advertised_count = 1;
    if (!w->stun_host[0]) return;
    tsc_endpoint server;
    tsc_stun_client stun;
    int rc = tsc_socket_resolve(w->stun_host, w->stun_port, TSC_AF_IPV4, &server);
    tsc_platform platform = tsc_socket_platform(&w->outer);
    if (!rc) rc = tsc_stun_start(&stun, &platform, &server);
    if (!rc) {
        /* Exclusive use of the SAME outer socket before joining WireGuard.
         * Reuse the as-yet uninitialized client's packet scratch. */
        do {
            rc = tsc_stun_poll(&stun, w->client.receive_buffer, sizeof(w->client.receive_buffer));
            if (rc == TSC_AGAIN) rtos_delay_milliseconds(10);
        } while (rc == TSC_AGAIN && !should_stop());
    }
    if (!rc && stun.mapped.family == TSC_AF_IPV4 &&
        !tsc_endpoint_equal(&stun.mapped, &w->endpoint)) {
        w->advertised[w->advertised_count] = stun.mapped;
        w->advertised_types[w->advertised_count++] = 2;
        ADDLOG_INFO(LOG_FEATURE_DRV, "Tailscale public UDP %u.%u.%u.%u:%u",
            stun.mapped.address[0], stun.mapped.address[1], stun.mapped.address[2],
            stun.mapped.address[3], stun.mapped.port);
    } else if (rc) {
        ADDLOG_INFO(LOG_FEATURE_DRV, "Tailscale STUN %d; advertising LAN endpoint", rc);
    }
}
static void worker(beken_thread_arg_t argument)
{
    (void)argument;
    worker_state *w = calloc(1, sizeof(*w));
    int rng_ready = 0;
    if (!w) { set_status("out of memory creating client"); goto finished; }
    w->outer.fd = -1;
    if (!read_config(w)) { set_status("invalid/missing tailscale.conf or CA"); goto finished; }
    if (Tailscale_SystemInit()) { set_status("hardware entropy unavailable"); goto finished; }
    rng_ready = 1;
    if (!load_identity(w)) { set_status("identity persistence failed"); goto finished; }
    while (!should_stop()) {
        if (!Main_HasWiFiConnected() || (TSC_ENABLE_TLS && !TIME_IsTimeSynced())) {
            rtos_delay_milliseconds(100);
            continue;
        }
        ip4_addr_t ip;
        if (!ip4addr_aton(HAL_GetMyIPString(), &ip)) { pause_retry(); continue; }
        w->endpoint.family = TSC_AF_IPV4;
        w->endpoint.port = 41641;
        memcpy(w->endpoint.address, &ip.addr, 4);
        tsc_endpoint bind = {.family = TSC_AF_IPV4, .port = 41641};
        const char *phase = "UDP open";
        int rc = tsc_socket_open(&w->outer, &bind);
        if (!rc) discover_public_endpoint(w);
        if (should_stop()) { tsc_socket_close(&w->outer); break; }
        tsc_client_config config = {
            .control_url = w->url, .auth_key = w->auth, .hostname = w->hostname,
            .server_public_key = w->pinned ? w->server_key : NULL,
            .ca_pem = w->ca, .ca_pem_length = w->ca ? strlen(w->ca) + 1 : 0,
            .identity = &w->saved.identity, .platform = tsc_socket_platform(&w->outer),
            .tai64n_now = tsc_socket_tai64n_now,
            .endpoints = w->advertised, .endpoint_count = w->advertised_count, .control_timeout_ms = 5000,
            .endpoint_types = w->advertised_types,
        };
        set_status("joining");
        if (!rc) { phase = "client init"; rc = tsc_client_init(&w->client, &config); }
        if (!rc) { phase = "control join"; rc = tsc_client_join(&w->client); }
        if (!rc) {
            tsc_client_set_ip_handler(&w->client, receive_ip, NULL);
            phase = "netif attach";
            rc = attach_netif(w->client.ipv4);
        }
        if (!rc) {
            char message[96];
            const tsc_control_map *map = tsc_control_get_map(&w->client.control);
            snprintf(message, sizeof(message), "online %u.%u.%u.%u, peers %u, heap %u",
                w->client.ipv4[0], w->client.ipv4[1], w->client.ipv4[2],
                w->client.ipv4[3], (unsigned)map->peer_count, (unsigned)xPortGetFreeHeapSize());
            set_status(message);
            tsc_crypto_zero(w->auth, sizeof(w->auth));
        } else {
            ADDLOG_ERROR(LOG_FEATURE_DRV, "Tailscale %s failed %d: %s", phase, rc, tsc_client_last_error(&w->client));
        }
        size_t pending = 0;
        uint64_t pending_since = 0;
        while (!rc && !should_stop() && connected(w)) {
            rc = tsc_client_poll(&w->client);
            if (rc == TSC_AGAIN || rc == TSC_ERR_PROTOCOL || rc == TSC_ERR_TRUNCATED) rc = TSC_OK;
            if (rc) break;
            if (!pending) {
                rtos_lock_mutex(&mutex);
                struct pbuf *p = queue_count ? queue[queue_head] : NULL;
                if (p) {
                    queue_head = (queue_head + 1) % TX_QUEUE_SIZE;
                    --queue_count;
                }
                rtos_unlock_mutex(&mutex);
                if (p) {
                    pending = pbuf_copy_partial(p, w->packet, p->tot_len, 0);
                    pbuf_free(p);
                    pending_since = tsc_system_monotonic_ms(NULL);
                }
            }
            if (pending) {
                int sent = tsc_client_send_ip(&w->client, w->packet, pending);
                if (sent != TSC_AGAIN || tsc_system_monotonic_ms(NULL) - pending_since > 5000) {
                    rtos_lock_mutex(&mutex);
                    if (sent == TSC_OK) ++tx_packets; else ++drops;
                    rtos_unlock_mutex(&mutex);
                    pending = 0;
                }
            }
            rc = tsc_client_maintain(&w->client, 30000);
            sample_stack();
            rtos_delay_milliseconds(10);
        }
        detach_netif();
        tsc_client_close(&w->client);
        tsc_socket_close(&w->outer);
        if (!should_stop()) { set_status("reconnecting"); pause_retry(); }
    }
    set_status("stopped");
finished:
    sample_stack();
    if (rng_ready) Tailscale_SystemClose();
    if (w) {
        free(w->ca);
        tsc_crypto_zero(w, sizeof(*w));
        free(w);
    }
    rtos_lock_mutex(&mutex);
    running = 0;
    rtos_unlock_mutex(&mutex);
    rtos_delete_thread(NULL);
}

static commandResult_t start(const void *ctx, const char *cmd, const char *args, int flags)
{
    (void)ctx; (void)cmd; (void)args; (void)flags;
    rtos_lock_mutex(&mutex);
    if (running) { rtos_unlock_mutex(&mutex); return CMD_RES_ERROR; }
    running = 1; stopping = 0;
    rtos_unlock_mutex(&mutex);
    if (rtos_create_thread(&thread, BEKEN_APPLICATION_PRIORITY, "tailscale",
        worker, TAILSCALE_STACK_SIZE, 0) != kNoErr) {
        rtos_lock_mutex(&mutex); running = 0; rtos_unlock_mutex(&mutex);
        return CMD_RES_ERROR;
    }
    return CMD_RES_OK;
}
static commandResult_t stop(const void *ctx, const char *cmd, const char *args, int flags)
{
    (void)ctx; (void)cmd; (void)args; (void)flags;
    rtos_lock_mutex(&mutex); stopping = 1; rtos_unlock_mutex(&mutex);
    return CMD_RES_OK;
}
static commandResult_t report(const void *ctx, const char *cmd, const char *args, int flags)
{
    (void)ctx; (void)cmd; (void)args; (void)flags;
    rtos_lock_mutex(&mutex);
    ADDLOG_INFO(LOG_FEATURE_DRV, "Tailscale: %s; RX %lu TX %lu drops %lu; heap %u min %u stack-free %u",
        status, rx_packets, tx_packets, drops, (unsigned)xPortGetFreeHeapSize(),
        (unsigned)xPortGetMinimumEverFreeHeapSize(), worker_stack_free);
    rtos_unlock_mutex(&mutex);
    return CMD_RES_OK;
}
void DRV_Tailscale_Init(void)
{
    if (!initialized) {
        if (rtos_init_mutex(&mutex) != kNoErr) return;
        CMD_RegisterCommand("TailscaleStart", start, NULL);
        CMD_RegisterCommand("TailscaleStop", stop, NULL);
        CMD_RegisterCommand("TailscaleStatus", report, NULL);
        initialized = 1;
    }
    start(NULL, NULL, NULL, 0);
}
void DRV_Tailscale_Stop(void)
{
    if (initialized) stop(NULL, NULL, NULL, 0);
}
#endif
