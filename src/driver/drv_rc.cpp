
#include "../obk_config.h"

#if ENABLE_DRIVER_RC

extern "C" {
	// these cause error: conflicting declaration of 'int bk_wlan_mcu_suppress_and_sleep(unsigned int)' with 'C' linkage
#include "../new_common.h"
#include "../httpserver/new_http.h"
#include "../logging/logging.h"
#include "../new_pins.h"
#include "../new_cfg.h"

}

#include "drv_rc.h"
#include "../libraries/rc-switch/src/RCSwitch.h"

RCSwitch mySwitch = RCSwitch();
void DRV_RC_Init() {
	// allow user to change them
	int pin = -1;
	bool pup = true;
	pin = PIN_FindPinIndexForRole(IOR_RCRecv, pin);
	if (pin == -1)
	{
		pin = PIN_FindPinIndexForRole(IOR_RCRecv_nPup, pin);
		if (pin >= 0)
			pup = false;
	}
	ADDLOG_INFO(LOG_FEATURE_IR, "DRV_RC_Init: passing pin %i\n", pin);
	mySwitch.enableReceive(pin, pup);  // Receiver on interrupt 0 => that is pin #2
}
extern long g_micros;
extern int rc_triggers;
extern int g_rcpin;
extern int rc_checkedProtocols;
extern int rc_singleRepeats;
extern int rc_repeats;

void RC_AppendInformationToHTTPIndexPage(http_request_t *request, int bPreState) {

	if (bPreState) {
	}
	else {
		hprintf255(request, "<h3>Triggers: %i</h3>", (int)rc_triggers);
		hprintf255(request, "<h3>Micros: %i</h3>", (int)g_micros);
		hprintf255(request, "<h3>g_rcpin: %i</h3>", (int)g_rcpin);
		hprintf255(request, "<h3>rc_checkedProtocols: %i</h3>", (int)rc_checkedProtocols);
		hprintf255(request, "<h3>rc_singleRepeats: %i</h3>", (int)rc_singleRepeats);
		hprintf255(request, "<h3>rc_repeats: %i</h3>", (int)rc_repeats);
	}
}
void DRV_RC_RunFrame() {

	if (mySwitch.available()) {

		ADDLOG_INFO(LOG_FEATURE_IR, "Received %lu / %u bit protocol %u\n",
			(unsigned long)mySwitch.getReceivedValue(),
			mySwitch.getReceivedBitlength(),
			mySwitch.getReceivedProtocol());
		// TODO 64 bit
		// addEventHandler RC 1234 toggleChannel 5 123
		EventHandlers_FireEvent(CMD_EVENT_RC, mySwitch.getReceivedValue());


		mySwitch.resetAvailable();
	}
}


#endif


