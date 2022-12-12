#ifdef WINDOWS
#include "Controller_Button.h"
#include "Shape.h"
#include "Junction.h"
#include "sim_import.h"

CControllerButton::CControllerButton(class CJunction *_a, class CJunction *_b) {
	timeAfterMouseHold = 99.0f;
	openPos.set(0, 16);
	closedPos.set(0, 0);
	a = _a;
	b = _b;
	remDist = 0;
}
class CControllerBase *CControllerButton::cloneController(class CShape *origOwner, class CShape *newOwner) {
	CControllerButton *r = new CControllerButton();
	r->openPos = this->openPos;
	r->closedPos = this->closedPos;
	r->remDist = this->remDist;
	r->timeAfterMouseHold = this->timeAfterMouseHold;
	r->mover = newOwner->findShapeByName_r(this->mover->getName());
	r->a = newOwner->findShapeByName_r(this->a->getName())->asJunction();
	r->b = newOwner->findShapeByName_r(this->b->getName())->asJunction();
	return r;
}
class CJunction *CControllerButton::findOtherJunctionIfPassable(class CJunction *ju) {
	//float dst = mover->getPosition().dist(closedPos);
	//printf("%f\n", dst);
	if (remDist > 5)
		return 0;
	if (a == ju)
		return b;
	return a;
}
void CControllerButton::rotateDegreesAround(float f, const Coord &p) {
	openPos = openPos.rotateDegreesAround(f, p);
	closedPos = closedPos.rotateDegreesAround(f, p);
}
void CControllerButton::sendEvent(int code, const class Coord &mouseOfs) {
	if (code == EVE_LMB_HOLD) {
		timeAfterMouseHold = 0;
	}
}
void CControllerButton::onDrawn() {
	float dt = SIM_GetDeltaTimeSeconds();
	float speed = 160.0f * dt;
	if (timeAfterMouseHold < 0.2f) {
		remDist = mover->moveTowards(closedPos, speed);
	}
	else {
		remDist = 9999;
		mover->moveTowards(openPos, speed);
	}
	timeAfterMouseHold += dt;
}

#endif
