#ifndef __CONTROLLER_BULB_H__
#define __CONTROLLER_BULB_H__

#include "sim_local.h"
#include "Controller_Base.h"
#include "Coord.h"

class CControllerBulb : public CControllerBase {
	class CShape *shape;
	class CJunction *a, *b;
public:
	CControllerBulb() {
		a = b = 0;
		shape = 0;
	}
	CControllerBulb(class CJunction *_a, class CJunction *_b) {
		a = _a;
		b = _b;
	}
	void setShape(CShape *p) {
		shape = p;
	}
	virtual void onDrawn();
	virtual class CControllerBase *cloneController(class CShape *origOwner, class CShape *newOwner);
};

#endif // __CONTROLLER_BULB_H__
