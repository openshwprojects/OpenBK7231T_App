#ifndef __RECT_H__
#define __RECT_H__

#include "sim_local.h"

class CRect : public CShape {
	float w, h;

	virtual void recalcBoundsSelf();
	void rotateDegreesAround_internal(float f, const Coord &p);
public:
	CRect(int _x, int _y, int _w, int _h) {
		this->setPosition(_x, _y);
		this->w = _w;
		this->h = _h;
	}
	void setFromTwoPoints(const Coord &a, const Coord &b) {
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
