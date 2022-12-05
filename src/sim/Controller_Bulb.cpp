#ifdef WINDOWS
#include "Controller_Bulb.h"
#include "Shape.h"
#include "Junction.h"


void CControllerBulb::setShapesActive(bool b) {
	for (int i = 0; i < shapes.size(); i++) {
		shapes[i]->setActive(b);
	}
}

void CControllerBulb::setShapesFillColor(const CColor &c) {
	for (int i = 0; i < shapes.size(); i++) {
		shapes[i]->setFillColor(c);
	}
}
void CControllerBulb::onDrawn() {
	switch (mode) {
		case BM_BULB:
		{
			setShapesFillColor(CColor(1.0f,1.0f,0.0f));
			if (a && b) {
				if (a->shouldLightUpBulb(b)) {
					setShapesActive(true);
				}
				else {
					setShapesActive(false);
				}
			}
		}
		break;
		case BM_CW:
		{
			if (gnd->getVisitCount() == 0)
			{
				setShapesActive(false);
				return;
			}
			bool bInv = false;
			if (gnd->getVoltage() > 1.5f) {
				bInv = true;
			}
			float frac_c = 0;
			float frac_w = 0;
			if (cool->getVisitCount()) {
				if (cool->getVoltage() > 1.5f) {
					frac_c = cool->getDutyRange01();
				}
			}
			if (warm->getVisitCount()) {
				if (warm->getVoltage() > 1.5f) {
					frac_w = warm->getDutyRange01();
				}
			}
			CColor col_cool(254, 160, 3);
			CColor col_warm(167, 209, 253);
			CColor fin = col_cool * frac_c + col_warm * frac_w;

			setShapesActive(true);
			setShapesFillColor(fin);
		}
		break;
	}
}
class CControllerBase *CControllerBulb::cloneController(class CShape *origOwner, class CShape *newOwner) {
	CControllerBulb *r = new CControllerBulb();
	for (int i = 0; i < shapes.size(); i++) {
		CShape *s = shapes[i];
		const char *searchName = s->getName();
		class CShape *newShape = newOwner->findShapeByName_r(searchName);
		r->addShape(newShape);
	}
	if (a) {
		r->a = newOwner->findShapeByName_r(a->getName())->asJunction();
	}
	if (b) {
		r->b = newOwner->findShapeByName_r(b->getName())->asJunction();
	}
	if (gnd) {
		r->gnd = newOwner->findShapeByName_r(gnd->getName())->asJunction();
	}
	if (cool) {
		r->cool = newOwner->findShapeByName_r(cool->getName())->asJunction();
	}
	if (warm) {
		r->warm = newOwner->findShapeByName_r(warm->getName())->asJunction();
	}
	if (red) {
		r->red = newOwner->findShapeByName_r(red->getName())->asJunction();
	}
	if (green) {
		r->green = newOwner->findShapeByName_r(green->getName())->asJunction();
	}
	if (blue) {
		r->blue = newOwner->findShapeByName_r(blue->getName())->asJunction();
	}
	r->mode = mode;
	return r;
}

#endif
