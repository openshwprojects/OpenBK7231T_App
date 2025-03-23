#ifdef WINDOWS
#include "Controller_Bulb.h"
#include "Shape.h"
#include "Junction.h"


void CControllerBulb::setShapesActive(bool b) {
	for (unsigned i = 0; i < shapes.size(); i++) {
		shapes[i]->setActive(b);
	}
}

void CControllerBulb::setShapesFillColor(const CColor &c) {
	for (unsigned int i = 0; i < shapes.size(); i++) {
		shapes[i]->setFillColor(c);
	}
}
void CControllerBulb::onDrawn() {
	CColor col_red(255, 0, 0);
	CColor col_green(0, 255, 0);
	CColor col_blue(0, 0, 255);
	CColor col_yellow(255, 255, 0);
	CColor col_cool(167, 209, 253);
	CColor col_warm(254, 160, 3);
	switch (mode) {
		case BM_BULB:
		{
			if (a->getVisitCount() == 0)
			{
				setShapesActive(false);
				return;
			}
			if (b->getVisitCount() == 0)
			{
				setShapesActive(false);
				return;
			}
			float frac_bulb;
			// TODO: better calc, this is straight up wrong in most cases
			frac_bulb = a->getDutyRange01() * b->getDutyRange01();
			setShapesFillColor(frac_bulb * col_yellow);
			if (a->shouldLightUpBulb(b)) {
				setShapesActive(true);
			}
			else {
				setShapesActive(false);
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
			float frac_c = gnd->determineLEDLightFraction(cool);
			float frac_w = gnd->determineLEDLightFraction(warm);
			CColor fin = col_cool * frac_c + col_warm * frac_w;

			setShapesActive(true);
			setShapesFillColor(fin);
		}
		break;
		case BM_RGB:
		{
			if (gnd->getVisitCount() == 0)
			{
				setShapesActive(false);
				return;
			}
			float frac_red = gnd->determineLEDLightFraction(red);
			float frac_green = gnd->determineLEDLightFraction(green);
			float frac_blue = gnd->determineLEDLightFraction(blue);
			CColor fin = col_red * frac_red + col_green * frac_green + col_blue * frac_blue;

			setShapesActive(true);
			setShapesFillColor(fin);
		}
		break;
		case BM_RGBCW:
		{
			if (gnd->getVisitCount() == 0)
			{
				setShapesActive(false);
				return;
			}
			float frac_red = gnd->determineLEDLightFraction(red);
			float frac_green = gnd->determineLEDLightFraction(green);
			float frac_blue = gnd->determineLEDLightFraction(blue);
			float frac_cool = gnd->determineLEDLightFraction(cool);
			float frac_warm = gnd->determineLEDLightFraction(warm);
			CColor fin = col_red * frac_red + col_green * frac_green + col_blue * frac_blue
				+ col_warm * frac_warm + col_cool * frac_cool;

			setShapesActive(true);
			setShapesFillColor(fin);
		}
		break;
	}
}
class CControllerBase *CControllerBulb::cloneController(class CShape *origOwner, class CShape *newOwner) {
	CControllerBulb *r = new CControllerBulb();
	for (unsigned int i = 0; i < shapes.size(); i++) {
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
