#ifdef WINDOWS
#include "Line.h"

void CLine::rotateDegreesAround_internal(float f, const Coord &p) {
	pos = pos.rotateDegreesAround(f, p);
	pos2 = pos2.rotateDegreesAround(f, p);
}
inline float fMAX(float a, float b) {
	if (a > b)
		return a;
	return b;
}
inline float fMIN(float a, float b) {
	if (a < b)
		return a;
	return b;
}
float CLine::moveTowards(const Coord &tg, float dt) {
	const float l2 = pos.distSq(pos2);  // i.e. |w-v|^2 -  avoid a sqrt
	float t = fMAX(0, fMIN(1, Coord::dot(tg - pos, pos2 - pos) / l2));
	const Coord projection = pos + t * (pos2 - pos);  // Projection falls on the segment
	Coord dir = tg - projection;
	float rem = dir.normalize();
	if (rem < dt) {
	}
	else {
		pos += dir * dt;
		pos2 += dir * dt;
	}
	return rem;
}
void CLine::recalcBoundsSelf() {
	bounds.clear();
	bounds.addPoint(Coord(0, 0));
	bounds.addPoint(pos2 - getPosition());
}

CShape *CLine::cloneShape() {
	CLine *n = new CLine();
	n->pos = this->pos;
	n->pos2 = this->pos2;
	this->cloneShapeTo(n);
	return n;
}
void CLine::drawShape() {
	g_style_shapes.apply();
	glBegin(GL_LINES);
	glVertex2f(getX(), getY());
	glVertex2f(pos2.getX(), pos2.getY());
	glEnd();
}


#endif

