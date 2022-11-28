#ifndef __WINMENUBAR_H__
#define __WINMENUBAR_H__

#include "sim_local.h"
#include <Windows.h>
#include <SDL_syswm.h>

class CWinMenuBar {
	HMENU hMenuBar;
	HMENU hFile;
	HMENU hEdit;
	HMENU hHelp;

	void createWindowsMenu(HWND windowRef);
	HWND getHWNDForSDLWindow(SDL_Window* win);
public:
	CWinMenuBar();

	void createMenuBar(SDL_Window *win);
	void processEvent(const SDL_Event &Event);

};

#endif