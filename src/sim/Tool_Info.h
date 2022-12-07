#ifndef __TOOL_INFO_H__
#define __TOOL_INFO_H__

#include "sim_local.h"
#include "Tool_Base.h"
#include "Coord.h"

class Tool_Info : public Tool_Base {
	class CShape *currentTarget;
	Coord prevPos;
public:
	Tool_Info();
	virtual const char *getName() const {
		return "Info";
	}
	virtual void drawTool();
	virtual void onMouseDown(const Coord &pos, int button);
};


#endif // __TOOL_INFO_H__
