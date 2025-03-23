#ifndef __SOLVER_H__
#define __SOLVER_H__

#include "sim_local.h"

class CSolver {
	class CSimulation *sim;

	void floodJunctions(class CJunction *ju, float voltage, float duty, int depth = 0);
public:
	void setSimulation(class CSimulation *p) {
		sim = p;
	}
	void solveVoltages();
	bool hasPath(class CJunction *a, class CJunction *b);
};

#endif