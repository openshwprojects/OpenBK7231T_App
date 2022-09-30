#if defined(PLATFORM_W800) || defined(PLATFORM_W600) 

#include "wm_include.h"

void HAL_RebootModule() {
    tls_sys_reset();
}

#endif
