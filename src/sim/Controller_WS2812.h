#ifndef __CONTROLLER_WS2812_H__
#define __CONTROLLER_WS2812_H__

#include "sim_local.h"
#include "Controller_Base.h"
#include "Coord.h"

class CControllerWS2812 : public CControllerBase {
	TArray<class CShape *>shapes;
	class CJunction *a, *b;

	void setShapesActive(bool b);
	void setShapesFillColor(const CColor &c);
public:
	CControllerWS2812() {
		a = b = 0;
	}
	CControllerWS2812(class CJunction *_a, class CJunction *_b) {
		a = _a;
		b = _b;
	}
	void setShape(CShape *p) {
		shapes.clear();
		shapes.push_back(p);
	}
	void addShape(CShape *p) {
		shapes.push_back(p);
	}
	virtual void onDrawn();
	virtual class CControllerBase *cloneController(class CShape *origOwner, class CShape *newOwner);
	virtual class CJunction *findOtherJunctionIfPassable(class CJunction *ju);
};

#endif // __CONTROLLER_WS2812_H__
