#ifndef __JUNCTION_H__
#define __JUNCTION_H__

#include "sim_local.h"
#include "Coord.h"

class CEdge : public CShape {
	CJunction *a, *b;
public:
	CEdge(CJunction * _a, CJunction * _b) {
		this->a = _a;
		this->b = _b;
	}
	virtual void translate(const Coord &o);
	const Coord &getPositionA() const;
	const Coord &getPositionB() const;
};

class CJunction : public CShape {
	std::string name;
	std::vector<class CJunction *> linked;
public:
	CJunction(int _x, int _y, const char *s) {
		this->setPosition(_x, _y);
		this->name = s;
	}
	virtual ~CJunction();
	void rotateDegreesAround_internal(float f, const Coord &p);
	void unlink(class CJunction *o);
	bool isWireJunction() const;
	void clearLinks() {
		linked.clear();
	}
	virtual bool isJunction() const {
		return true;
	}
	bool hasLinkedOnlyWires() const;
	virtual void translate(const Coord &o);
	void translateLinked(const Coord &o);
	void addLink(CJunction *j) {
		linked.push_back(j);
	}
	virtual void drawShape();
};


#endif // __JUNCTION_H__