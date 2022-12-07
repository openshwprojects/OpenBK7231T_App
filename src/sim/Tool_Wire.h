#ifndef __TOOL_WIRE_H__
#define __TOOL_WIRE_H__

#include "sim_local.h"
#include "Tool_Base.h"
#include "Coord.h"

class Tool_Wire : public Tool_Base {
	Coord basePos;
	bool bActive;
	bool bSideness;
	class CWire *newWire;
	Coord a, b, c;
public:
	Tool_Wire();
	virtual const char *getName() const {
		return "Wire";
	}
	virtual void onKeyDown(int button);
	virtual void onMouseDown(const Coord &pos, int button);
	virtual void drawTool();
};

#endif // __TOOL_WIRE_H__
