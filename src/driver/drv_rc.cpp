
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
	mySwitch.enableReceive(0);  // Receiver on interrupt 0 => that is pin #2
}
void DRV_RC_RunFrame() {

	if (mySwitch.available()) {

		Serial.print("Received ");
		Serial.print(mySwitch.getReceivedValue());
		Serial.print(" / ");
		Serial.print(mySwitch.getReceivedBitlength());
		Serial.print("bit ");
		Serial.print("Protocol: ");
		Serial.println(mySwitch.getReceivedProtocol());

		mySwitch.resetAvailable();
	}
}


#endif


