#ifndef __SIMULATION_H__
#define __SIMULATION_H__

#include "sim_local.h"


class CSimulation {
	class CSimulator *sim;
	// all allocated objects (must be fried later)
	TArray<class CShape*> objects;
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
	class CShape *findDeepText_r(const class Coord &p, class CShape *cur);
public:
	template <typename T>
	T *findFirstControllerOfType() {
		for (int i = 0; i < objects.size(); i++) {
			CControllerBase *c = objects[i]->getController();
			T *r = dynamic_cast<T*>(c);
			if (r)
				return r;
		}
		return 0;
	}
	template <typename T>
	TArray<T*> findControllersOfType() {
		TArray<T*> ret;
		for (int i = 0; i < objects.size(); i++) {
			CControllerBase *c = objects[i]->getController();
			T *r = dynamic_cast<T*>(c);
			if (r) {
				ret.push_back(r);
			}
		}
		return ret;
	}
	void setSimulator(class CSimulator *ssim) {
		this->sim = ssim;
	}
	int getObjectsCount() const {
		return objects.size();
	}
	class CShape *getObject(int i) {
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
	void createDemo();
	void createDemoOnlyWB3S();
	void matchAllJunctions();
	void drawSim();
	class CShape *addObject(class CShape *o);
	class CText *addText(const class Coord &p, const char *txt);
	class CWire *addWire(const class Coord &a, const class Coord &b);
	class CWire *addWire(float x0, float y0, float x1, float y1);
	class CShape *findShapeByBoundsPoint(const class Coord &p, bool bIncludeDeepText = false);
	void destroyObject(CShape *s);
	void tryMatchJunction(class CJunction *jn, class CJunction *other);
	void matchJunction_r(class CShape *sh, class CJunction *j);
	void matchJunction(class CJunction *j);
	void matchJunction(class CWire *w);
	void matchJunctionsOf_r(class CShape *s);
	float drawTextStats(float h);
};

#endif