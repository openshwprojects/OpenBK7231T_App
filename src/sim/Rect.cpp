#ifdef WINDOWS
#include "Rect.h"




void CRect::drawShape() {
	glBegin(GL_LINE_LOOP);
	glVertex2f(getX(), getY());
	glVertex2f(getX() + w, getY());
	glVertex2f(getX() + w, getY() + h);
	glVertex2f(getX(), getY() + h);
	glEnd();
}


void CRect::rotateDegreesAround_internal(float f, const Coord &p) {
	Coord a = getPosition();
	Coord b = getCorner();
	a = a.rotateDegreesAround(f, p);
	b = b.rotateDegreesAround(f, p);
	setFromTwoPoints(a, b);
}
void CRect::recalcBoundsSelf() {
	bounds.clear();
	bounds.addPoint(Coord(0, 0));
	bounds.addPoint(Coord(w, h));
}


#endif
