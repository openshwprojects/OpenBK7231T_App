#ifdef WINDOWS
#include "Simulator.h"
#include "Shape.h"
#include "Object.h"
#include "Wire.h"
#include "Controller_Button.h"
#include "Junction.h"
#include "Tool_Base.h"
#include "Tool_Wire.h"
#include "Tool_Use.h"
#include "Tool_Delete.h"
#include "Tool_Move.h"
#include "Tool_Info.h"
#include "Tool_Copy.h"
#include "Simulation.h"
#include "CursorManager.h"
#include "Solver.h"
#include "WinMenuBar.h"
#include "../cJSON/cJSON.h"


CSimulator::CSimulator() {
	memset(bMouseButtonStates, 0, sizeof(bMouseButtonStates));
	activeTool = 0;
	Window = 0;
	Context = 0;
	WindowFlags = SDL_WINDOW_OPENGL;
	Running = 1;
	FullScreen = 0;
	//setTool(new Tool_Wire());
	//setTool(new Tool_Use());
	setTool(new Tool_Move());
	sim = new CSimulation();
	sim->createDemo();
	solver = new CSolver();
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

void CSimulator::drawWindow() {
	SDL_Event Event;
	while (SDL_PollEvent(&Event))
	{
		if (winMenu) {
			winMenu->processEvent(Event);
		}
		if (Event.type == SDL_KEYDOWN)
		{
			onKeyDown(Event.key.keysym.sym);
		}
		else if (Event.type == SDL_MOUSEBUTTONDOWN)
		{
			int x = Event.button.x;
			int y = Event.button.y;
			int which = Event.button.button;
			if (activeTool) {
				activeTool->onMouseDown(Coord(x, y), which);
			}
			bMouseButtonStates[Event.button.button] = true;
		}
		else if (Event.type == SDL_MOUSEBUTTONUP)
		{
			int x = Event.button.x;
			int y = Event.button.y;
			int which = Event.button.button;
			if (activeTool) {
				activeTool->onMouseUp(Coord(x, y), which);
			}
			bMouseButtonStates[Event.button.button] = false;
		}
		else if (Event.type == SDL_QUIT)
		{
			Running = 0;
		}
	}

	glViewport(0, 0, WinWidth, WinHeight);
	glClearColor(1.f, 0.f, 1.f, 0.f);
	//glClearColor(1.f, 0.9f, 1.f, 0.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0f, WinWidth, WinHeight, 0.0f, 0.0f, 1.0f);

	int h = 20;
	h = drawText(10, h, "Demo %i", 1);
	if (sim != 0) {
		h = sim->drawTextStats(h);
	}
	if (activeTool != 0) {
		h = drawText(10, h, "Active Tool: %s", activeTool->getName());
		h = activeTool->drawTextStats(h);
	}

	glColor3f(0.7f, 0.7f, 0.7f);
	glLineWidth(0.25f);
	glBegin(GL_LINES);
	for (int i = 0; i < WinWidth; i += gridSize) {
		glVertex2f(i, 0);
		glVertex2f(i, WinHeight);
	}
	for (int i = 0; i < WinHeight; i += gridSize) {
		glVertex2f(0, i);
		glVertex2f(WinWidth, i);
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
void CSimulator::destroyObject(CShape *s) {
	sim->destroyObject(s);
}
class CShape *CSimulator::getShapeUnderCursor() {
	Coord p = GetMousePos();
	return sim->findShapeByBoundsPoint(p);
}
bool CSimulator::createSimulation(const char *s) {
	CString memPath = CString::constructPathByStrippingExt(s, "flashMemory.bin");
	CString simPath = CString::constructPathByStrippingExt(s, "simulation.txt");

	cJSON *root_proj = cJSON_CreateObject();
	cJSON_AddStringToObject(root_proj, "created", "qqq");
	cJSON_AddStringToObject(root_proj, "lastModified", "qqq");

	//sim->saveTo(simPath.c_str());
	cJSON *root_sim = cJSON_CreateObject();
	cJSON *main_sim = cJSON_AddObjectToObject(root_sim, "simulation");
	cJSON *main_objects = cJSON_AddObjectToObject(main_sim, "objects");
	for (int i = 0; i < sim->getObjectsCount(); i++) {
		CObject *obj = sim->getObject(i);
		cJSON *j_obj = cJSON_AddObjectToObject(main_objects, "object");
		const Coord &pos = obj->getPosition();
		float rot = obj->getRotationAccum();
		const char *name = obj->getName();
		cJSON_AddStringToObject(j_obj, "name", name);
		cJSON_AddNumberToObject(j_obj, "rotation", rot);
		cJSON_AddNumberToObject(j_obj, "x", pos.getX());
		cJSON_AddNumberToObject(j_obj, "y", pos.getY());
	}
	cJSON *main_wires = cJSON_AddObjectToObject(main_sim, "wires");
	for (int i = 0; i < sim->getWiresCount(); i++) {
		CWire *wire = sim->getWires(i);
		cJSON *j_wire = cJSON_AddObjectToObject(main_wires, "wire");
		CJunction *jA = wire->getJunction(0);
		CJunction *jB = wire->getJunction(1);
		cJSON_AddNumberToObject(j_wire, "x0", jA->getX());
		cJSON_AddNumberToObject(j_wire, "y0", jB->getY());
		cJSON_AddNumberToObject(j_wire, "x1", jA->getX());
		cJSON_AddNumberToObject(j_wire, "y1", jB->getY());
	}
	char *msg = cJSON_Print(root_sim);
	cJSON *n_jSim = cJSON_Parse(msg);
	cJSON *n_jSimSim = cJSON_GetObjectItemCaseSensitive(n_jSim, "simulation");
	cJSON *n_jWires = cJSON_GetObjectItemCaseSensitive(n_jSimSim, "wires");
	cJSON *n_jObjects = cJSON_GetObjectItemCaseSensitive(n_jSimSim, "objects");
	cJSON *jObject;
	cJSON *jWire;
	cJSON_ArrayForEach(jObject, n_jObjects)
	{
		cJSON *jX = cJSON_GetObjectItemCaseSensitive(jObject, "x");
		cJSON *jY = cJSON_GetObjectItemCaseSensitive(jObject, "y");
		cJSON *jName = cJSON_GetObjectItemCaseSensitive(jObject, "name");

	}
	cJSON_ArrayForEach(jWire, n_jWires)
	{
		cJSON *jX0 = cJSON_GetObjectItemCaseSensitive(jWire, "x0");
		cJSON *jY0 = cJSON_GetObjectItemCaseSensitive(jWire, "y0");
		cJSON *jX1 = cJSON_GetObjectItemCaseSensitive(jWire, "x1");
		cJSON *jY1 = cJSON_GetObjectItemCaseSensitive(jWire, "y1");

	}
	FILE *f = fopen("test.txt", "w");
	fprintf(f, msg);
	fclose(f);
	return false;
}
bool CSimulator::loadSimulation(const char *s) {

	return false;
}
bool CSimulator::saveSimulationAs(const char *s) {

	return false;
}


void CSimulator::createWindow() {
	Window = SDL_CreateWindow("OpenGL Test", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WinWidth, WinHeight, WindowFlags);
	assert(Window);
	winMenu = new CWinMenuBar();
	winMenu->setSimulator(this);
	winMenu->createMenuBar(Window);
	Context = SDL_GL_CreateContext(Window);
	cur = new CursorManager();
	createSimulation("default.obkproj");
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


#endif
