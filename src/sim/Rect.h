#ifndef __RECT_H__
#define __RECT_H__

#include "sim_local.h"
#include "Shape.h"
#include "Coord.h"

class CRect : public CShape {
	float w, h;

	virtual void recalcBoundsSelf();
	void rotateDegreesAround_internal(float f, const class Coord &p);
public:
	CRect() {
		this->w = 0;
		this->h = 0;
	}
	CRect(float _x, float _y, float _w, float _h) {
		this->setPosition(_x, _y);
		this->w = _w;
		this->h = _h;
	}
	virtual CShape *cloneShape();
	virtual const char *getClassName() const {
		return "CRect";
	}
	void setFromTwoPoints(const class Coord &a, const class Coord &b) {
		setPosition(a);
		w = b.getX() - a.getX();
		h = b.getY() - a.getY();
	}
	Coord getCorner() const {
		return Coord(getX() + w, getY() + h);
	}
	virtual void drawShape();
};

#endif // __RECT_H__
