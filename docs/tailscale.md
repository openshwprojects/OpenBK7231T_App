# Experimental Tailscale integration (BK7231N)

The `Tailscale` driver embeds [tailscaleC](https://github.com/niteria/tailscaleC)
and exposes a layer-3 interface to the SDK's existing lwIP. OpenBeken's HTTP,
REST and ordinary native sockets use the tunnel. The library's private
application TCP/UDP stacks are disabled.

This first port supports direct IPv4 connectivity on the original BK7231N SDK.
It is an opt-in experimental build with bounded peer/session storage.

## What it is useful for

[Tailscale](https://tailscale.com/) provides a private network using encrypted
WireGuard connections. Each enrolled OpenBeken device gets a tailnet IPv4
address, so authorized clients can use its existing web UI and REST API from
another network. Examples include controlling a socket in a second home or
accessing its readings while away, without forwarding the device's HTTP port
on the router. This implementation requires a working direct UDP path.

## Build

Initialize the `libraries/tailscalec` submodule on the host before using a
build container:

```sh
git submodule update --init libraries/tailscalec
make OpenBK7231N VARIANT=tailscale APP_VERSION=1.18.318-tailscale
```

The existing `CFG_USE_TAILSCALE=1` build switch is also supported. The BK7231N
prebuild initializes the dependency when the feature is enabled. Standard
builds leave the driver disabled and do not initialize this optional submodule.
Use the [normal build environment](../BUILDING.md), with support for the SDK's
32-bit GCC 4.9 toolchain. The build uses the application's mbedTLS 2.28.5 source
download and `src/tailscale/mbedtls_config.h`.

`platforms/tailscale.mk` builds a separate archive with unique object names:
the legacy SDK otherwise flattens different `crypto.c` and `chacha20.c` files
into the same object paths. `CFG_USE_MQTT_TLS=1` is rejected for this variant
until both features can share one mbedTLS configuration.

Upload the **RBL** through OpenBeken's normal OTA interface. Observe the board's
OTA/LittleFS size limits. The enabled firmware measured below was 619,520 bytes.

GitHub Actions builds this variant on pushes and pull requests as
`Build (OpenBK7231N, tailscale)`. Download the `OpenBK7231N` artifact with the
`_tailscale` suffix from the workflow run; the OTA image is
`OpenBK7231N_<version>_tailscale.rbl`.

## Configuration and startup

Upload `tailscale.conf` to LittleFS. Each non-comment line is `name=value`:

```ini
url=http://coordinator.example:8080
server_key=64_HEXADECIMAL_DIGITS_OF_THE_TRUSTED_NOISE_PUBLIC_KEY
auth_key=INITIAL_ENROLLMENT_KEY
hostname=obk-plug
stun=derp4f.tailscale.com:3478
```

HTTP coordination requires an independently trusted/pinned Noise server key.
Coordination messages are encrypted and authenticated by Noise. Obtain the key
through an authenticated channel, such as verified HTTPS key discovery; enter
the 64 hexadecimal digits without the `mkey:` prefix.

For HTTPS, use `url=https://...` and `ca=tailscale-ca.pem`, and upload the PEM
trust-root file. Certificate chain, date and hostname verification are enabled.
HTTPS is provider-tested on the host; its peak RAM has not been qualified on
BK7231N. The hardware results below used pinned Noise over HTTP.

Enable Wi-Fi and NTP, then start the driver:

```text
startDriver NTP
startDriver Tailscale
TailscaleStatus
```

The worker waits for Wi-Fi and synchronized time. After successful enrollment,
remove `auth_key` from the file. The worker clears its in-memory enrollment key
after joining; removing the stored key is an explicit configuration step.
Add `startDriver Tailscale` to the existing startup command or `autoexec.bat`
alongside the NTP startup command for automatic startup.

Other commands:

```text
TailscaleStop
TailscaleStart
stopDriver Tailscale
```

Stop is cooperative. `TailscaleStart` and `TailscaleStatus` become available
after the first `startDriver Tailscale`. Status reports tunnel packet counters,
drops, current/minimum heap and the worker's stack high-water mark.

The versioned identity is stored in EasyFlash. Each worker start commits and
reads back an incremented boot epoch before creating WireGuard timestamps.
The HMAC-DRBG is seeded from the SDK TRNG driver with error/repeated-word checks;
the adapter avoids the SDK's time-seeded `rand()` and short-read RNG wrapper.

## Peers, policy and networking

All valid, authorized, unexpired peers supplied by the coordinator are eligible
within the configured directory capacity. There is no controller allowlist.
The current library profile provides:

- Up to 32 known peers, allocated as needed; two cached authenticated sessions
  plus one probationary handshake slot.
- Replay history keyed by peer identity across eviction and map reordering.
  A cold peer must authenticate before replacing an established cached session.
- Four IPv4 endpoint candidates per peer; a rotating discovery window probes
  online peers, with cached discovery shared secrets.
- Eight filter rules, 32 sources and four destinations per rule. Unsupported
  policy constructs and exceeded bounds fail closed.
- Streaming peer decoding, a 4 KiB retained JSON document and 384 compact tokens.
  Input bounds include nesting 32, scalar length 512 and a 128 KiB wire map.
- Temporary join/refresh scratch, one decoded map and full-sized Noise receive
  records. Traffic is disabled during refresh and remains disabled on failure.
- A 1280-byte MTU, two copied native-output packet slots and one pending packet.
  Queue pressure returns `ERR_MEM` for native TCP to retry.
- One 6144-byte worker stack. Calls into the protocol library are serialized.

`stun` discovers a public endpoint on the same UDP socket before joining, and
is retried on driver/Wi-Fi reconnection. A failed probe advertises the LAN
endpoint. Periodic public-endpoint updates and DERP fallback are future work.

Clients may use their usual DNS to resolve a device name to its Tailscale IPv4
address. Device-originated split-DNS/MagicDNS lookups require separate resolver
integration.

## Hardware evidence (2026-10-23)

The prototype ran on a BK7231N/T34 socket based on OpenBeken commit
`6a8d05f942622e030b21d3452a0524a9d6be73f9`:

- Inbound native HTTP/API and the web UI worked on a real tailnet from three
  ordinary clients, including an across-apartments path. Alternating clients
  exercised eviction from the two-session cache.
- The coordinator supplied 16 usable peers; all were retained.
- Automatic startup and a reboot without an enrollment key preserved the node
  identity/address. GPIO settings and the timer file matched their earlier hashes.
- An earlier isolated Headscale test completed 35 consecutive API requests
  across map refreshes and transferred the full web UI.

| Measurement | Real-tailnet prototype |
| --- | ---: |
| RBL size | 619,520 B |
| Idle free heap | 31,816 B |
| Free heap during status HTTP | 20,120 B |
| Recorded minimum free heap | 14,344 B |
| Worker stack remaining | 2,672 B |

Transient low-heap warnings occurred without a reset in the measured run.
These are workload measurements, not worst-case guarantees. Cold/cache-
replacement checks took roughly 3–12 seconds including remote test setup.
Synchronous map refresh also caused occasional roughly one-second HTTP delays.

The pinned library passed its host ASan/UBSan suites: 17 passed/2 skipped for
the full profile, and 16 passed/1 skipped for the compact mbedTLS profile. The
cache tests include three controllers through two slots, stale initiation
replay after eviction/reordering, forged timestamp authentication and revocation.

## Follow-up work

Before treating this as a general-purpose release: qualify sustained traffic
and maximum supported maps against the existing low-memory watchdog; test
repeated reconnect/start-stop cycles; review the parser/cache security paths;
and extend runtime test coverage. Further connectivity work includes periodic
NAT remapping, DERP lifecycle/failover, refresh latency and outbound DNS.

## Library licensing

The library's project-original code uses **CC0 1.0 Universal**. Bundled and
adapted third-party code has its own licenses; see the library's `LICENSE`
and `THIRD_PARTY_NOTICES.md`.
