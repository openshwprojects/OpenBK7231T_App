#ifdef WINDOWS
#include "Simulator.h"
#include "Shape.h"
#include "Wire.h"
#include "Text.h"
#include "Controller_Button.h"
#include "Junction.h"
#include "Tool_Base.h"
#include "Tool_Wire.h"
#include "Tool_Use.h"
#include "Tool_Delete.h"
#include "Tool_Move.h"
#include "Tool_Info.h"
#include "Tool_Copy.h"
#include "Tool_Text.h"
#include "Simulation.h"
#include "CursorManager.h"
#include "PrefabManager.h"
#include "Solver.h"
#include "WinMenuBar.h"
#include "SaveLoad.h"
#include "sim_import.h"
#include "RecentList.h"


// not the best solution... but I need LFS access
extern "C" {
	void Test_FakeHTTPClientPacket_POST(const char *tg, const char *data);
}



CSimulator::CSimulator() {
	currentlyEditingText = 0;
	memset(bMouseButtonStates, 0, sizeof(bMouseButtonStates));
	activeTool = 0;
	Window = 0;
	Context = 0;
	WindowFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
	Running = 1;
	FullScreen = 0;
	//setTool(new Tool_Wire());
	//setTool(new Tool_Use());
	setTool(new Tool_Move());
	solver = new CSolver();
	saveLoad = new CSaveLoad();
	saveLoad->setSimulator(this);
	recents = new CRecentList();
	recents->load();
}

void CSimulator::setTool(Tool_Base *tb) {
	if (activeTool) {
		activeTool->onEnd();
		delete activeTool;
	}
	activeTool = tb;
	activeTool->setSimulator(this);
	activeTool->onBegin();
}
Coord camera(0, 0);
float zoomFactor = 1.0f;

Coord GetMousePosWorld() {
	Coord r;
	int mx, my;
	//SDL_GetGlobalMouseState(&mx, &my);
	SDL_GetMouseState(&mx, &my);
	// No longer needed after resize event was introduced
	// BUGFIX FOR MENUBAR OFFSET
	//my += WINDOWS_MOUSE_MENUBAR_OFFSET;
	r.set(mx, my);
	r = camera + r / zoomFactor;
	return r;
}
void CSimulator::drawWindow() {
	char buffer[256];
	const char *projectPathDisp = projectPath.c_str();
	if (*projectPathDisp == 0)
		projectPathDisp = "none";
	sprintf(buffer, "OpenBeken Simulator " __DATE__  " - %s", projectPathDisp);
	if (SIM_IsFlashModified()) {
		strcat(buffer, " (FLASH MODIFIED)");
	}
	if (bSchematicModified) {
		strcat(buffer, " (SCHEMATIC MODIFIED)");
	}
	SDL_SetWindowTitle(Window, buffer);

	SDL_Event Event;
	while (SDL_PollEvent(&Event))
	{
		if (winMenu) {
			winMenu->processEvent(Event);
		}
		if (Event.type == SDL_KEYDOWN)
		{
			if (currentlyEditingText) {
				if (currentlyEditingText->processKeyDown(Event.key.keysym.sym)) {
					currentlyEditingText->setTextEditMode(false);
					currentlyEditingText = 0;
				}
			}
			else {
				switch (Event.key.keysym.sym) {
				case SDLK_LEFT: camera.addX(-10.0f); break;
				case SDLK_RIGHT: camera.addX(10.0f); break;
				case SDLK_UP: camera.addY(10.0f); break;
				case SDLK_DOWN: camera.addY(-10.0f); break;
				}
				onKeyDown(Event.key.keysym.sym);
			}
		}
		else if (Event.type == SDL_MOUSEBUTTONDOWN)
		{
			//int x = Event.button.x;
			//int y = Event.button.y;
			Coord mouse = GetMousePosWorld();
			int which = Event.button.button;
			if (activeTool) {
				activeTool->onMouseDown(mouse, which);
			}
			bMouseButtonStates[Event.button.button] = true;
		}
		else if (Event.type == SDL_MOUSEBUTTONUP)
		{
			//int x = Event.button.x;
			//int y = Event.button.y;
			Coord mouse = GetMousePosWorld();
			int which = Event.button.button;
			if (activeTool) {
				activeTool->onMouseUp(mouse, which);
			}
			bMouseButtonStates[Event.button.button] = false;
		}
		else if (Event.type == SDL_TEXTEDITING)
		{
			printf("SDL_TEXTEDITING %s %i %i\n", Event.edit.text,Event.edit.start, Event.edit.length);
		}
		else if (Event.type == SDL_TEXTINPUT)
		{
			printf("SDL_TEXTINPUT %s\n", Event.text.text);
			if (currentlyEditingText) {
				currentlyEditingText->appendText(Event.text.text);
			}
			else {
				if (activeTool) {
					activeTool->onTextInput(Event.text.text);
				}
			}
		}
		else if (Event.type == SDL_MOUSEWHEEL)
		{
			Coord mouse = GetMousePosWorld();

			Coord worldBeforeZoom = camera + (mouse / zoomFactor);

			if (Event.wheel.y > 0) {
				zoomFactor *= 1.1f; 
			}
			else if (Event.wheel.y < 0) {
				zoomFactor /= 1.1f;
			}

			Coord worldAfterZoom = camera + (mouse / zoomFactor);

			camera += (worldBeforeZoom - worldAfterZoom);
		}
		else if (Event.type == SDL_QUIT)
		{
			Running = 0;
		}
		else if (Event.type == SDL_WINDOWEVENT) {
			switch (Event.window.event) {

			case SDL_WINDOWEVENT_RESIZED:   // exit game
				SDL_GetWindowSize(Window, &WinWidth, &WinHeight);
				break;
			case SDL_WINDOWEVENT_CLOSE:   // exit game
				onUserClose();
				break;

			default:
				break;
			}
		}
	}

	glViewport(0, 0, WinWidth, WinHeight);
	glClearColor(1.f, 1.f, 1.f, 1.f);
	//glClearColor(1.f, 0.f, 1.f, 0.f);
	//glClearColor(1.f, 0.9f, 1.f, 0.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, WinWidth,  WinHeight, 0, 0.0f, 1.0f);


	float h = 40.0f;
	h = drawText(NULL, 10, h, "OpenBeken Simulator");
	if (sim != 0) {
		h = sim->drawTextStats(h);
	}
	if (activeTool != 0) {
		h = drawText(NULL, 10, h, "Active Tool: %s", activeTool->getName());
		h = activeTool->drawTextStats(h);
	}
	if (currentlyEditingText) {
		h = drawText(NULL, 10, h, "You are currently editing a text field.");
	}
	glColor3f(1.0f, 0.0f, 0.0f);
	drawText(&g_style_text_red, 260, 40, "WARNING: The following sketch may not be a correct circuit schematic. Connections in this simulator are simplified.");

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(camera.getX(), camera.getX() + WinWidth / zoomFactor,
		camera.getY() + WinHeight / zoomFactor, camera.getY(), 0.0f, 1.0f);

	glColor3f(0.7f, 0.7f, 0.7f);
	glLineWidth(0.25f);
	glBegin(GL_LINES);
	float minX = camera.getX();
	float maxX = camera.getX() + WinWidth / zoomFactor;
	float minY = camera.getY();
	float maxY = camera.getY() + WinHeight / zoomFactor;

	float startX = minX - fmod(minX, gridSize);
	float startY = minY - fmod(minY, gridSize);

	for (float x = startX; x <= maxX; x += gridSize) {
		glVertex2f(x, minY);
		glVertex2f(x, maxY);
	}

	for (float y = startY; y <= maxY; y += gridSize) {
		glVertex2f(minX, y);
		glVertex2f(maxX, y);
	}
	glEnd();
	glColor3f(1, 1, 0);
	glLineWidth(2);
	//glBegin(GL_LINE_STRIP);
	//glVertex3f(100, 100, 0);
	//glVertex3f(400, 400, 0);
	//glVertex3f(100, 400, 0);
	//glEnd();

	sim->drawSim();
	// sim
	solver->setSimulation(sim);
	solver->solveVoltages();

	if (activeTool) {
		activeTool->drawTool();
	}
	//static Texture2D *t = 0;
	//if (t == 0) {
	//	t = new Texture2D();
	//	t->loadTexture("WB3S.png");
	//}
	//glColor3f(1, 1, 1);
	//glEnable(GL_TEXTURE_2D);
	//glBindTexture(GL_TEXTURE_2D, t->getHandle());
	//glBegin(GL_QUADS);
	//glTexCoord2f(0, 1);
	//glVertex2f(10, 10);
	//glTexCoord2f(0, 0);
	//glVertex2f(10, 150);
	//glTexCoord2f(1, 0);
	//glVertex2f(150, 150);
	//glTexCoord2f(1, 1);
	//glVertex2f(150, 10);
	//glEnd();
	//glDisable(GL_TEXTURE_2D);
	SDL_GL_SwapWindow(Window);
}
class CShape *CSimulator::allocByClassName(const char *className) {
	if (!stricmp(className, "CText"))
		return new CText();
	printf("CSimulator::allocByClassName: unhandled class %s\n", className);
	return 0;
}
void CSimulator::beginEditingText(class CText *txt) {
	if (currentlyEditingText != 0) {
		currentlyEditingText->setTextEditMode(false);
	}
	currentlyEditingText = txt;
	if (currentlyEditingText) {
		currentlyEditingText->setTextEditMode(true);
	}
}
void CSimulator::destroyObject(CShape *s) {
	sim->destroyObject(s);
}
class CShape *CSimulator::getShapeUnderCursor(bool bIncludeDeepText) {
	Coord p = GetMousePosWorld();
	return sim->findShapeByBoundsPoint(p,bIncludeDeepText);
}
bool CSimulator::createSimulation(bool bDemo) {
	projectPath = "";
	project = new CProject();
	sim = new CSimulation();
	sim->setSimulator(this);
	if (bDemo) {
		sim->createDemo();
	}
	else {
		sim->createDemoOnlyWB3S();
	}
	SIM_SetupEmptyFlashModeNoFile();
	SIM_ClearOBK(0);

	return false;
}
bool CSimulator::beginAddingPrefab(const char *s) {
	CShape *newShape = prefabs->instantiatePrefab(s);
	if (newShape == 0) {
		return true;
	}
	newShape->setPosition(80, 80);
	sim->addObject(newShape);
	return false;
}
void CSimulator::formatLFS() {
	CMD_ExecuteCommand("lfs_format", 0);
}
bool CSimulator::setAutoexecBat(const char *s) {
	if (FS_Exists(s) == false) {
		printf("CSimulator::setAutoexecBat: %s does not exist\n", s);
		return true;
	}
	char *data = FS_ReadTextFile(s);
	if (data == 0) {
		printf("CSimulator::setAutoexecBat: cannot open %s\n", s);
		return true;
	}
	Test_FakeHTTPClientPacket_POST("api/lfs/autoexec.bat", data);
	free(data);
	return false;
}
bool CSimulator::loadSimulation(const char *s) {
	CString fixed;
	if (FS_Exists(s) == false) {
		fixed = s;
		fixed.append(".obkproj");
		s = fixed.c_str();
		if (FS_Exists(s) == false) {
			printf("CSimulator::loadSimulation: cannot open %s\n", s);
			return true;
		}
	}
	printf("CSimulator::loadSimulation: called with name %s\n", s);
	CString simPath = CString::constructPathByStrippingExt(s, "simulation.json");
	CString memPath = CString::constructPathByStrippingExt(s, "flashMemory.bin");

	printf("CSimulator::loadSimulation: simPath %s\n", simPath.c_str());
	printf("CSimulator::loadSimulation: memPath %s\n", memPath.c_str());

	if (FS_Exists(simPath.c_str()) == false) {
		printf("CSimulator::loadSimulation: there is no %s\n", simPath.c_str());
		return true;
	}
	if (FS_Exists(memPath.c_str()) == false) {
		printf("CSimulator::loadSimulation: there is no %s\n", memPath.c_str());
		return true;
	}
	printf("CSimulator::loadSimulation: going to load %s\n", s);
	CProject *newProject = saveLoad->loadProjectFile(s);
	if (newProject == 0) {
		printf("CSimulator::loadSimulation: failed reading %s\n", s);
		return true;
	}
	printf("CSimulator::loadSimulation: loaded %s\n", s);
	CSimulation *newSim = saveLoad->loadSimulationFromFile(simPath.c_str());
	if (newSim == 0) {
		printf("CSimulator::loadSimulation: failed reading %s\n", simPath.c_str());
		return true;
	}
	printf("CSimulator::loadSimulation: loaded %s\n", simPath.c_str());
	projectPath = s;
	project = newProject;
	sim = newSim;
	recents->registerAndSave(projectPath.c_str());
	SIM_ClearOBK(memPath.c_str());
	sim->recalcBounds();
	bSchematicModified = false;

	return false;
}
void CSimulator::saveOrShowSaveAsDialogIfNeeded() {
	if (this->hasProjectPath()) {
		this->saveSimulation();
	}
	else {
		winMenu->showSaveAsDialog();
	}
}
bool CSimulator::saveSimulation() {

	saveSimulationAs(projectPath.c_str());

	return false;
}
bool CSimulator::saveSimulationAs(const char *s) {
	printf("CSimulator::saveSimulationAs: called with name %s\n", s);
	projectPath = s;
	project->setModifiedDate();
	CString simPath = CString::constructPathByStrippingExt(projectPath.c_str(), "simulation.json");
	CString memPath = CString::constructPathByStrippingExt(projectPath.c_str(), "flashMemory.bin");
	printf("CSimulator::saveSimulationAs: simPath %s\n", simPath.c_str());
	printf("CSimulator::saveSimulationAs: memPath %s\n", memPath.c_str());
	FS_CreateDirectoriesForPath(simPath.c_str());
	FS_CreateDirectoriesForPath(memPath.c_str());
	recents->registerAndSave(projectPath.c_str());
	saveLoad->saveProjectToFile(project, projectPath.c_str());
	saveLoad->saveSimulationToFile(sim, simPath.c_str());
	SIM_SaveFlashData(memPath.c_str());
	bSchematicModified = false;
	return false;
}


void CSimulator::showExitSaveMessageBox() {
	const SDL_MessageBoxButtonData buttons[] =
	{
		{SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 2, "Cancel"},
		{ /* .flags, .buttonid, .text */        0, 0, "No" },
		{SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 1, "Yes"}
	};
	const SDL_MessageBoxColorScheme colorScheme =
	{
		{ /* .colors (.r, .g, .b) */
			/* [SDL_MESSAGEBOX_COLOR_BACKGROUND] */
			{ 255,   0,   0 },
			/* [SDL_MESSAGEBOX_COLOR_TEXT] */
			{   0, 255,   0 },
			/* [SDL_MESSAGEBOX_COLOR_BUTTON_BORDER] */
			{ 255, 255,   0 },
			/* [SDL_MESSAGEBOX_COLOR_BUTTON_BACKGROUND] */
			{   0,   0, 255 },
			/* [SDL_MESSAGEBOX_COLOR_BUTTON_SELECTED] */
			{ 255,   0, 255 }
		}
	};
	const SDL_MessageBoxData messageboxdata =
	{
		SDL_MESSAGEBOX_INFORMATION, /* .flags */
		NULL, /* .window */
		"Save changes?", /* .title */
		"Do you want to save changes (yes), exit without saving (no), or cancel?", /* .message */
		SDL_arraysize(buttons), /* .numbuttons */
		buttons, /* .buttons */
		&colorScheme /* .colorScheme */
	};
	int buttonid;
	if (SDL_ShowMessageBox(&messageboxdata, &buttonid) < 0)
	{
		SDL_Log("error displaying message box");
		return;
	}
	if (buttonid == 0) {
		exit(0);
	}
	else if (buttonid == 1) {
		saveOrShowSaveAsDialogIfNeeded();
		exit(0);
	}
	else {
		// cancel
	}
}
void CSimulator::onUserClose() {
	if (SIM_IsFlashModified()==false && !bSchematicModified) {
		exit(0);
	}
	showExitSaveMessageBox();
}
void CSimulator::createWindow() {
	Window = SDL_CreateWindow("OpenBeken Simulator", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WinWidth, WinHeight, WindowFlags);
	assert(Window);
	winMenu = new CWinMenuBar();
	winMenu->setSimulator(this);
	prefabs = new PrefabManager(this);
	prefabs->createDefaultPrefabs();
	winMenu->createMenuBar(Window);
	Context = SDL_GL_CreateContext(Window);
	cur = new CursorManager();
	createSimulation(true);
}
void CSimulator::onKeyDown(int keyCode) {
	if (keyCode == '1') {
		setTool(new Tool_Use());
	}
	if (keyCode == '2') {
		setTool(new Tool_Move());
	}
	if (keyCode == '3') {
		setTool(new Tool_Wire());
	}
	if (keyCode == '4') {
		setTool(new Tool_Delete());
	}
	if (keyCode == '5') {
		setTool(new Tool_Copy());
	}
	if (keyCode == '6') {
		setTool(new Tool_Info());
	}
	if (keyCode == '7') {
		setTool(new Tool_Text());
	}
	//SDL_Cursor* cursor;
	//cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_CROSSHAIR);
	//SDL_SetCursor(cursor);

	if (activeTool) {
		activeTool->onKeyDown(keyCode);
	}
	switch (keyCode)
	{
	case SDLK_ESCAPE:
		//Running = 0;
		break;
	case 'f':
		FullScreen = !FullScreen;
		if (FullScreen)
		{
			SDL_SetWindowFullscreen(Window, WindowFlags | SDL_WINDOW_FULLSCREEN_DESKTOP);
		}
		else
		{
			SDL_SetWindowFullscreen(Window, WindowFlags);
		}
		break;
	default:
		break;
	}
}

void CSimulator::loadRecentProject() {
	if (recents->size() > 0) {
		loadSimulation(recents->get(0));
	}
}


#endif
