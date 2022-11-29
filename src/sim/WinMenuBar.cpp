#if WINDOWS

#include "WinMenuBar.h"
#include "Simulator.h"
#include <nfd.h>
#pragma comment (lib, "nfd_d.lib")

CWinMenuBar::CWinMenuBar() {


}
enum {
	ID_NEW,
	ID_NEW_DEMO,
	ID_LOAD,
	ID_SAVEAS,
	ID_SAVE,
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

	AppendMenu(hFile, MF_STRING, ID_NEW, "New (empty)");
	AppendMenu(hFile, MF_STRING, ID_NEW_DEMO, "New (built-in demo)");
	AppendMenu(hFile, MF_STRING, ID_LOAD, "Load...");
	AppendMenu(hFile, MF_STRING, ID_SAVE, "Save");
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
void CWinMenuBar::showSaveAsDialog() {
	nfdchar_t *outPath = NULL;
	nfdresult_t result;
	CString tmp;
	result = NFD_SaveDialog("obkproj", NULL, &outPath);
	if (result == NFD_OKAY) {
		tmp = outPath;
		if (tmp.hasExtension() == false) {
			tmp.append(".obkproj");
		}
		sim->saveSimulationAs(tmp.c_str());
	}
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
				sim->createSimulation(false);
			}
			else if (LOWORD(Event.syswm.msg->msg.win.wParam) == ID_NEW_DEMO)
			{
				sim->createSimulation(true);
			}
			else if (LOWORD(Event.syswm.msg->msg.win.wParam) == ID_LOAD)
			{
				result = NFD_OpenDialog("obkproj", NULL, &outPath);
				if (result == NFD_OKAY) {
					sim->loadSimulation(outPath);
				}
			}
			else if (LOWORD(Event.syswm.msg->msg.win.wParam) == ID_SAVEAS)
			{
				showSaveAsDialog();
			}
			else if (LOWORD(Event.syswm.msg->msg.win.wParam) == ID_SAVE)
			{
				if (sim->hasProjectPath()) {
					sim->saveSimulation();
				}
				else {
					showSaveAsDialog();
				}
			}
		}
	}
}

#endif