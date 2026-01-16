
#include "../obk_config.h"

#if ENABLE_DRIVER_RC

extern "C" {
	// these cause error: conflicting declaration of 'int bk_wlan_mcu_suppress_and_sleep(unsigned int)' with 'C' linkage
#include "../new_common.h"
#include "../httpserver/new_http.h"
#include "../logging/logging.h"
#include "../new_pins.h"
#include "../new_cfg.h"
#include "../cmnds/cmd_public.h"

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
//extern long g_micros;
//extern int rc_triggers;
//extern int g_rcpin;
//extern int rc_checkedProtocols;
//extern int rc_singleRepeats;
//extern int rc_repeats;
static int rc_totalDecoded = 0;

void RC_AppendInformationToHTTPIndexPage(http_request_t *request, int bPreState) {

	if (bPreState) {
	}
	else {
		hprintf255(request, "<h3>RC signals decoded: %i</h3>", rc_totalDecoded);
		//hprintf255(request, "<h3>Triggers: %i</h3>", (int)rc_triggers);
		//hprintf255(request, "<h3>Micros: %i</h3>", (int)g_micros);
		//hprintf255(request, "<h3>g_rcpin: %i</h3>", (int)g_rcpin);
		//hprintf255(request, "<h3>rc_checkedProtocols: %i</h3>", (int)rc_checkedProtocols);
		//hprintf255(request, "<h3>rc_singleRepeats: %i</h3>", (int)rc_singleRepeats);
		//hprintf255(request, "<h3>rc_repeats: %i</h3>", (int)rc_repeats);
	}
}
unsigned long rc_prev = 0;
int loopsUntilClear = 0;
void DRV_RC_RunFrame() {
	if (loopsUntilClear) {
		loopsUntilClear--;
		if (loopsUntilClear <= 0) {
			rc_prev = 0;
			ADDLOG_INFO(LOG_FEATURE_IR, "Clearing hold timer\n");
		}
	}
	if (mySwitch.available()) {
		rc_totalDecoded++;

		unsigned long rc_now = mySwitch.getReceivedValue();
		int bHold = 0;
		if (rc_now != rc_prev) {
			rc_prev = rc_now;
		}
		else {
			bHold = 1;
		}
		loopsUntilClear = 15;
		// TODO 64 bit
		// generic
		// addEventHandler RC 1234 toggleChannel 5 123
		// on first receive
		// addEventHandler2 RC 1234 0 toggleChannel 5 123
		// on hold
		// addEventHandler2 RC 1234 1 toggleChannel 5 123
		ADDLOG_INFO(LOG_FEATURE_IR, "Received %lu / %u bit protocol %u, hold %i\n",
			rc_now,
			mySwitch.getReceivedBitlength(),
			mySwitch.getReceivedProtocol(),
			bHold);
		EventHandlers_FireEvent2(CMD_EVENT_RC, mySwitch.getReceivedValue(), bHold);


		mySwitch.resetAvailable();
	}
}


#endif


