#ifdef WINDOWS
#include "Controller_WS2812.h"
#include "Shape.h"
#include "Junction.h"

extern "C" {
	bool SM16703P_GetPixel(uint32_t pixel, byte *dst);
}

void CControllerWS2812::setShapesActive(bool b) {
	for (int i = 0; i < shapes.size(); i++) {
		shapes[i]->setActive(b);
	}
}

void CControllerWS2812::setShapesFillColor(const CColor &c) {
	for (int i = 0; i < shapes.size(); i++) {
		shapes[i]->setFillColor(c);
	}
}
void CControllerWS2812::onDrawn() {
	byte rgb[3];

	CColor col_green(0, 255, 0);
	int idx = a->getDepth();
	if (b->getDepth() < idx)
		idx = b->getDepth();
	//printf("%i - depth %i\n", this, idx);
	SM16703P_GetPixel(idx, rgb);
	col_green.fromRGB(rgb);
	setShapesActive(true);
	setShapesFillColor(col_green);
}
class CJunction *CControllerWS2812::findOtherJunctionIfPassable(class CJunction *ju) {
	if (ju == a)
		return b;
	return a;
}
class CControllerBase *CControllerWS2812::cloneController(class CShape *origOwner, class CShape *newOwner) {
	CControllerWS2812 *r = new CControllerWS2812();
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
	return r;
}

#endif
