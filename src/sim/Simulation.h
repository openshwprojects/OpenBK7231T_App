#ifndef __SIMULATION_H__
#define __SIMULATION_H__

#include "sim_local.h"

class CSimulation {
	std::vector<class CObject*> objects;
	std::vector<class CWire*> wires;

public:

	class CObject *generateWB3S();
	class CObject *generateButton();
	void createDemo();
	void drawSim();
	class CObject *addObject(CObject *o);
	class CWire *addWire(const class Coord &a, const class Coord &b);
};

#endif