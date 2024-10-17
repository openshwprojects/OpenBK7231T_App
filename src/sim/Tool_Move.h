#ifndef __TOOL_MOVE_H__
#define __TOOL_MOVE_H__

#include "sim_local.h"
#include "Tool_Base.h"
#include "Coord.h"

class Tool_Move : public Tool_Base {
	bool bMovingButtonHeld;
	class CShape *currentTarget;
	Coord prevPos;
public:
	Tool_Move();
	virtual const char *getName() const {
		return "Move";
	}
	virtual float drawTextStats(float h);
	virtual void onEnd();
	virtual void drawTool();
	virtual void onMouseDown(const Coord &pos, int button);
	virtual void onMouseUp(const Coord &pos, int button);
};


#endif // __TOOL_MOVE_H__
