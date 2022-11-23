#ifndef __TOOL_USE_H__
#define __TOOL_USE_H__

#include "sim_local.h"
#include "Tool_Base.h"
#include "Coord.h"

class Tool_Use : public Tool_Base {
	class CShape *currentTarget;
public:
	Tool_Use() {
		currentTarget = 0;
	}
	virtual const char *getName() const {
		return "Use";
	}
	virtual void drawTool();
	virtual void onEnd();
	virtual void onMouseDown(const Coord &pos, int button);
};

#endif // __TOOL_USE_H__
