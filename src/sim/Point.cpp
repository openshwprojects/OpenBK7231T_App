#ifdef WINDOWS
#include "Point.h"




CShape *CPoint::cloneShape() {
	CPoint *n = new CPoint();
	n->pos = this->pos;
	this->cloneShapeTo(n);
	return n;
}
void CPoint::drawShape() {
		g_style_shapes.apply();
		glBegin(GL_POINTS);
		glVertex2f(getX(), getY());
		glEnd();
}


void CPoint::rotateDegreesAround_internal(float f, const Coord &p) {
	Coord a = getPosition();
	a = a.rotateDegreesAround(f, p);
	setPosition(a);
}
void CPoint::recalcBoundsSelf() {
	bounds.clear();
	bounds.addPoint(Coord(0, 0));
}


#endif
