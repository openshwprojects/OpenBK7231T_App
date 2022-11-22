#ifndef __SIMULATOR_H__
#define __SIMULATOR_H__

#include "sim_local.h"

class CSimulator {
	// window
	SDL_Window *Window;
	SDL_GLContext Context;
	u32 WindowFlags;
	bool Running;
	bool FullScreen;
	class Tool_Base *activeTool;
	class CSimulation *sim;

public:
	CSimulator();
	void drawWindow();
	void createWindow();
	class CSimulation*getSim() {
		return sim;
	}
};

#endif