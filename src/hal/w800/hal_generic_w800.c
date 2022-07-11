#ifdef PLATFORM_W800

#include "wm_include.h"

void HAL_RebootModule() {
    tls_sys_reset();
}

#endif
