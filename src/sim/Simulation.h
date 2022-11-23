#ifndef __SIMULATION_H__
#define __SIMULATION_H__

#include "sim_local.h"

class CSimulation {
	std::vector<class CObject*> objects;
	std::vector<class CWire*> wires;

public:

	void recalcBounds();
	class CObject *generateBulb();
	class CObject *generateWB3S();
	class CObject *generateButton();
	class CObject *generateTest();
	class CObject *generateGND();
	void createDemo();
	void drawSim();
	class CObject *addObject(CObject *o);
	class CWire *addWire(const class Coord &a, const class Coord &b);
	class CShape *findShapeByBoundsPoint(const class Coord &p);
	void destroyObject(CShape *s);
	void tryMatchJunction(class CJunction *jn, class CJunction *other);
	void matchJunction_r(class CShape *sh, class CJunction *j);
	void matchJunction(class CJunction *j);
	void matchJunction(class CWire *w);
	void matchJunctionsOf_r(class CShape *s);
	int drawTextStats(int h);
};

#endif