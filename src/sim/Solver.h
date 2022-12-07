#ifndef __SOLVER_H__
#define __SOLVER_H__

#include "sim_local.h"

class CSolver {
	class CSimulation *sim;

	void floodJunctions(class CJunction *ju, float voltage, float duty);
public:
	void setSimulation(class CSimulation *p) {
		sim = p;
	}
	void solveVoltages();
};

#endif