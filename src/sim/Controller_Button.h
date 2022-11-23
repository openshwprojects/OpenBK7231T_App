#ifndef __CONTROLLER_BUTTON_H__
#define __CONTROLLER_BUTTON_H__

#include "sim_local.h"
#include "Controller_Base.h"
#include "Coord.h"

class CControllerButton : public CControllerBase {
	Coord openPos;
	Coord closedPos;
	class CShape *mover;
	float timeAfterMouseHold;
public:
	CControllerButton();
	void setMover(CShape *p) {
		mover = p;
	}
	virtual void sendEvent(int code);
	virtual void onDrawn();
	virtual void rotateDegreesAround(float f, const Coord &p);
};

#endif // __CONTROLLER_BUTTON_H__
