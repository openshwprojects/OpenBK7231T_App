#ifdef WINDOWS
#include "Controller_Button.h"
#include "Shape.h"

CControllerButton::CControllerButton() {
	timeAfterMouseHold = 99.0f;
	openPos.set(0, 16);
	closedPos.set(0, 0);
}
void CControllerButton::rotateDegreesAround(float f, const Coord &p) {
	openPos = openPos.rotateDegreesAround(f, p);
	closedPos = closedPos.rotateDegreesAround(f, p);
}
void CControllerButton::sendEvent(int code) {
	if (code == EVE_LMB_HOLD) {
		timeAfterMouseHold = 0;
	}
}
void CControllerButton::onDrawn() {
	float speed = 2.0f;
	if (timeAfterMouseHold < 0.2f) {
		mover->moveTowards(closedPos, speed);
	}
	else {
		mover->moveTowards(openPos, speed);
	}
	timeAfterMouseHold += 0.1f;
}

#endif
