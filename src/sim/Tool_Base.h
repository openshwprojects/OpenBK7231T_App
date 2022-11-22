#ifndef __TOOL_BASE_H__
#define __TOOL_BASE_H__

#include "sim_local.h"

class Tool_Base {
protected:
	class CSimulator *sim;
public:
	void setSimulator(class CSimulator *s) {
		this->sim = s;
	}
	virtual void onKeyDown(int button) {

	}
	virtual void onMouseDown(const Coord &pos, int button) {

	}
	virtual void onMouseUp(const Coord &pos, int button) {

	}
	virtual void drawTool() {

	}
	virtual void onBegin() {

	}
	virtual void onEnd() {

	}
};

#endif // __TOOL_BASE_H__
