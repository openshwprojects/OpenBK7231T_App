#ifndef __SIMULATION_H__
#define __SIMULATION_H__

#include "sim_local.h"

class CSimulation {
	TArray<class CObject*> objects;
	TArray<class CWire*> wires;

	void removeJunctions(class CShape *s);
	void removeJunction(class CJunction *ju);
	bool isJunctionOnList(class CJunction *ju);
	void registerJunction(class CJunction *ju);
	void registerJunctions(class CWire *w);
	void registerJunctions(class CShape *s);
	void floodJunctions(CJunction *ju, float voltage);
	TArray<class CJunction*> junctions;
public:

	int getJunctionsCount() const {
		return junctions.size();
	}
	class CJunction *getJunction(int i) {
		return junctions[i];
	}

	void recalcBounds();
	class CObject *generateBulb();
	class CObject *generateWB3S();
	class CObject *generateButton();
	class CObject *generateTest();
	class CObject *generateGND();
	class CObject *generateVDD();
	void createDemo();
	void matchAllJunctions();
	void drawSim();
	class CObject *addObject(class CObject *o);
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