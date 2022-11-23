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
#include "Tool_Use.h"
#include "Tool_Move.h"
#include "Tool_Delete.h"
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
CJunction::~CJunction() {
	for (int i = 0; i < linked.size(); i++) {
		CJunction *oj = linked[i];
		oj->unlink(this);
	}
}
bool CJunction::isWireJunction() const {
	return getParent()->isWire();
}
void CJunction::unlink(class CJunction *o) {
	linked.erase(std::remove(linked.begin(), linked.end(), o), linked.end());
}
bool CJunction::hasLinkedOnlyWires() const {
	for (int i = 0; i < linked.size(); i++) {
		CJunction *oj = linked[i];
		if (oj->isWireJunction() == false)
			return false;
	}
	return true;
}
void CJunction::translateLinked(const Coord &o) {
	for (int i = 0; i < linked.size(); i++) {
		CJunction *oj = linked[i];
		oj->setPosition(oj->getPosition() + o);
	}
}
void CJunction::translate(const Coord &o) {
	if (hasLinkedOnlyWires()) {
		CShape::translate(o);
		translateLinked(o);
	}
}
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
CWire::~CWire() {
	for (int i = 0; i < junctions.size(); i++) {
		delete junctions[i];
	}
	for (int i = 0; i < edges.size(); i++) {
		delete edges[i];
	}
}
void CWire::drawWire() {
	glColor3f(0, 1, 0);
	glLineWidth(2.0f);
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
	for (int i = 0; i < wires.size(); i++) {
		CShape *r = wires[i]->findSubPart(p);
		if (r)
			return r;
	}
	return 0;
}
class CWire *CSimulation::addWire(const class Coord &a, const class Coord &b) {
	class CWire *cw = new CWire(a, b);
	wires.push_back(cw);
	matchJunction(cw);
	return cw;
}
CObject * CSimulation::addObject(CObject *o) {
	objects.push_back(o);
	return o;
}
void CSimulation::createDemo() {
	addObject(generateWB3S())->setPosition(300, 200);
	addObject(generateButton())->setPosition(500, 260)->rotateDegreesAroundSelf(180);
	addObject(generateTest())->setPosition(500, 400);
	addObject(generateGND())->setPosition(600, 400);
	addWire(Coord(460, 260), Coord(380, 260));
	addWire(Coord(540, 260), Coord(600, 260));
	addWire(Coord(600, 400), Coord(600, 260));


	addWire(Coord(560, 220), Coord(380, 220));
	addObject(generateButton())->setPosition(600, 220)->rotateDegreesAroundSelf(180);
	addWire(Coord(640, 220), Coord(700, 220));
	addWire(Coord(700, 400), Coord(700, 220));
	addObject(generateGND())->setPosition(700, 400);
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
	virtual void rotateDegreesAround(float f, const Coord &p) { }
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
	virtual void rotateDegreesAround(float f, const Coord &p);
};
CControllerButton::CControllerButton() {
	timeAfterMouseHold = 99.0f;
	openPos.set(0, 16);
	closedPos.set(0, 0);
}
void CControllerButton::rotateDegreesAround(float f, const Coord &p) {
	openPos = openPos.rotateDegreesAround(f, p);
	closedPos = closedPos.rotateDegreesAround(f,p);
}
void CControllerButton::sendEvent(int code) {
	if (code == EVE_LMB_HOLD) {
		timeAfterMouseHold = 0;
	}
}
void CControllerButton::onDrawn() {
	float speed = 2.0f;
	if (timeAfterMouseHold < 0.2f) {
		mover->moveTowards(closedPos, speed);
	}
	else {
		mover->moveTowards(openPos, speed);
	}
	timeAfterMouseHold += 0.1f;
}
inline float fMAX(float a, float b) {
	if (a > b)
		return a;
	return b;
}
inline float fMIN(float a, float b) {
	if (a < b)
		return a;
	return b;
}
void CLine::moveTowards(const Coord &tg, float dt) {
	const float l2 = pos.distSq(pos2);  // i.e. |w-v|^2 -  avoid a sqrt
	float t = fMAX(0, fMIN(1, Coord::dot(tg - pos, pos2 - pos) / l2));
	const Coord projection = pos + t * (pos2 - pos);  // Projection falls on the segment
	Coord dir = tg - projection;
	float rem = dir.normalize();
	if (rem < dt) {
	}
	else {
		pos += dir * dt;
		pos2 += dir * dt;
	}
}
void CLine::recalcBoundsSelf() {
	bounds.clear();
	bounds.addPoint(Coord(0,0));
	bounds.addPoint(pos2 - getPosition());
}
void CRect::rotateDegreesAround_internal(float f, const Coord &p) {
	Coord a = getPosition();
	Coord b = getCorner();
	a = a.rotateDegreesAround(f, p);
	b = b.rotateDegreesAround(f, p);
	setFromTwoPoints(a, b);
}
void CRect::recalcBoundsSelf() {
	bounds.clear();
	bounds.addPoint(Coord(0, 0));
	bounds.addPoint(Coord(w, h));
}
void CLine::drawShape() {
	glBegin(GL_LINES);
	glVertex2f(getX(), getY());
	glVertex2f(pos2.getX(),pos2.getY());
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
class CObject *CSimulation::generateGND() {
	CObject *o = new CObject();
	o->addJunction(0, -20, "pad_a");
	o->addLine(0, -20, 0, 0);
	o->addLine(-10, 0, 10, 0);
	o->addLine(-8, 2, 8, 2);
	o->addLine(-6, 4, 6, 4);
	o->addLine(-4, 6, 4, 6);
	return o;
}
class CObject *CSimulation::generateButton() {
	CObject *o = new CObject();
	CControllerButton *btn = new CControllerButton();
	o->setController(btn);
	o->addJunction(-40, -10,"pad_a");
	o->addJunction(40, -10,"pad_b");
	o->addLine(40, -10, 20, -10);
	o->addLine(-40, -10, -20, -10);
#if 0
	CShape *mover = new CShape();
	mover->addLine(20, 10, -20, 10);
	mover->addLine(0, 20, 0, 10);
	o->addShape(mover);
#else
	CShape *mover = o->addLine(20, 10, -20, 10);
//	mover->addLine(-20, 0, -20, 10);

#endif
	btn->setMover(mover);

	o->translateEachChild(0, 10);
	return o;
}
class CObject *CSimulation::generateTest() {

	CObject *o = new CObject();
	o->addLine(50, 10, -50, 10);
	o->addLine(50, -10, -50, -10);
	o->addLine(50, -10, 50, 10);
	o->addLine(-50, 10, -50, -10);
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

void Tool_Use::onMouseDown(const Coord &pos, int button) {

}

Tool_Move::Tool_Move() {
	currentTarget = 0;
}
void Tool_Move::onMouseUp(const Coord &pos, int button) {
	//sim->getCursorMgr()->setCursor(SDL_SYSTEM_CURSOR_ARROW);

}
void Tool_Move::onEnd() {
	sim->getCursorMgr()->setCursor(SDL_SYSTEM_CURSOR_ARROW);
}
void Tool_Move::onMouseDown(const Coord &pos, int button) {
	if (button == SDL_BUTTON_LEFT) {
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
	if (button == SDL_BUTTON_RIGHT) {
		if (currentTarget) {
			currentTarget->rotateDegreesAroundSelf(90);
		}
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
				sim->getSim()->matchJunctionsOf_r(currentTarget);
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
void Tool_Use::onEnd() {
	sim->getCursorMgr()->setCursor(SDL_SYSTEM_CURSOR_ARROW);
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
Tool_Delete::Tool_Delete() {
	currentTarget = 0;
}
void Tool_Delete::onMouseDown(const Coord &pos, int button) {
	if (currentTarget != 0) {
		sim->destroyObject(currentTarget);
	}
}
void Tool_Delete::drawTool() {
	currentTarget = sim->getShapeUnderCursor();
	if (currentTarget) {
		sim->getCursorMgr()->setCursor(SDL_SYSTEM_CURSOR_NO);
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
#if 1				
			newWire = sim->getSim()->addWire(a, b);
			newWire = sim->getSim()->addWire(b, c);
#else
			if (newWire == 0) {
				newWire = sim->getSim()->addWire(a, b);
				newWire->addPoint(c);
			} else {
				newWire->addPoint(b);
				newWire->addPoint(c);
			}
#endif
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
	sim->getCursorMgr()->setCursor(SDL_SYSTEM_CURSOR_CROSSHAIR);
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
void CShape::rotateDegreesAround(float f, const Coord &p) {
	Coord p_i = p - this->getPosition();
	rotateDegreesAround_internal(f, p);
	if (controller != 0) {
		controller->rotateDegreesAround(f, p_i);
	}
	for (int i = 0; i < shapes.size(); i++) {
		shapes[i]->rotateDegreesAround(f, p_i);
	}
	recalcBoundsAll();
}
void CShape::rotateDegreesAroundSelf (float f) {
	//rotateDegreesAround(f, Coord(0, 0));
	rotateDegreesAround(f, getPosition());
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
	addShape(n);
	return n;
}
class CShape* CShape::addShape(CShape *p) {
	p->parent = this;
	shapes.push_back(p);
	return p;
}
class CShape* CShape::addJunction(int x, int y, const char *name) {
	CJunction *n = new CJunction(x, y, name);
	addShape(n);
	return n;
}
class CShape* CShape::addRect(int x, int y, int w, int h) {
	CRect *n = new CRect(x, y, w, h);
	addShape(n);
	return n;
}
class CShape* CShape::addText(int x, int y, const char *s) {
	CText *n = new CText(x, y, s);
	addShape(n);
	return n;
}
Coord CShape::getAbsPosition() const {
	Coord ofs = this->getPosition();
	const CShape *p = this->getParent();
	while (p) {
		ofs += p->getPosition();
		p = p->getParent();
	}
	return ofs;
}
void CShape::translate(const Coord &ofs) {
	pos += ofs;
	// special handling for connected wires
	for (int i = 0; i < shapes.size(); i++) {
		CShape *o = shapes[i];
		if (o->isJunction() == false)
			continue;
		CJunction *j = dynamic_cast<CJunction*>(o);
		j->translateLinked(ofs);
	}
}
void CLine::rotateDegreesAround_internal(float f, const Coord &p) {
	pos = pos.rotateDegreesAround(f, p);
	pos2 = pos2.rotateDegreesAround(f, p);
}
void CShape::rotateDegreesAround_internal(float f, const Coord &p) {
	pos = pos.rotateDegreesAround(f, p);
}
void CJunction::rotateDegreesAround_internal(float f, const Coord &p) {
	pos = pos.rotateDegreesAround(f, p);
}
void CText::rotateDegreesAround_internal(float f, const Coord &p) {
	//pos = pos.rotateDegreesAround(f, p);
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
void CEdge::translate(const Coord &o) {
	a->translate(o);
	b->translate(o);
}
const Coord &CEdge::getPositionA() const {
	return a->getPosition();
}
const Coord &CEdge::getPositionB() const {
	return b->getPosition();
}
class CShape* CWire::findSubPart(const Coord &p) {
	for (int i = 0; i < junctions.size(); i++) {
		if (p.isWithinDist(junctions[i]->getPosition(), 5))
		{
			return junctions[i];
		}
	}
	for (int i = 0; i < edges.size(); i++) {
		if (p.isWithinDist(edges[i]->getPositionA(), edges[i]->getPositionB(), 5))
		{
			return edges[i];
		}
	}
	return 0;
}

void CWire::addPoint(const Coord &p) {
	CJunction *prev = 0;
	if (junctions.size()) {
		prev = junctions[junctions.size() - 1];
	}
	CJunction *j = new CJunction(p.getX(),p.getY(),"");
	j->setParent(this);
	junctions.push_back(j);
	if (prev != 0) {
		CEdge *e = new CEdge(prev, j);
		e->setParent(this);
		edges.push_back(e);
	}
}



void CSimulator::destroyObject(CShape *s) {
	sim->destroyObject(s);
}
void CSimulation::matchJunctionsOf_r(class CShape *s) {
	for (int j = 0; j < s->getShapesCount(); j++) {
		CShape *ch = s->getShape(j);
		if (ch->isJunction()) {
			// this could be done much better, dont rematch all, just add new
			CJunction *jun = dynamic_cast<CJunction*>(ch);
			matchJunction(jun);
		}
		else {
			matchJunctionsOf_r(ch);
		}
	}
}
void CSimulation::matchJunction(class CWire *w) {
	for (int j = 0; j < w->getJunctionsCount(); j++) {
		CJunction *oj = w->getJunction(j);
		matchJunction(oj);
	}
}
void CSimulation::tryMatchJunction(class CJunction *jn, class CJunction *oj) {
	if (oj == 0)
		return;
	if (jn == 0)
		return;
	if (oj == jn)
		return;
	if (oj->isWireJunction() == false && jn->isWireJunction() == false)
		return;
	if (oj->absPositionDistance(jn) < 1) {
		jn->addLink(oj);
		oj->addLink(jn);
	}
}
void CSimulation::matchJunction_r(class CShape *sh, class CJunction *jn) {
	for (int i = 0; i < sh->getShapesCount(); i++) {
		CShape *s = sh->getShape(i);
		tryMatchJunction(jn, dynamic_cast<CJunction*>(s));
	}
}

void CSimulation::matchJunction(class CJunction *jn) {
	jn->clearLinks();
	for (int i = 0; i < wires.size(); i++) {
		CWire *w = wires[i];
		for (int j = 0; j < w->getJunctionsCount(); j++) {
			CJunction *oj = w->getJunction(j);
			tryMatchJunction(jn, oj);
		}
	}
	for (int i = 0; i < objects.size(); i++) {
		CShape *s = objects[i];
		matchJunction_r(s, jn);
	}
}
void CSimulation::destroyObject(CShape *s) {
	CJunction *j = 0;
	CEdge *ed = 0;
	CShape *p = s->getParent();
	CWire *w = dynamic_cast<CWire*>(p);
	if (w) {
		j = dynamic_cast<CJunction*>(s);
		if (j) {
		}
		ed = dynamic_cast<CEdge*>(s);
		if (ed) {
			delete w;
			wires.erase(std::remove(wires.begin(), wires.end(), w), wires.end());
		}
	}

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
	//glClearColor(1.f, 0.9f, 1.f, 0.f);
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
