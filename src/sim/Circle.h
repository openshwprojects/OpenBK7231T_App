#ifndef __CIRCLE_H__
#define __CIRCLE_H__

#include "sim_local.h"
#include "Shape.h"

class CCircle : public CShape {
	float radius;
	int corners;
	virtual void recalcBoundsSelf();
	void rotateDegreesAround_internal(float f, const Coord &p);
public:
	CCircle() {
		radius = 0;
		corners = 0;
	}
	CCircle(float _x, float _y, float _r, int _corners = 20) {
		this->setPosition(_x, _y);
		this->radius = _r;
		this->corners = _corners;
	}
	virtual CShape *cloneShape();
	virtual const char *getClassName() const {
		return "CCircle";
	}
	virtual void drawShape();
};

#endif // __CIRCLE_H__
