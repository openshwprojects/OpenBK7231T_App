#if WINDOWS

#include "WinMenuBar.h"
#include "Simulator.h"
#include "RecentList.h"
#include "Shape.h"
#include "PrefabManager.h"
#include <nfd.h>

// removeme
#include <Windows.h>
#include <shellapi.h>

#if DEBUG
#pragma comment (lib, "nfd_d.lib")
#else
#pragma comment (lib, "nfd.lib")
#pragma comment (lib, "vcruntime.lib")
char *strncat(char *s1, const char *s2, size_t n)
{
	unsigned int len1 = strlen(s1);
	unsigned int len2 = strlen(s2);

	if (len2 < n) {
		strcpy(&s1[len1], s2);
	}
	else {
		strncpy(&s1[len1], s2, n);
		s1[len1 + n] = '\0';
	}
	return s1;
}

#endif


#include <vector>
#include <string>

#ifdef _WIN32
#include <windows.h>
#else
#include <dirent.h>
#include <cstring>
#endif

bool has_extension(const std::string& filename, const std::string& ext) {
	size_t pos = filename.rfind('.');
	return (pos != std::string::npos && filename.substr(pos) == ext);
}

std::vector<std::string> list_files_with_extension(const std::string& path, const std::string& ext) {
	std::vector<std::string> file_list;

#ifdef _WIN32
	WIN32_FIND_DATA find_file_data;
	HANDLE hFind = FindFirstFile((path + "\\*").c_str(), &find_file_data);
	if (hFind == INVALID_HANDLE_VALUE) return file_list;

	do {
		const std::string file_name = find_file_data.cFileName;
		if (!(find_file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && has_extension(file_name, ext)) {
			file_list.push_back(file_name);
		}
	} while (FindNextFile(hFind, &find_file_data) != 0);

	FindClose(hFind);
#else
	DIR *dir = opendir(path.c_str());
	if (!dir) return file_list;

	struct dirent *entry;
	while ((entry = readdir(dir)) != NULL) {
		std::string file_name = entry->d_name;
		if (entry->d_type != DT_DIR && has_extension(file_name, ext)) {
			file_list.push_back(file_name);
		}
	}
	closedir(dir);
#endif

	return file_list;
}

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
	ID_OPEN_SAMPLE_FIRST,
	ID_OPEN_SAMPLE_LAST = ID_OPEN_SAMPLE_FIRST + 100,
	ID_OPEN_SAMPLES_NONE,
	ID_CREATE_FIRST,
	ID_CREATE_LAST = ID_CREATE_FIRST + 100,
	ID_OPTIONS,
	ID_LITTLEFS_FORMAT,
	ID_LITTLEFS_SETAUTOEXEC,
	ID_LITTLEFS_COPYAUTOEXECTOCLIPBOARD,
	ID_HELP_ABOUT,
	ID_ABOUT,
};
void CWinMenuBar::createWindowsMenu(HWND windowRef) {

	hMenuBar = CreateMenu();
	hFile = CreateMenu();
	hEdit = CreateMenu();
	hLittleFS = CreateMenu();
	hHelp = CreateMenu();
	HMENU hRecent = CreateMenu();
	HMENU hCreate = CreateMenu();
	HMENU hSample = CreateMenu();

	AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hFile, "File");
	AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hEdit, "Edit");
	AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hLittleFS, "LittleFS");
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
	AppendMenu(hFile, MF_POPUP, (UINT_PTR)hSample, "Open sample");
	samples = list_files_with_extension("examples/", ".obkproj");
	for (int i = 0; i < samples.size(); i++) {
		const char *entr = samples[i].c_str();
		recents.push_back(entr);
		CString tmp = "Open ";
		tmp.append(entr);
		AppendMenu(hSample, MF_STRING, ID_OPEN_SAMPLE_FIRST + i, tmp.c_str());
	}
	if (samples.size() == 0) {
		AppendMenu(hSample, MF_STRING, ID_OPEN_SAMPLES_NONE, "No samples found");
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


	AppendMenu(hLittleFS, MF_STRING, ID_LITTLEFS_SETAUTOEXEC, "Upload autoexec.bat...");
	AppendMenu(hLittleFS, MF_STRING, ID_LITTLEFS_COPYAUTOEXECTOCLIPBOARD, "Copy autoexec.bat to clipboard");
	AppendMenu(hLittleFS, MF_STRING, ID_LITTLEFS_FORMAT, "Format FileSystem");

	AppendMenu(hHelp, MF_STRING, ID_HELP_ABOUT, "About (open link)");

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
extern "C" const char *CMD_ExpandConstantString(const char *s, const char *stop, char *out, int outLen);

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
			else if (id == ID_HELP_ABOUT)
			{
				ShellExecute(0, 0, "https://www.elektroda.com/rtvforum/topic4046056.html",
					0, 0, SW_SHOW);
			}
			else if (id == ID_LITTLEFS_COPYAUTOEXECTOCLIPBOARD)
			{
				int maxLen = 1024 * 1024;
				char *buffer = (char*)malloc(maxLen);
				const char *res = CMD_ExpandConstantString("autoexec.bat", 0, buffer, maxLen);
				if (res&&*res) {
				}
				else {
					res = "No autoexec.bat file present!";
				}
				SDL_SetClipboardText(res);
				free(buffer);
			}
			else if (id == ID_LITTLEFS_SETAUTOEXEC)
			{
				result = NFD_OpenDialog("bat", NULL, &outPath);
				if (result == NFD_OKAY) {
					sim->setAutoexecBat(outPath);
				}
			}
			else if (id == ID_ABOUT)
			{
				MessageBoxA(NULL, "OpenBeken simulator - try and test OBK on Windows. Pair your Windows machine with Home Assistant via MQTT!",
					"About OBK Simulator", 0);
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
			else if (id >= ID_OPEN_SAMPLE_FIRST && id <= ID_OPEN_SAMPLE_LAST)
			{
				int sampleID = id - ID_OPEN_SAMPLE_FIRST;
				const char *sampleStr = samples[sampleID].c_str();
				std::string full = "examples/";
				full.append(sampleStr);
				sim->loadSimulation(full.c_str());
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