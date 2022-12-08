#ifndef __CONTROLLER_POT_H__
#define __CONTROLLER_POT_H__

#include "sim_local.h"
#include "Controller_Base.h"
#include "Coord.h"

class CControllerPot : public CControllerBase {
	Coord posA, posB;
	float frac;
	class CShape *mover;
	class CText *display;
	class CJunction *a, *b, *o;
public:
	CControllerPot() {
		mover = 0;
		a = b = o = 0;
	}
	CControllerPot(class CJunction *_a, class CJunction *_b, class CJunction *_o);
	void setMover(CShape *p) {
		mover = p;
	}
	void setDisplay(CText *p) {
		display = p;
	}
	virtual void sendEvent(int code, const class Coord &mouseOfs);
	virtual void onDrawn();
	virtual void rotateDegreesAround(float f, const Coord &p);
	virtual class CJunction *findOtherJunctionIfPassable(class CJunction *ju);
	virtual class CControllerBase *cloneController(class CShape *origOwner, class CShape *newOwner);
};

#endif // __CONTROLLER_POT_H__
