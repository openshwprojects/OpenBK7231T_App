#ifndef __JUNCTION_H__
#define __JUNCTION_H__

#include "sim_local.h"

class CJunction : public CShape {
	std::string name;
public:
	CJunction(int _x, int _y, const char *s) {
		this->setPosition(_x, _y);
		this->name = s;
	}
	virtual void drawShape();
};


#endif // __JUNCTION_H__