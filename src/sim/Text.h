#ifndef __TEXT_H__
#define __TEXT_H__

#include "sim_local.h"

class CText : public CShape {
	std::string txt;

	void rotateDegreesAround_internal(float f, const Coord &p);
public:
	CText(int _x, int _y, const char *s) {
		this->setPosition(_x, _y);
		this->txt = s;
	}
	virtual void drawShape();
};


#endif // __TEXT_H__
