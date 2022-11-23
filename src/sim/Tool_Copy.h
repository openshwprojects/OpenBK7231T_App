#ifndef __TOOL_WIRE_H__
#define __TOOL_WIRE_H__

#include "sim_local.h"
#include "Tool_Base.h"
#include "Coord.h"

class Tool_Copy : public Tool_Base {
	CObject *copyingObject;
public:
	Tool_Copy();
	virtual const char *getName() const {
		return "Copy";
	}
	virtual void onKeyDown(int button);
	virtual void onMouseDown(const Coord &pos, int button);
	virtual void drawTool();
};

#endif // __TOOL_WIRE_H__
