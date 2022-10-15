// test of c++


extern "C" {
    // these cause error: conflicting declaration of 'int bk_wlan_mcu_suppress_and_sleep(unsigned int)' with 'C' linkage
    #include "../new_pins.h"
    #include "../new_cfg.h"
    #include "../logging/logging.h"
    #include "../obk_config.h"
    #include "bk_timer_pub.h"

    #include <ctype.h>
}


void cpptest(){
	ADDLOG_INFO(LOG_FEATURE_CMD, "Log from CPP");
}

extern "C" void testmehere(){
	ADDLOG_INFO(LOG_FEATURE_CMD, "Log from extern C CPP");
    cpptest();
}


