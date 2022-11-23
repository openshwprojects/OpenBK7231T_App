#ifdef WINDOWS
#include "Simulation.h"
#include "Shape.h"
#include "Object.h"
#include "Wire.h"
#include "Controller_Button.h"
#include "Junction.h"


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
	//addObject(new CObject(new CCircle(800, 500, 100)));
	addObject(generateBulb())->setPosition(800, 400);
	recalcBounds();
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
	o->addJunction(-40, -10, "pad_a");
	o->addJunction(40, -10, "pad_b");
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

class CObject *CSimulation::generateBulb() {
	float bulb_radius = 20.0f;

	CObject *o = new CObject();
	o->addText(-40, -25, "Bulb");
	o->addCircle(0, 0, bulb_radius);
	o->addJunction(-bulb_radius, 0)->addText(-5, -5, "");
	o->addJunction(bulb_radius, 0)->addText(-25, -5, "");
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
	else {
		CObject *o = dynamic_cast<CObject*>(s);
		if (o != 0) {
			delete o;
			objects.erase(std::remove(objects.begin(), objects.end(), o), objects.end());
		}
	}

}

#endif


