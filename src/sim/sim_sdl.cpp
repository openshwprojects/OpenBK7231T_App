#ifdef WINDOWS

#include "sim_local.h"
#include "Texture.h"
#include "Shape.h"
#include "Object.h"
#include "Coord.h"
#include "Simulator.h"
#include "Simulation.h"
#include "Wire.h"
#include "Junction.h"
#include "Text.h"
#include "Line.h"
#include "Rect.h"
#include "Tool_Base.h"
#include "Tool_Wire.h"
#include "CursorManager.h"

#pragma comment (lib, "SDL2.lib")
#pragma comment (lib, "Opengl32.lib")
#pragma comment (lib, "freeglut.lib")

int WinWidth = 1680;
int WinHeight = 940;
#undef main

void drawText(int x, int y, const char* text) {
	glRasterPos2i(x, y);
	for (int i = 0; i < strlen(text); i++) {
		glutBitmapCharacter(GLUT_BITMAP_9_BY_15, (int)text[i]);
	}
}
int gridSize = 20;
float roundToGrid(float f) {
	float g = f / gridSize;
	g += 0.5;
	g = floor(g);
	return g * gridSize;
}
Coord roundToGrid(Coord c) {
	return Coord(roundToGrid(c.getX()), roundToGrid(c.getY()));
}

enum AnimTarget {
	ANT_BASEPOS,
	ANT_TARGETPOS,
};

class CAnimation {
	CShape *target;
	Coord basePos;
	Coord targetPos;
	float time;
public:
	void update(AnimTarget tg) {

	}
};
void CJunction::drawShape() {
	glPointSize(8.0f);
	glColor3f(0, 1, 0);
	glBegin(GL_POINTS);
	glVertex2f(getX(), getY());
	glEnd();
	glColor3f(1, 1, 1);
}
Coord GetMousePos() {
	Coord r;
	int mx, my;
	//SDL_GetGlobalMouseState(&mx, &my);
	SDL_GetMouseState(&mx, &my);
	r.set(mx, my);
	return r;
}
void CSimulation::recalcBounds() {
	for (int i = 0; i < objects.size(); i++) {
		objects[i]->recalcBoundsAll();
	}
}
void CSimulation::drawSim() {
	for (int i = 0; i < objects.size(); i++) {
		objects[i]->drawWithChildren(0);
	}
	for (int i = 0; i < wires.size(); i++) {
		wires[i]->drawWire();
	}
}
void CWire::drawWire() {
	glBegin(GL_LINE_STRIP);
	for (int i = 0; i < junctions.size(); i++) {
		CJunction *j = junctions[i];
		glVertex2f(j->getX(), j->getY());
	}
	glEnd();
}
class CShape *CSimulation::findShapeByBoundsPoint(const class Coord &p) {
	for (int i = 0; i < objects.size(); i++) {
		if (objects[i]->hasWorldPointInside(p))
			return objects[i];
	}
	return 0;
}
class CWire *CSimulation::addWire(const class Coord &a, const class Coord &b) {
	class CWire *cw = new CWire(a, b);
	wires.push_back(cw);
	return cw;
}
CObject * CSimulation::addObject(CObject *o) {
	objects.push_back(o);
	return o;
}
void CSimulation::createDemo() {
	addObject(generateWB3S())->setPosition(300, 200);
	addObject(generateButton())->setPosition(500, 200);
	recalcBounds();
}
class CBaseObject {

};
enum {
	EVE_NONE,
	EVE_LMB_HOLD,
};
class CControllerBase {

public:
	virtual void onDrawn() { }
	virtual void sendEvent(int code) { }
};
class CControllerButton : public CControllerBase {
	Coord openPos;
	Coord closedPos;
	CShape *mover;
	float timeAfterMouseHold;
public:
	CControllerButton();
	void setMover(CShape *p) {
		mover = p;
	}
	virtual void sendEvent(int code);
	virtual void onDrawn();
};
CControllerButton::CControllerButton() {
	timeAfterMouseHold = 99.0f;
	openPos.set(0, 8);
	closedPos.set(0, -8);
}
void CControllerButton::sendEvent(int code) {
	if (code == EVE_LMB_HOLD) {
		timeAfterMouseHold = 0;
	}
}
void CControllerButton::onDrawn() {
	float speed = 0.8f;
	if (timeAfterMouseHold < 0.2f) {
		mover->moveTowards(closedPos, speed);
	}
	else {
		mover->moveTowards(openPos, speed);
	}
	timeAfterMouseHold += 0.1f;
}

void CLine::recalcBoundsSelf() {
	bounds.clear();
	bounds.addPoint(Coord(0,0));
	bounds.addPoint(Coord(x2, y2) - getPosition());
}
void CRect::recalcBoundsSelf() {
	bounds.clear();
	bounds.addPoint(Coord(0, 0));
	bounds.addPoint(Coord(w, h));
}
void CLine::drawShape() {
	glBegin(GL_LINES);
	glVertex2f(getX(), getY());
	glVertex2f(x2, y2);
	glEnd();
}


void CText::drawShape() {
	drawText(getX(),getY(), txt.c_str());
}


void CRect::drawShape() {
	glBegin(GL_LINE_LOOP);
	glVertex2f(getX(),getY());
	glVertex2f(getX() + w, getY());
	glVertex2f(getX() + w, getY() + h);
	glVertex2f(getX(), getY() +h);
	glEnd();
}

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
class CObject *CSimulation::generateButton() {
	CObject *o = new CObject();
	CControllerButton *btn = new CControllerButton();
	o->setController(btn);
	o->addJunction(-40, -10);
	o->addJunction(40, -10);
	o->addLine(40, -10, 20, -10);
	o->addLine(-40, -10, -20, -10);
	CShape *mover = new CShape();
	mover->addLine(20, 10, -20, 10);
	mover->addLine(0, 20, 0, 10);
	o->addShape(mover);
	btn->setMover(mover);

	o->translateEachChild(0, 10);
	return o;
}
class CObject *CSimulation::generateWB3S() {

	CObject *o = new CObject();
	o->addText(-40, -25, "WB3S");
	o->addRect(-50, -20, 100, 180);
	const char *wb3sPinsRight[] = {
		"TXD1",
		"RXD1",
		"PWM2",
		"PWM3",
		"RXD2",
		"TXD2",
		"PWM1",
		"GND"
	};
	const char *wb3sPinsLeft[] = {
		"CEN",
		"ADC3",
		"EN",
		"P14",
		"PWM5",
		"PWM4",
		"PWM3",
		"VCC"
	};
	for (int i = 0; i < 8; i++) {
		int y = i * 20;
		o->addLine(50, y, 80, y);
		o->addLine(-50, y, -80, y);
		o->addJunction(-80, y)->addText(-5, -5, wb3sPinsLeft[i]);
		o->addJunction(80, y)->addText(-25, -5, wb3sPinsRight[i]);
	}
	return o;
}

class Tool_Move : public Tool_Base {
	class CShape *currentTarget;
	Coord prevPos;
public:
	Tool_Move();
	virtual void drawTool();
	virtual void onMouseDown(const Coord &pos, int button);
	virtual void onMouseUp(const Coord &pos, int button);
};
class Tool_Use : public Tool_Base {
	class CShape *currentTarget;
public:
	virtual void drawTool();
	virtual void onMouseDown(const Coord &pos, int button);
};
void Tool_Use::onMouseDown(const Coord &pos, int button) {

}
class Tool_Delete : public Tool_Base {

};
Tool_Move::Tool_Move() {
	currentTarget = 0;
}
void Tool_Move::onMouseUp(const Coord &pos, int button) {
	//sim->getCursorMgr()->setCursor(SDL_SYSTEM_CURSOR_ARROW);

}
void Tool_Move::onMouseDown(const Coord &pos, int button) {
	currentTarget = sim->getShapeUnderCursor();
	if (currentTarget) {
		prevPos = GetMousePos();
		prevPos = roundToGrid(prevPos);
		//sim->getCursorMgr()->setCursor(SDL_SYSTEM_CURSOR_SIZEALL);
	}
	else {
		//sim->getCursorMgr()->setCursor(SDL_SYSTEM_CURSOR_ARROW);
	}

}
void Tool_Move::drawTool() {
	if (sim->isMouseButtonHold(SDL_BUTTON_LEFT)) {
		if (currentTarget != 0) {
			Coord nowPos = GetMousePos();
			nowPos = roundToGrid(nowPos);
			Coord delta = nowPos - prevPos;
			if (delta.isNonZero()) {
				prevPos = nowPos;
				currentTarget->translate(delta);
			}

		}
	}
	else {
		CShape *o = sim->getShapeUnderCursor();
		if(o){
			sim->getCursorMgr()->setCursor(SDL_SYSTEM_CURSOR_SIZEALL);
		}
		else {
			sim->getCursorMgr()->setCursor(SDL_SYSTEM_CURSOR_ARROW);
		}
	}

}
void Tool_Use::drawTool() {
	currentTarget = sim->getShapeUnderCursor();
	if (currentTarget) {
		sim->getCursorMgr()->setCursor(SDL_SYSTEM_CURSOR_HAND);
	}
	else {
		sim->getCursorMgr()->setCursor(SDL_SYSTEM_CURSOR_ARROW);
	}
	if (currentTarget != 0) {
		CControllerBase *cntrl = currentTarget->getController();
		if (cntrl != 0) {
			if (sim->isMouseButtonHold(SDL_BUTTON_LEFT)) {
				cntrl->sendEvent(EVE_LMB_HOLD);
			}
		}
	}
}
Tool_Wire::Tool_Wire() {
	newWire = 0;
	bActive = false;
}
void Tool_Wire::onKeyDown(int button) {
	if (button == SDLK_ESCAPE) {
		bActive = false;
		newWire = 0;
	}
}
void Tool_Wire::onMouseDown(const Coord &pos, int button) {
	if (button == SDL_BUTTON_RIGHT) {
		bSideness = !bSideness;
	}
	if (button == SDL_BUTTON_LEFT) {
		Coord curPos = roundToGrid(GetMousePos());
		if (bActive) {
			if (newWire == 0) {
				newWire = sim->getSim()->addWire(a, b);
				newWire->addPoint(c);
			} else {
				newWire->addPoint(b);
				newWire->addPoint(c);
			}
			basePos = curPos;
		}
		else {
			basePos = curPos;
			bActive = true;
		}
	}
}
void Tool_Wire::drawTool() {
	Coord m;
	m = roundToGrid(GetMousePos());
	if (bActive) {
		if (0) {
			glBegin(GL_LINES);
			glVertex2fv(basePos);
			glVertex2fv(m);
			glEnd();
		}
		a = basePos;
		b = basePos;
		if (bSideness) {
			b.setX(m.getX());
		}
		else {
			b.setY(m.getY());
		}
		c = m;
		glBegin(GL_LINE_STRIP);
		glVertex2fv(a);
		glVertex2fv(b);
		glVertex2fv(c);
		glEnd();

	}
}



void CShape::translateEachChild(float oX, float oY) {
	for (int i = 0; i < shapes.size(); i++) {
		shapes[i]->translate(oX, oY);
	}
}
bool CShape::hasWorldPointInside(const Coord &p)const {
	Coord loc = p - this->getPosition();
	return hasLocalPointInside(loc);
}
bool CShape::hasLocalPointInside(const Coord &p)const {
	return bounds.isInside(p);
}
void CShape::recalcBoundsAll() {
	bounds.clear();
	this->recalcBoundsSelf();
	for (int i = 0; i < shapes.size(); i++) {
		shapes[i]->recalcBoundsAll();
		bounds.addBounds(shapes[i]->getBounds(), shapes[i]->getPosition());
	}
}
class CShape* CShape::addLine(int x, int y, int x2, int y2) {
	CLine *n = new CLine(x, y, x2, y2);
	shapes.push_back(n);
	return n;
}
class CShape* CShape::addShape(CShape *p) {
	shapes.push_back(p);
	return p;
}
class CShape* CShape::addJunction(int x, int y, const char *name) {
	CJunction *n = new CJunction(x, y, name);
	shapes.push_back(n);
	return n;
}
class CShape* CShape::addRect(int x, int y, int w, int h) {
	CRect *n = new CRect(x, y, w, h);
	shapes.push_back(n);
	return n;
}
class CShape* CShape::addText(int x, int y, const char *s) {
	CText *n = new CText(x, y, s);
	shapes.push_back(n);
	return n;
}
void CShape::moveTowards(const Coord &tg,float dt) {
	pos = pos.moveTowards(tg, dt);
}
void CShape::drawWithChildren(int depth) {
	if (controller != 0) {
		controller->onDrawn();
	}
	drawShape();
	glPushMatrix();
	glTranslatef(getX(), getY(), 0);
	for (int i = 0; i < shapes.size(); i++) {
		shapes[i]->drawWithChildren(depth+1);
	}
	//recalcBoundsAll();
	if (depth == 0) {
		glColor3f(0, 1, 0);
		glLineWidth(0.5f);
		glBegin(GL_LINE_LOOP);
		Bounds bb = bounds;
		bb.extend(4);
		for (int i = 0; i < 4; i++) {
			glVertex2fv(bb.getCorner(i));
		}
		glEnd();
	}
	glPopMatrix();
}
CSimulator::CSimulator() {
	memset(bMouseButtonStates,0, sizeof(bMouseButtonStates));
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
CWire::CWire(const Coord &a, const Coord &b) {
	addPoint(a);
	addPoint(b);
}

void CWire::addPoint(const Coord &p) {
	CJunction *j = new CJunction(p.getX(),p.getY(),"");
	junctions.push_back(j);
}



class CShape *CSimulator::getShapeUnderCursor() {
	Coord p = GetMousePos();
	return sim->findShapeByBoundsPoint(p);
}
void CSimulator::createWindow() {
	Window = SDL_CreateWindow("OpenGL Test", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WinWidth, WinHeight, WindowFlags);
	assert(Window);
	Context = SDL_GL_CreateContext(Window);
	cur = new CursorManager();
}

void CSimulator::onKeyDown(int keyCode) {
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
void CSimulator::drawWindow() {
	SDL_Event Event;
	while (SDL_PollEvent(&Event))
	{
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
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0f, WinWidth, WinHeight, 0.0f, 0.0f, 1.0f);

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
int main(int argc, char **argv)
{
	glutInit(&argc, argv);
	CSimulator *sim = new CSimulator();
	sim->createWindow();

	while (1)
	{
		sim->drawWindow();
	}
	return 0;
}
#endif
