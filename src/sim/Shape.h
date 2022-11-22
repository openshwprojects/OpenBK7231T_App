#ifndef __SHAPE_H__
#define __SHAPE_H__

#include "sim_local.h"

class CShape {
	std::vector<class CShape*> shapes;
protected:
	int x, y;
public:
	void setPosition(int nX, int nY) {
		x = nX;
		y = nY;
	}
	float getX() const {
		return x;
	}
	float getY() const {
		return y;
	}
	virtual void drawWithChildren();
	virtual void drawShape() { }

	class CShape* addLine(int x, int y, int x2, int y2);
	class CShape* addRect(int x, int y, int w, int h);
	class CShape* addText(int x, int y, const char *s);
	class CShape* addJunction(int x, int y, const char *name = "");
};

#endif 

