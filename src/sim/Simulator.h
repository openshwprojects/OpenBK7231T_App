#ifndef __SIMULATOR_H__
#define __SIMULATOR_H__

#include "sim_local.h"

class CSimulator {
	// window
	bool bMouseButtonStates[10];
	SDL_Window *Window;
	SDL_GLContext Context;
	u32 WindowFlags;
	bool Running;
	bool FullScreen;
	class Tool_Base *activeTool;
	class CSimulation *sim;
	class CursorManager *cur;

	void onKeyDown(int keyCode);
	void setTool(Tool_Base *tb);
public:
	CSimulator();
	void drawWindow();
	void createWindow();
	void destroyObject(CShape *s);
	class CShape *getShapeUnderCursor();
	class CSimulation*getSim() {
		return sim;
	}
	class CursorManager*getCursorMgr() {
		return cur;
	}
	bool isMouseButtonHold(int idx) const {
		return bMouseButtonStates[idx];
	}
};

#endif