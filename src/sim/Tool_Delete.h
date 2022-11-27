#ifndef __TOOL_DELETE_H__
#define __TOOL_DELETE_H__

#include "sim_local.h"
#include "Tool_Base.h"
#include "Coord.h"

class Tool_Delete : public Tool_Base {
	class CShape *currentTarget;
public:
	Tool_Delete();
	virtual const char *getName() const {
		return "Delete";
	}
	virtual void drawTool();
	virtual void onEnd();
	virtual void onMouseDown(const Coord &pos, int button);

};


#endif // __TOOL_DELETE_H__
