#include "../new_common.h"
#if ENABLE_DRIVER_TAILSCALE
#include "tsc_system.h"
#include "../driver/drv_ntp.h"
#include "../driver/drv_deviceclock.h"
#include "tailscalec/crypto.h"
#include "tailscalec/socket.h"
#include "drv_model_pub.h"
#include "irda_pub.h"
#include "mbedtls/hmac_drbg.h"
#include "mbedtls/platform.h"

/* All providers are called by the single Tailscale worker. The SDK's
 * BkRandomNumberRead ignores driver failure and fills at most four bytes, so
 * read the TRNG directly through its registered driver and seed a DRBG. */
static mbedtls_hmac_drbg_context random_state;
static int random_ready;
static uint32_t previous_word;
static int have_word;
static uint64_t boot_epoch;
static uint32_t handshake_serial;

void Tailscale_SystemSetBoot(uint64_t boot)
{
    boot_epoch = boot;
    handshake_serial = 0;
}

static int entropy(void *user, unsigned char *out, size_t n)
{
    (void)user;
    while (n) {
        uint32_t word = 0;
        if (sddev_control(IRDA_DEV_NAME, TRNG_CMD_GET, &word) != 0 ||
            (have_word && previous_word == word)) return -1;
        previous_word = word;
        have_word = 1;
        size_t take = n < sizeof(word) ? n : sizeof(word);
        memcpy(out, &word, take);
        out += take;
        n -= take;
    }
    return 0;
}

int tsc_system_random_bytes(void *user, void *out, size_t n)
{
    (void)user;
    if ((!out && n) || !random_ready) return TSC_ERR_IO;
    while (n) {
        size_t take = n < 256 ? n : 256;
        if (mbedtls_hmac_drbg_random(&random_state, out, take)) return TSC_ERR_IO;
        out = (unsigned char *)out + take;
        n -= take;
    }
    return TSC_OK;
}

uint64_t tsc_system_monotonic_ms(void *user)
{
    static uint32_t previous;
    static uint64_t elapsed;
    uint32_t now = rtos_get_time();
    (void)user;
    elapsed += (uint32_t)(now - previous);
    previous = now;
    return elapsed;
}

static time_t utc_now(time_t *out)
{
    time_t now = (time_t)TIME_GetCurrentTimeWithoutOffset();
    if (out) *out = now;
    return now;
}

int Tailscale_SystemInit(void)
{
    static const unsigned char personal[] = "OpenBeken tailscaleC";
    mbedtls_hmac_drbg_init(&random_state);
    random_ready = 0;
    if (mbedtls_hmac_drbg_seed(&random_state,
        mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), entropy, NULL,
        personal, sizeof(personal) - 1)) {
        mbedtls_hmac_drbg_free(&random_state);
        return TSC_ERR_IO;
    }
    random_ready = 1;
#if TSC_ENABLE_TLS
    mbedtls_platform_set_time(utc_now);
#endif
    return TSC_OK;
}

void Tailscale_SystemClose(void)
{
    random_ready = 0;
    mbedtls_hmac_drbg_free(&random_state);
}

int tsc_socket_tai64n_now(void *user, uint8_t out[12])
{
    (void)user;
    /* WireGuard needs an increasing value, not UTC. The identity's boot epoch
     * is committed and read back before use; this survives NTP rollback and
     * multiple restarts in the same wall-clock second. */
    if (!out || !boot_epoch || boot_epoch > UINT64_MAX - UINT64_C(0x400000000000000a) ||
        handshake_serial >= 999999999) return TSC_ERR_IO;
    uint64_t now = boot_epoch + UINT64_C(0x400000000000000a);
    uint32_t serial = ++handshake_serial;
    for (unsigned i = 0; i < 8; ++i) out[i] = (uint8_t)(now >> (56 - 8 * i));
    for (unsigned i = 0; i < 4; ++i) out[8 + i] = (uint8_t)(serial >> (24 - 8 * i));
    return TSC_OK;
}
#endif
