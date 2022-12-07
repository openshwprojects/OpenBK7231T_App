#ifdef WINDOWS
#include "Controller_Pot.h"
#include "Shape.h"
#include "Junction.h"

CControllerPot::CControllerPot(class CJunction *_a, class CJunction *_b, class CJunction *_o) {
	a = _a;
	b = _b;
	o = _o;
	frac = 0.5f;
	posA.set(-180, -60);
	posB.set(160, -60);
}
class CControllerBase *CControllerPot::cloneController(class CShape *origOwner, class CShape *newOwner) {
	CControllerPot *r = new CControllerPot();
	r->posA = posA;
	r->posB = posB;
	r->frac = frac;
	r->mover = newOwner->findShapeByName_r(this->mover->getName());
	r->a = newOwner->findShapeByName_r(this->a->getName())->asJunction();
	r->b = newOwner->findShapeByName_r(this->b->getName())->asJunction();
	r->o = newOwner->findShapeByName_r(this->o->getName())->asJunction();
	return r;
}
class CJunction *CControllerPot::findOtherJunctionIfPassable(class CJunction *ju) {

	return 0;
}
void CControllerPot::rotateDegreesAround(float f, const Coord &p) {
	posA = posA.rotateDegreesAround(f, p);
	posB = posB.rotateDegreesAround(f, p);
}
void CControllerPot::sendEvent(int code, const class Coord &mouseOfs) {
	// could be way better with some math
	frac += mouseOfs.getX()*0.005f;

	if (frac < 0)
		frac = 0;
	if (frac > 1)
		frac = 1;
}
void CControllerPot::onDrawn() {
	mover->setPosition(posA.lerp(posB, frac));
}

#endif
