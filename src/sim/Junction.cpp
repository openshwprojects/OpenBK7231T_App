#ifdef WINDOWS
#include "Junction.h"

CJunction::~CJunction() {
	for (int i = 0; i < linked.size(); i++) {
		CJunction *oj = linked[i];
		oj->unlink(this);
	}
}
bool CJunction::isWireJunction() const {
	return getParent()->isWire();
}
void CJunction::unlink(class CJunction *o) {
	linked.erase(std::remove(linked.begin(), linked.end(), o), linked.end());
}
bool CJunction::hasLinkedOnlyWires() const {
	for (int i = 0; i < linked.size(); i++) {
		CJunction *oj = linked[i];
		if (oj->isWireJunction() == false)
			return false;
	}
	return true;
}
void CJunction::translateLinked(const Coord &o) {
	for (int i = 0; i < linked.size(); i++) {
		CJunction *oj = linked[i];
		oj->setPosition(oj->getPosition() + o);
	}
}
void CJunction::translate(const Coord &o) {
	if (hasLinkedOnlyWires()) {
		CShape::translate(o);
		translateLinked(o);
	}
}
void CJunction::drawShape() {
	glPointSize(8.0f);
	glColor3f(0, 1, 0);
	glBegin(GL_POINTS);
	glVertex2f(getX(), getY());
	glEnd();
	glColor3f(1, 1, 1);
}
void CJunction::rotateDegreesAround_internal(float f, const Coord &p) {
	pos = pos.rotateDegreesAround(f, p);
}




#endif

