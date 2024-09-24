#ifndef __CONTROLLER_BASE_H__
#define __CONTROLLER_BASE_H__

#include "sim_local.h"

class CControllerBase {

public:
	virtual void onDrawn() { }
	virtual void sendEvent(int code, const class Coord &mouseOfs) { }
	virtual void rotateDegreesAround(float f, const class Coord &p) { }
	// for buttons
	virtual class CJunction *findOtherJunctionIfPassable(class CJunction *ju) { return 0; }
	virtual class CControllerBase *cloneController(class CShape *origOwner, class CShape *newOwner) { return 0; }
	// save and load
	virtual void saveTo(struct cJSON *j_obj) { }
	virtual void loadFrom(struct cJSON *j_obj) { }
	// simulation
	virtual void onPostSolveVoltages() { }


};


#endif // __CONTROLLER_BASE_H__
