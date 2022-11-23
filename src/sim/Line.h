#ifndef __LINE_H__
#define __LINE_H__

#include "sim_local.h"


class CLine : public CShape {
	Coord pos2;
public:
	CLine(float _x, float _y, float _x2, float _y2) {
		this->setPosition(_x, _y);
		this->pos2.set(_x2, _y2);
	}
	virtual void translate(const Coord &p) {
		pos.add(p.getX(), p.getY());
		pos2.add(p.getX(), p.getY());
	}
	virtual void moveTowards(const Coord &tg, float dt);
	virtual void rotateDegreesAround_internal(float f, const Coord &p);
	virtual void recalcBoundsSelf();
	virtual void drawShape();
};

#endif // __LINE_H__
