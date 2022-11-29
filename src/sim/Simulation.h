#ifndef __SIMULATION_H__
#define __SIMULATION_H__

#include "sim_local.h"

class CSimulation {
	// all allocated objects (must be fried later)
	TArray<class CObject*> objects;
	// all allocated wires (must be fried later)
	TArray<class CWire*> wires;
	// only pointers to junctions that belongs to allocated objects or wires
	TArray<class CJunction*> junctions;

	void removeJunctions(class CShape *s);
	void removeJunction(class CJunction *ju);
	bool isJunctionOnList(class CJunction *ju);
	void registerJunction(class CJunction *ju);
	void registerJunctions(class CWire *w);
	void registerJunctions(class CShape *s);
public:

	int getObjectsCount() const {
		return objects.size();
	}
	class CObject *getObject(int i) {
		return objects[i];
	}
	int getWiresCount() const {
		return wires.size();
	}
	class CWire *getWires(int i) {
		return wires[i];
	}
	int getJunctionsCount() const {
		return junctions.size();
	}
	class CJunction *getJunction(int i) {
		return junctions[i];
	}

	void recalcBounds();
	class CObject *generateLED_CW();
	class CObject *generateLED_RGB();
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