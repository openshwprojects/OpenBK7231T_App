#ifdef PLATFORM_BL602

#include "../../new_common.h"
#include <hal_sys.h>

void HAL_RebootModule() {

	hal_reboot();

}

#endif // PLATFORM_XR809