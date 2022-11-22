#ifndef __RECT_H__
#define __RECT_H__

#include "sim_local.h"

class CRect : public CShape {
	int w, h;

	virtual void recalcBoundsSelf();
public:
	CRect(int _x, int _y, int _w, int _h) {
		this->setPosition(_x, _y);
		this->w = _w;
		this->h = _h;
	}
	virtual void CRect::drawShape();
};

#endif // __RECT_H__
