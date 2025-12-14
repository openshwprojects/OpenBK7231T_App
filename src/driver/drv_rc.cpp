
#include "../obk_config.h"

#if ENABLE_DRIVER_RC

extern "C" {
	// these cause error: conflicting declaration of 'int bk_wlan_mcu_suppress_and_sleep(unsigned int)' with 'C' linkage
#include "../new_common.h"

}

#include "drv_rc.h"
#include "../libraries/rc-switch/src/RCSwitch.h"

RCSwitch mySwitch = RCSwitch();
void DRV_RC_Init() {
	// allow user to change them
	int pin = 0;
	bool pup = true;
	pin = PIN_FindPinIndexForRole(IOR_IRRecv, pin);
	if (pin == -1)
	{
		pin = PIN_FindPinIndexForRole(IOR_IRRecv_nPup, pin);
		if (pin >= 0)
			pup = false;
	}
	mySwitch.enableReceive(pin, pup);  // Receiver on interrupt 0 => that is pin #2
}
void DRV_RC_RunFrame() {

	if (mySwitch.available()) {

		ADDLOG_INFO(LOG_FEATURE_IR, "Received %i / %i bit protocol %i\n",
			mySwitch.getReceivedValue(),mySwitch.getReceivedBitlength(),mySwitch.getReceivedProtocol());

		mySwitch.resetAvailable();
	}
}


#endif


