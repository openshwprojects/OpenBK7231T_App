#ifdef WINDOWS
#include "Simulation.h"
#include "Shape.h"
#include "Object.h"
#include "Wire.h"
#include "Controller_Button.h"
#include "Controller_Bulb.h"
#include "Controller_SimulatorLink.h"
#include "Junction.h"


void CSimulation::removeJunctions(class CShape *s) {
	CJunction *j = dynamic_cast<CJunction*>(s);
	if (j != 0) {
		removeJunction(j);
	}
	CWire *w = dynamic_cast<CWire*>(s);
	if (w) {
		for (int i = 0; i < w->getJunctionsCount(); i++) {
			removeJunction(w->getJunction(i));
		}
	}
	for (int i = 0; i < s->getShapesCount(); i++) {
		CShape *s2 = s->getShape(i);
		removeJunctions(s2);
	}
}
void CSimulation::removeJunction(class CJunction *ju) {
	junctions.remove(ju);
}
int CSimulation::drawTextStats(int h) {
	h = drawText(10, h, "Objects %i, wires %i", objects.size(), wires.size());
	return h;
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
	if (a.dist(b) < 5) {
		return 0;
	}
	class CWire *cw = new CWire(a, b);
	wires.push_back(cw);
	matchJunction(cw);
	registerJunctions(cw);
	return cw;
}
bool CSimulation::isJunctionOnList(class CJunction *ju) {
	for (int i = 0; i < junctions.size(); i++) {
		if (junctions[i] == ju)
			return true;
	}
	return false;
}
void CSimulation::registerJunction(class CJunction *ju) {
	junctions.push_back(ju);
}
void CSimulation::registerJunctions(class CWire *w) {
	for (int i = 0; i < w->getJunctionsCount(); i++) {
		registerJunction(w->getJunction(i));
	}
}
void CSimulation::registerJunctions(class CShape *s) {
	CJunction *j = dynamic_cast<CJunction*>(s);
	if (j != 0) {
		registerJunction(j);
	}
	for (int i = 0; i < s->getShapesCount(); i++) {
		CShape *s2 = s->getShape(i);
		registerJunctions(s2);
	}
}
CObject * CSimulation::addObject(CObject *o) {
	objects.push_back(o);
	registerJunctions(o);
	return o;
}
void CSimulation::createDemo() {
	addObject(generateWB3S())->setPosition(300, 200);
	addObject(generateButton())->setPosition(500, 260)->rotateDegreesAroundSelf(180);
	addObject(generateTest())->setPosition(500, 400);
	addObject(generateGND())->setPosition(600, 420);
	addWire(Coord(460, 260), Coord(380, 260));
	addWire(Coord(540, 260), Coord(600, 260));
	addWire(Coord(600, 400), Coord(600, 260));


	addWire(Coord(500, 220), Coord(380, 220));
	addWire(Coord(500, 220), Coord(500, 160));
	addObject(generateBulb())->setPosition(500, 140)->rotateDegreesAroundSelf(90);
	addWire(Coord(500, 60), Coord(500, 120));
	addObject(generateVDD())->setPosition(500, 60);

	addWire(Coord(560, 240), Coord(380, 240));
	addObject(generateButton())->setPosition(600, 240)->rotateDegreesAroundSelf(180);
	addWire(Coord(640, 240), Coord(700, 240));
	addWire(Coord(700, 400), Coord(700, 240));
	addObject(generateGND())->setPosition(700, 420);
	//addObject(new CObject(new CCircle(800, 500, 100)));
	addObject(generateBulb())->setPosition(440, 140)->rotateDegreesAroundSelf(90);
	addWire(Coord(440, 60), Coord(440, 120));
	addWire(Coord(440, 200), Coord(440, 160));
	addWire(Coord(440, 200), Coord(380, 200));
	addObject(generateVDD())->setPosition(440, 60);
	matchAllJunctions();
	recalcBounds();
}
void CSimulation::matchAllJunctions() {
	for (int i = 0; i < wires.size(); i++) {
		CWire *w = wires[i];
		matchJunction(w);
	}
}



class CObject *CSimulation::generateVDD() {
	CObject *o = new CObject();
	o->addJunction(0, 0, "VDD");
	o->addLine(0, -20, 0, 0);
	o->addCircle(0, -30, 10);
	return o;
}
class CObject *CSimulation::generateGND() {
	CObject *o = new CObject();
	o->addJunction(0, -20, "GND");
	o->addLine(0, -20, 0, 0);
	o->addLine(-10, 0, 10, 0);
	o->addLine(-8, 2, 8, 2);
	o->addLine(-6, 4, 6, 4);
	o->addLine(-4, 6, 4, 6);
	return o;
}
class CObject *CSimulation::generateButton() {
	CObject *o = new CObject();
	CJunction *a = o->addJunction(-40, -10, "pad_a");
	CJunction *b = o->addJunction(40, -10, "pad_b");
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

	CControllerButton *btn = new CControllerButton(a,b);
	btn->setMover(mover);
	o->setController(btn);
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
	CControllerSimulatorLink *link = new CControllerSimulatorLink();
	o->setController(link);
	o->addText(-40, -25, "WB3S");
	o->addRect(-50, -20, 100, 180);
	struct PinDef_s {
		const char *name;
		int gpio;
	};
	PinDef_s wb3sPinsRight[] = {
		{ "TXD1", 11 },
		{ "RXD1", 10 },
		{ "PWM2", 8 },
		{ "PWM3", 9 },
		{ "RXD2", 1 },
		{ "TXD2", 0 },
		{ "PWM1", 7 },
		{ "GND", -1 }
	};
	PinDef_s wb3sPinsLeft[] = {
		{ "CEN", -1 },
		{ "ADC3", 23 },
		{ "EN", -1 },
		{ "P14", 14 },
		{ "PWM5", 26 },
		{ "PWM4", 24 },
		{ "PWM0", 6 },
		{ "VCC", -1 }
	};
	for (int i = 0; i < 8; i++) {
		int y = i * 20;
		o->addLine(50, y, 80, y);
		o->addLine(-50, y, -80, y);
		CJunction *a = o->addJunction(-80, y, "", wb3sPinsLeft[i].gpio);
		a->addText(-5, -5, wb3sPinsLeft[i].name);
		CJunction *b = o->addJunction(80, y, "", wb3sPinsRight[i].gpio);
		b->addText(-25, -5, wb3sPinsRight[i].name);
		link->addRelatedJunction(a);
		link->addRelatedJunction(b);
	}
	return o;
}

class CObject *CSimulation::generateBulb() {
	float bulb_radius = 20.0f;

	CObject *o = new CObject();
	o->addText(-40, -25, "Bulb");
	CShape *filler = o->addCircle(0, 0, bulb_radius);
	CShape *body = o->addCircle(0, 0, bulb_radius);
	filler->setFill(true);
	CJunction *a = o->addJunction(-bulb_radius, 0);
	a->addText(-5, -5, "");
	CJunction *b = o->addJunction(bulb_radius, 0);
	b->addText(-25, -5, "");
	CControllerBulb *bulb = new CControllerBulb(a, b);
	o->setController(bulb);
	bulb->setShape(filler);
	float mul = sqrt(2) * 0.5f;
	o->addLine(-bulb_radius * mul, -bulb_radius * mul, bulb_radius * mul, bulb_radius * mul);
	o->addLine(-bulb_radius * mul, bulb_radius * mul, bulb_radius * mul, -bulb_radius * mul);
	return o;
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
	// cannot connect two objects directly without wire
	// (in Eagle, such connection will automatically create wire, but let's keep it simple)
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
	removeJunctions(s);
	CWire *w = dynamic_cast<CWire*>(p);
	if (w) {
		j = dynamic_cast<CJunction*>(s);
		if (j) {
		}
		ed = dynamic_cast<CEdge*>(s);
		if (ed) {
			delete w;
			wires.remove(w);
		}
	}
	else {
		CObject *o = dynamic_cast<CObject*>(s);
		if (o != 0) {
			delete o;
			objects.remove(o);
		}
	}

}

#endif


