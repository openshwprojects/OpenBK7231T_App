#ifdef WINDOWS
#include "Junction.h"

CJunction::~CJunction() {
	for (int i = 0; i < linked.size(); i++) {
		CJunction *oj = linked[i];
		oj->unlink(this);
	}
}
float CJunction::drawInformation2D(float x, float h) {
	h = CShape::drawInformation2D(x, h);
	h = drawText(x, h, "Position: %f %f", getX(), getY());
	return h;
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
bool CJunction::shouldLightUpBulb(class CJunction *other) {
	float v = other->getVoltage();
	float v2 = this->getVoltage();
	if (abs(v - v2) > 1.5f)
		return true;
	return false;
}
void CJunction::translateLinked(const Coord &o) {
	for (int i = 0; i < linked.size(); i++) {
		CJunction *oj = linked[i];
		oj->setPosition(oj->getPosition() + o);
	}
}
void CJunction::setPosLinked(const Coord &o) {
	for (int i = 0; i < linked.size(); i++) {
		CJunction *oj = linked[i];
		oj->setPosition(o);
	}
}
void CJunction::translate(const Coord &o) {
	if (hasLinkedOnlyWires()) {
		CShape::translate(o);
		setPosLinked(this->getPosition());
	}
}
void CJunction::drawShape() {
	glPointSize(8.0f);
	if (voltage < 0) {
		glColor3f(0, 1, 0);
	} else if (voltage < 1) {
		glColor3f(0, 0, 1);
	}
	else {
		glColor3f(1, 0, 0);
	}
	//glColor3f(0, 1, 0);
	glBegin(GL_POINTS);
	glVertex2f(getX(), getY());
	glEnd();
	glColor3f(1, 1, 1);
}
void CJunction::rotateDegreesAround_internal(float f, const Coord &p) {
	pos = pos.rotateDegreesAround(f, p);
}




#endif

