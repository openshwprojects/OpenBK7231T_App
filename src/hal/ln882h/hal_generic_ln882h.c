#ifdef PLATFORM_LN882H

#include "utils/reboot_trace/reboot_trace.h"

void HAL_RebootModule() {
    ln_chip_reboot();
}

#endif // PLATFORM_LN882H