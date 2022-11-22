#ifndef __LINE_H__
#define __LINE_H__

#include "sim_local.h"


class CLine : public CShape {
	float x2, y2;
public:
	CLine(float _x, float _y, float _x2, float _y2) {
		this->setPosition(_x, _y);
		this->x2 = _x2;
		this->y2 = _y2;
	}
	virtual void translate(float oX, float oY) {
		pos.add(oX, oY);
		x2 += oX;
		y2 += oY;
	}
	virtual void recalcBoundsSelf();
	virtual void drawShape();
};

#endif // __LINE_H__
