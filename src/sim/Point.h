#ifndef __POINT_H__
#define __POINT_H__

#include "sim_local.h"
#include "Shape.h"

class CPoint : public CShape {
	virtual void recalcBoundsSelf();
	void rotateDegreesAround_internal(float f, const Coord &p);
public:
	CPoint() {
	}
	CPoint(float _x, float _y) {
		this->setPosition(_x, _y);
	}
	virtual CShape *cloneShape();
	virtual const char *getClassName() const {
		return "CPoint";
	}
	virtual void drawShape();
};

#endif // __POINT_H__
