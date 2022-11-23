#ifndef __WIRE_H__
#define __WIRE_H__

#include "sim_local.h"
#include "Shape.h"

class CWire : public CShape {
	std::vector<class CJunction*> junctions;
	std::vector<class CEdge*> edges;

public:
	CWire(const Coord &a, const Coord &b);
	virtual ~CWire();
	void drawWire();
	void addPoint(const Coord &p); 
	class CShape* findSubPart(const Coord &p);
	int getJunctionsCount() const {
		return junctions.size();
	}
	class CJunction*getJunction(int j) {
		return junctions[j];
	}
	virtual bool isWire() const {
		return true;
	}
};

#endif // __WIRE_H__