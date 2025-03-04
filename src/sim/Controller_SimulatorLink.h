#ifndef __CONTROLLER_SIMULATORLINK_H__
#define __CONTROLLER_SIMULATORLINK_H__

#include "sim_local.h"
#include "Controller_Base.h"
#include "Coord.h"

class CControllerSimulatorLink : public CControllerBase {
	TArray<CJunction*> related;
	bool bPowered;
public:
	CControllerSimulatorLink();
	void addRelatedJunction(CJunction *p) {
		related.push_back(p);
	}
	virtual void onDrawn();
	virtual void onPostSolveVoltages();
	class CControllerBase *cloneController(class CShape *origOwner, class CShape *newOwner);

	CJunction *findJunctionByGPIOIndex(int idx);
	bool isPowered() const;
};

#endif // __CONTROLLER_SIMULATORLINK_H__
