#ifndef __CONTROLLER_BUTTON_H__
#define __CONTROLLER_BUTTON_H__

#include "sim_local.h"
#include "Controller_Base.h"
#include "Coord.h"

class CControllerButton : public CControllerBase {
	float remDist;
	Coord openPos;
	Coord closedPos;
	class CShape *mover;
	float timeAfterMouseHold;
	class CJunction *a, *b;
public:
	CControllerButton(class CJunction *_a, class CJunction *_b);
	void setMover(CShape *p) {
		mover = p;
	}
	virtual void sendEvent(int code);
	virtual void onDrawn();
	virtual void rotateDegreesAround(float f, const Coord &p);
	virtual class CJunction *findOtherJunctionIfPassable(class CJunction *ju);
};

#endif // __CONTROLLER_BUTTON_H__
