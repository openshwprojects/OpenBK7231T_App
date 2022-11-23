#ifdef WINDOWS
#include "CursorManager.h"


void CursorManager::setCursor(int cursorCode) {
	switch (cursorCode) {
	case SDL_SYSTEM_CURSOR_HAND:
	{
		if (hand == 0) {
			hand = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND);
		}
		SDL_SetCursor(hand);
		break;
	}
	case SDL_SYSTEM_CURSOR_SIZEALL:
	{
		if (sizeAll == 0) {
			sizeAll = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEALL);
		}
		SDL_SetCursor(sizeAll);
		break;
	}
	case SDL_SYSTEM_CURSOR_CROSSHAIR:
	{
		if (crosshair == 0) {
			crosshair = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_CROSSHAIR);
		}
		SDL_SetCursor(crosshair);
		break;
	}
	case SDL_SYSTEM_CURSOR_ARROW:
	{
		if (arrow == 0) {
			arrow = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
		}
		SDL_SetCursor(arrow);
		break;
	}
	case SDL_SYSTEM_CURSOR_NO:
	{
		if (no == 0) {
			no = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NO);
		}
		SDL_SetCursor(no);
		break;
	}
	}
}

#endif
