#if WINDOWS

#include "WinMenuBar.h"
#include "Simulator.h"
#include <nfd.h>
#pragma comment (lib, "nfd_d.lib")

CWinMenuBar::CWinMenuBar() {


}
enum {
	ID_NEW,
	ID_LOAD,
	ID_SAVEAS,
	ID_EXIT,
	ID_OPTIONS,
	ID_ABOUT,
};
void CWinMenuBar::createWindowsMenu(HWND windowRef) {

	hMenuBar = CreateMenu();
	hFile = CreateMenu();
	hEdit = CreateMenu();
	hHelp = CreateMenu();

	AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hFile, "File");
	AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hEdit, "Edit");
	AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hHelp, "Help");

	AppendMenu(hFile, MF_STRING, ID_NEW, "New...");
	AppendMenu(hFile, MF_STRING, ID_LOAD, "Load...");
	AppendMenu(hFile, MF_STRING, ID_SAVEAS, "Save as...");
	AppendMenu(hFile, MF_STRING, ID_EXIT, "Exit");

	AppendMenu(hEdit, MF_STRING, ID_OPTIONS, "Options");

	AppendMenu(hHelp, MF_STRING, ID_ABOUT, "About");

	SetMenu(windowRef, hMenuBar);

}
HWND CWinMenuBar::getHWNDForSDLWindow(SDL_Window* win) {
	SDL_SysWMinfo infoWindow;
	SDL_VERSION(&infoWindow.version);
	if (!SDL_GetWindowWMInfo(win, &infoWindow))
	{
		return NULL;
	}
	return (infoWindow.info.win.window);
}
void CWinMenuBar::createMenuBar(SDL_Window *win) {
	HWND hwnd = getHWNDForSDLWindow(win);
	createWindowsMenu(hwnd);
	//Enable WinAPI Events Processing
	SDL_EventState(SDL_SYSWMEVENT, SDL_ENABLE);
}
void CWinMenuBar::processEvent(const SDL_Event &Event) {
	nfdchar_t *outPath = NULL;
	nfdresult_t result;
	if (Event.type == SDL_SYSWMEVENT) {
		if (Event.syswm.msg->msg.win.msg == WM_COMMAND)
		{
			if (LOWORD(Event.syswm.msg->msg.win.wParam) == ID_EXIT)
			{
				exit(0);
			}
			else if (LOWORD(Event.syswm.msg->msg.win.wParam) == ID_NEW)
			{
				//nfdchar_t *outPath = NULL;
				//nfdresult_t result = NFD_PickFolder(NULL, &outPath);
				result = NFD_SaveDialog("sim",NULL, &outPath);
				if (result == NFD_OKAY) {
					sim->createSimulation(outPath);
				}
			}
			else if (LOWORD(Event.syswm.msg->msg.win.wParam) == ID_LOAD)
			{
				result = NFD_OpenDialog("sim", NULL, &outPath);
				if (result == NFD_OKAY) {
					sim->loadSimulation(outPath);
				}
			}
			else if (LOWORD(Event.syswm.msg->msg.win.wParam) == ID_SAVEAS)
			{
				result = NFD_SaveDialog("sim", NULL, &outPath);
				if (result == NFD_OKAY) {
					sim->saveSimulationAs(outPath);
				}

			}
		}
	}
}

#endif