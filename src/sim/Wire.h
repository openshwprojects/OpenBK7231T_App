#ifndef __WIRE_H__
#define __WIRE_H__

#include "sim_local.h"

class CWire {
	std::vector<class CJunction*> junctions;

public:
	CWire(const Coord &a, const Coord &b);
	void drawWire();
	void addPoint(const Coord &p);
};

#endif // __WIRE_H__