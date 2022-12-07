#ifdef WINDOWS
#include "Circle.h"

CShape *CCircle::cloneShape() {
	CCircle *r = new CCircle();
	r->pos = this->pos;
	r->radius = this->radius;
	r->corners = this->corners;
	this->cloneShapeTo(r);
	return r;
}
void CCircle::recalcBoundsSelf() {
	bounds.clear();
	bounds.addPoint(Coord(0, 0));
	bounds.addPoint(-radius, radius);
	bounds.addPoint(radius, -radius);
	bounds.addPoint(-radius, -radius);
	bounds.addPoint(radius, radius);
}
void CCircle::rotateDegreesAround_internal(float f, const Coord &p) {
	pos = pos.rotateDegreesAround(f, p);
}
void CCircle::drawShape() {
	if (bFill) {
		glColor3fv(getFillColor());
		glBegin(GL_TRIANGLE_FAN);
		for (int i = 0; i < corners; i++) {
			float a = (2.0f * M_PI) * (i / (float)corners);
			Coord p = this->pos;
			p.addX(sin(a) * radius);
			p.addY(cos(a) * radius);
			glVertex2fv(p);
		}
		glEnd();
	}
	else {
		g_style_shapes.apply();
		glBegin(GL_LINE_LOOP);
		for (int i = 0; i < corners; i++) {
			float a = (2.0f * M_PI) * (i / (float)corners);
			Coord p = this->pos;
			p.addX(sin(a) * radius);
			p.addY(cos(a) * radius);
			glVertex2fv(p);
		}
		glEnd();
	}
}
#endif
