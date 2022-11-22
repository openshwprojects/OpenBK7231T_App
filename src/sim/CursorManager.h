#ifndef __CURSORMANAGER_H__
#define __CURSORMANAGER_H__

#include "sim_local.h"

class CursorManager {
	// SDL_SYSTEM_CURSOR_HAND
	SDL_Cursor *hand;
	// SDL_SYSTEM_CURSOR_CROSSHAIR
	SDL_Cursor *crosshair;
	// SDL_SYSTEM_CURSOR_ARROW
	SDL_Cursor *arrow;
	// SDL_SYSTEM_CURSOR_SIZEALL
	SDL_Cursor *sizeAll;
	// SDL_SYSTEM_CURSOR_NO
	SDL_Cursor *no;
public:
	CursorManager() {
		hand = 0;
		crosshair = 0;
		arrow = 0;
		sizeAll = 0;
		no = 0;
	}
	void setCursor(int cursorCode);
};

#endif 

