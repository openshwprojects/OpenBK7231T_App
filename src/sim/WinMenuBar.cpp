#if WINDOWS

#include "WinMenuBar.h"
#include "Simulator.h"
#include "RecentList.h"
#include "Shape.h"
#include "PrefabManager.h"
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
	ID_OPEN_RECENT_FIRST,
	ID_OPEN_RECENT_LAST = ID_OPEN_RECENT_FIRST + 100,
	ID_CREATE_FIRST,
	ID_CREATE_LAST = ID_CREATE_FIRST + 100,
	ID_OPTIONS,
	ID_ABOUT,
};
void CWinMenuBar::createWindowsMenu(HWND windowRef) {

	hMenuBar = CreateMenu();
	hFile = CreateMenu();
	hEdit = CreateMenu();
	hHelp = CreateMenu();
	HMENU hRecent = CreateMenu();
	HMENU hCreate = CreateMenu();

	AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hFile, "File");
	AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hEdit, "Edit");
	AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hHelp, "Help");

	AppendMenu(hFile, MF_STRING, ID_NEW, "New (empty)");
	AppendMenu(hFile, MF_STRING, ID_NEW_DEMO, "New (built-in demo)");
	AppendMenu(hFile, MF_STRING, ID_LOAD, "Load...");
	AppendMenu(hFile, MF_STRING, ID_SAVE, "Save");
	AppendMenu(hFile, MF_STRING, ID_SAVEAS, "Save as...");
	AppendMenu(hFile, MF_POPUP, (UINT_PTR)hRecent, "Open recent");
	CRecentList *rList = sim->getRecents();
	recents.clear();
	for (int i = 0; i < rList->getSizeCappedAt(10); i++) {
		const char *entr = rList->get(i);
		recents.push_back(entr);
		CString tmp = "Open ";
		tmp.append(entr);
		AppendMenu(hRecent, MF_STRING, ID_OPEN_RECENT_FIRST + i, tmp.c_str());
	}
	AppendMenu(hFile, MF_STRING, ID_EXIT, "Exit");
	prefabNames.clear();
	AppendMenu(hEdit, MF_STRING, ID_OPTIONS, "Options");
	AppendMenu(hEdit, MF_POPUP, (UINT_PTR)hCreate, "Add..");
	PrefabManager *prefabs = sim->getPfbs();
	for (int i = 0; i < prefabs->size(); i++) {
		CShape *sh = prefabs->get(i);
		prefabNames.push_back(sh->getName());
		CString tmp = "Add ";
		tmp.append(sh->getName());
		AppendMenu(hCreate, MF_STRING, ID_CREATE_FIRST + i, tmp.c_str());
	}

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
			int id = LOWORD(Event.syswm.msg->msg.win.wParam);
			if (id == ID_EXIT)
			{
				sim->onUserClose();
			}
			else if (id == ID_NEW)
			{
				sim->createSimulation(false);
			}
			else if (id == ID_NEW_DEMO)
			{
				sim->createSimulation(true);
			}
			else if (id == ID_LOAD)
			{
				result = NFD_OpenDialog("obkproj", NULL, &outPath);
				if (result == NFD_OKAY) {
					sim->loadSimulation(outPath);
				}
			}
			else if (id == ID_SAVEAS)
			{
				showSaveAsDialog();
			}
			else if (id == ID_SAVE)
			{
				sim->saveOrShowSaveAsDialogIfNeeded();
			}
			else if (id >= ID_OPEN_RECENT_FIRST && id <= ID_OPEN_RECENT_LAST)
			{
				int recentID = id - ID_OPEN_RECENT_FIRST;
				const char *recentStr = recents[recentID].c_str();
				sim->loadSimulation(recentStr);
			}
			else if (id >= ID_CREATE_FIRST && id <= ID_CREATE_LAST)
			{
				int createID = id - ID_CREATE_FIRST;
				const char *createStr = prefabNames[createID].c_str();
				sim->beginAddingPrefab(createStr);
			}
		}
	}
}

#endif