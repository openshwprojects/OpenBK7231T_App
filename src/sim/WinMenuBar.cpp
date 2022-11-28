#if WINDOWS

#include "WinMenuBar.h"

CWinMenuBar::CWinMenuBar() {


}
enum {
	ID_NEW,
	ID_LOAD,
	ID_SAVEAS,
	ID_EXIT,
};
void CWinMenuBar::createWindowsMenu(HWND windowRef) {

	hMenuBar = CreateMenu();
	hFile = CreateMenu();
	hEdit = CreateMenu();
	hHelp = CreateMenu();

	AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hFile, "File");
	AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hEdit, "Edit");
	AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hHelp, "Help");

	AppendMenu(hFile, MF_STRING, 1, "New...");
	AppendMenu(hFile, MF_STRING, 1, "Load...");
	AppendMenu(hFile, MF_STRING, 1, "Save as...");
	AppendMenu(hFile, MF_STRING, ID_EXIT, "Exit");

	AppendMenu(hEdit, MF_STRING, 1, "Options");

	AppendMenu(hHelp, MF_STRING, 1, "About");

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
	if (Event.type == SDL_SYSWMEVENT) {
		if (Event.syswm.msg->msg.win.msg == WM_COMMAND)
		{
			if (LOWORD(Event.syswm.msg->msg.win.wParam) == ID_EXIT)
			{
				exit(0);
			}
			else if (LOWORD(Event.syswm.msg->msg.win.wParam) == ID_NEW)
			{

			}
			else if (LOWORD(Event.syswm.msg->msg.win.wParam) == ID_LOAD)
			{

			}
			else if (LOWORD(Event.syswm.msg->msg.win.wParam) == ID_SAVEAS)
			{

			}
		}
	}
}

#endif