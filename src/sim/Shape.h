#ifndef __SHAPE_H__
#define __SHAPE_H__

#include "sim_local.h"
#include "Coord.h"
#include "Bounds.h"

class CShape {
	std::vector<class CShape*> shapes;
	class CControllerBase *controller;

	virtual void recalcBoundsSelf() {}
protected:
	Bounds bounds;
	Coord pos;
public:
	CShape() {
		controller = 0;
	}
	void moveTowards(const Coord &tg, float dt);
	void setController(CControllerBase *c) {
		controller = c;
	}
	CControllerBase *getController() {
		return controller;
	}
	const Bounds &getBounds() const {
		return bounds;
	}
	const Coord &getPosition() const {
		return pos;
	}
	void setPosition(float nX, float nY) {
		pos.set(nX, nY);
	}
	virtual void translate(float oX, float oY) {
		pos.add(oX, oY);
	}
	virtual void translate(const Coord &o) {
		pos += o;
	}
	float getX() const {
		return pos.getX();
	}
	float getY() const {
		return pos.getY();
	}
	virtual void drawWithChildren(int depth);
	virtual void drawShape() { }

	class CShape* addLine(int x, int y, int x2, int y2);
	class CShape* addRect(int x, int y, int w, int h);
	class CShape* addText(int x, int y, const char *s);
	class CShape* addJunction(int x, int y, const char *name = "");
	class CShape* addShape(CShape *p);
	void recalcBoundsAll();
	bool hasWorldPointInside(const Coord &p)const;
	bool hasLocalPointInside(const Coord &p)const;
	void translateEachChild(float oX, float oY);
};

#endif 

