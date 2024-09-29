#ifdef WINDOWS
#include "Controller_Switch.h"
#include "Shape.h"
#include "Line.h"
#include "Junction.h"
#include "sim_import.h"

CControllerSwitch::CControllerSwitch(class CJunction *_a, class CJunction *_b) {
	timeAfterMouseHold = 99.0f;
	openPos.set(0, 16);
	closedPos.set(0, 0);
	a = _a;
	b = _b;
	remDist = 0;
	bPressed = false;
	bVisualPressed = false;
}
class CControllerBase *CControllerSwitch::cloneController(class CShape *origOwner, class CShape *newOwner) {
	CControllerSwitch *r = new CControllerSwitch();
	r->openPos = this->openPos;
	r->closedPos = this->closedPos;
	r->remDist = this->remDist;
	r->timeAfterMouseHold = this->timeAfterMouseHold;
	r->mover = newOwner->findShapeByName_r(this->mover->getName());
	r->a = newOwner->findShapeByName_r(this->a->getName())->asJunction();
	r->b = newOwner->findShapeByName_r(this->b->getName())->asJunction();
	return r;
}
class CJunction *CControllerSwitch::findOtherJunctionIfPassable(class CJunction *ju) {
	//float dst = mover->getPosition().dist(closedPos);
	//printf("%f\n", dst);
	if (bVisualPressed == false)
		return 0;
	if (a == ju)
		return b;
	return a;
}
void CControllerSwitch::rotateDegreesAround(float f, const Coord &p) {
	openPos = openPos.rotateDegreesAround(f, p);
	closedPos = closedPos.rotateDegreesAround(f, p);
}
void CControllerSwitch::sendEvent(int code, const class Coord &mouseOfs) {
	if (code == EVE_LMB_DOWN) {
		bPressed = !bPressed;
	}
}
void CControllerSwitch::onDrawn() {
	float dt = SIM_GetDeltaTimeSeconds();
	float speed = 160.0f * dt;
	CLine *l = dynamic_cast<CLine*>(mover);
	float tgPos;
	if (bPressed)
		tgPos = 0;
	else
		tgPos = 20;
	remDist = l->getPos2().moveMeTowards(Coord(l->getPos2().getX(), tgPos),speed);
	timeAfterMouseHold += dt;
	bVisualPressed = false;
	if (bPressed) {
		if(remDist <= 0.01f) {
			bVisualPressed = true;
		}
	}
}

#endif
