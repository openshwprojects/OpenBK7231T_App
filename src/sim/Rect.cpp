#ifdef WINDOWS
#include "Rect.h"




CShape *CRect::cloneShape() {
	CRect *n = new CRect();
	n->pos = this->pos;
	n->w = this->w;
	n->h = this->h;
	this->cloneShapeTo(n);
	return n;
}
void CRect::drawShape() {
	if (bFill) {
		glColor3fv(getFillColor());
		glBegin(GL_QUADS);
		glVertex2f(getX(), getY());
		glVertex2f(getX() + w, getY());
		glVertex2f(getX() + w, getY() + h);
		glVertex2f(getX(), getY() + h);
		glEnd();
	}
	else {
		g_style_shapes.apply();
		glBegin(GL_LINE_LOOP);
		glVertex2f(getX(), getY());
		glVertex2f(getX() + w, getY());
		glVertex2f(getX() + w, getY() + h);
		glVertex2f(getX(), getY() + h);
		glEnd();
	}
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
