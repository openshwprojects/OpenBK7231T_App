#ifndef OBK_TAILSCALE_SYSTEM_H
#define OBK_TAILSCALE_SYSTEM_H

#include <stdint.h>

int Tailscale_SystemInit(void);
void Tailscale_SystemClose(void);
void Tailscale_SystemSetBoot(uint64_t boot);
uint64_t tsc_system_monotonic_ms(void *user);

#endif
