#ifndef __CONTROLLER_SIMULATORLINK_H__
#define __CONTROLLER_SIMULATORLINK_H__

#include "sim_local.h"
#include "Controller_Base.h"
#include "Coord.h"

class CControllerSimulatorLink : public CControllerBase {
	TArray<CJunction*> related;
public:
	CControllerSimulatorLink();
	void addRelatedJunction(CJunction *p) {
		related.push_back(p);
	}
	virtual void onDrawn(); 
	class CControllerBase *cloneController(class CShape *origOwner, class CShape *newOwner);

	CJunction *findJunctionByGPIOIndex(int idx);
};

#endif // __CONTROLLER_SIMULATORLINK_H__
