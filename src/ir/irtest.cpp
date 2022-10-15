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


class cpptest2 {
    public:
        int initialised;
        cpptest2(){
        	// remove else static class may kill us!!!ADDLOG_INFO(LOG_FEATURE_CMD, "Log from Class constructor");
            initialised = 42;
        };
        ~cpptest2(){
            initialised = 24;
        	ADDLOG_INFO(LOG_FEATURE_CMD, "Log from Class destructor");
        }

        void print(){
        	ADDLOG_INFO(LOG_FEATURE_CMD, "Log from Class %d", initialised);
        }
};

cpptest2 staticclass;

void cpptest(){
	ADDLOG_INFO(LOG_FEATURE_CMD, "Log from CPP");

    cpptest2 test;

    test.print();

    cpptest2 *test2 = new cpptest2();

    test2->print();

	ADDLOG_INFO(LOG_FEATURE_CMD, "Log from static class (is it initialised?):");
    staticclass.print();
}

extern "C" void testmehere(){
	ADDLOG_INFO(LOG_FEATURE_CMD, "Log from extern C CPP");
    cpptest();
}


