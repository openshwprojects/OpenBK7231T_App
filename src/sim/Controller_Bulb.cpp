#ifdef WINDOWS
#include "Controller_Bulb.h"
#include "Shape.h"
#include "Junction.h"


void CControllerBulb::onDrawn() {
	if (a->shouldLightUpBulb(b)) {
		shape->setActive(true);
	}
	else {
		shape->setActive(false);
	}

}
class CControllerBase *CControllerBulb::cloneController(class CShape *origOwner, class CShape *newOwner) {
	CControllerBulb *r = new CControllerBulb();
	const char *searchName = this->shape->getName();
	class CShape *newShape = newOwner->findShapeByName_r(searchName);
	r->setShape(newShape);
	r->a = newOwner->findShapeByName_r(a->getName())->asJunction();
	r->b = newOwner->findShapeByName_r(b->getName())->asJunction();
	return r;
}

#endif
