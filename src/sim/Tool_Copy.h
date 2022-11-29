#ifndef __TOOL_COPY_H__
#define __TOOL_COPY_H__

#include "sim_local.h"
#include "Tool_Base.h"
#include "Coord.h"

class Tool_Copy : public Tool_Base {
	class CShape *copyingObject;
	Coord prevMousePos;
public:
	Tool_Copy();
	~Tool_Copy();
	virtual const char *getName() const {
		return "Copy";
	}
	virtual void onMouseDown(const class Coord &pos, int button);
	virtual void drawTool();
};

#endif // __TOOL_COPY_H__
