#ifdef WINDOWS
#include "Shape.h"
#include "Circle.h"
#include "Rect.h"
#include "Line.h"
#include "Text.h"
#include "Junction.h"
#include "Controller_Base.h"
#include "Controller_Button.h"

CShape::~CShape() {
	for (unsigned int i = 0; i < shapes.size(); i++) {
		delete shapes[i];
	}
	shapes.clear();
	if (controller != 0) {
		delete controller;
		controller = 0;
	}
}
class CShape *CShape::findShapeByName_r(const char *name) {
	if (this->hasName(name))
		return this;
	for (unsigned int i = 0; i < shapes.size(); i++) {
		CShape *r = shapes[i]->findShapeByName_r(name);
		if (r != 0)
			return r;
	}
	return 0;
}
class CJunction *CShape::asJunction() {
	return dynamic_cast<CJunction*>(this);
}
class CText *CShape::asText() {
	return dynamic_cast<CText*>(this);
}
void CShape::snapToGrid() {
	Coord n = roundToGrid(getPosition());
	setPosition(n);
}
void CShape::cloneShapeTo(CShape *o) {
	o->name = this->name;
	o->pos = this->pos;
	o->bounds = this->bounds;
	o->bFill = this->bFill;
	o->rotationAccum = this->rotationAccum;
	for (unsigned int i = 0; i < shapes.size(); i++) {
		o->addShape(shapes[i]->cloneShape());
	}
	if (this->controller != 0) {
		o->setController(this->controller->cloneController(this,o));
	}
}
CShape *CShape::cloneShape() {
	CShape *r = new CShape();
	this->cloneShapeTo(r);
	return r;
}

float CShape::drawPrivateInformation2D(float x, float h) {
	h = drawText(NULL, x, h, "Position: %f %f", this->getX(), getY());
	return h;
}
float CShape::drawInformation2D(float x, float h) {
	h = drawText(NULL, x, h, "ClassName: %s, int. name %s", this->getClassName(), this->getName());
	h = drawPrivateInformation2D(x, h);
	if (shapes.size()) {
		h = drawText(NULL, x, h, "SubShapes: %i", shapes.size());
		for (unsigned int i = 0; i < shapes.size(); i++) {
			h = drawText(NULL, x + 20, h, "SubShape: %i/%i", i+1, shapes.size());
			h = shapes[i]->drawInformation2D(x + 40, h);
		}
	}
	return h;
}

void CShape::translateEachChild(float oX, float oY) {
	for (unsigned int i = 0; i < shapes.size(); i++) {
		shapes[i]->translate(oX, oY);
	}
}
bool CShape::hasWorldPointInside(const Coord &p)const {
	Coord loc = p - this->getPosition();
	return hasLocalPointInside(loc);
}
bool CShape::hasLocalPointInside(const Coord &p)const {
	if (bounds.isInside(p))
		return true;
	return false;
}
void CShape::rotateDegreesAround(float f, const Coord &p) {
	Coord p_i = p - this->getPosition();
	rotateDegreesAround_internal(f, p);
	if (controller != 0) {
		controller->rotateDegreesAround(f, p_i);
	}
	for (unsigned int i = 0; i < shapes.size(); i++) {
		shapes[i]->rotateDegreesAround(f, p_i);
	}
	recalcBoundsAll();
}
void CShape::rotateDegreesAroundSelf(float f) {
	//rotateDegreesAround(f, Coord(0, 0));
	rotateDegreesAround(f, getPosition());
	rotationAccum += f;
}
void CShape::recalcBoundsAll() {
	bounds.clear();
	this->recalcBoundsSelf();
	for (int i = 0; i < shapes.size(); i++) {
		shapes[i]->recalcBoundsAll();
		bounds.addBounds(shapes[i]->getBounds(), shapes[i]->getPosition());
	}
}
class CShape* CShape::addLine(float x, float y, float x2, float y2) {
	CLine *n = new CLine(x, y, x2, y2);
	addShape(n);
	return n;
}
class CShape* CShape::addShape(CShape *p) {
	p->parent = this;
	shapes.push_back(p);
	return p;
}
class CJunction* CShape::addJunction(float x, float y, const char *name, int gpio) {
	CJunction *n = new CJunction(x, y, name, gpio);
	addShape(n);
	return n;
}
class CShape* CShape::addCircle(float x, float y, float r) {
	CCircle *n = new CCircle(x, y, r);
	addShape(n);
	return n;
}
class CShape* CShape::addRect(float x, float y, float w, float h) {
	CRect *n = new CRect(x, y, w, h);
	addShape(n);
	return n;
}
class CShape* CShape::addText(float x, float y, const char *s, bool bDeepText, bool bAllowNewLine) {
	CText *n = new CText(x, y, s);
	n->setDeepText(bDeepText);
	n->setAllowNewLine(bAllowNewLine);
	addShape(n);
	return n;
}
Coord CShape::getAbsPosition() const {
	Coord ofs = this->getPosition();
	const CShape *p = this->getParent();
	while (p) {
		ofs += p->getPosition();
		p = p->getParent();
	}
	return ofs;
}
void CShape::translate(const Coord &ofs) {
	pos += ofs;
	// special handling for connected wires
	for (unsigned int i = 0; i < shapes.size(); i++) {
		CShape *o = shapes[i];
		if (o->isJunction() == false)
			continue;
		CJunction *j = dynamic_cast<CJunction*>(o);
		j->translateLinked(ofs);
	}
}

void CShape::rotateDegreesAround_internal(float f, const Coord &p) {
	pos = pos.rotateDegreesAround(f, p);
}



float CShape::moveTowards(const Coord &tg, float dt) {
	pos = pos.moveTowards(tg, dt);
	return pos.dist(tg);
}
void CShape::drawWithChildren(int depth) {
	if (bActive == false) {
		return;
	}
	if (controller != 0) {
		controller->onDrawn();
	}
	drawShape();
	glPushMatrix();
	glTranslatef(getX(), getY(), 0);
	for (unsigned int i = 0; i < shapes.size(); i++) {
		shapes[i]->drawWithChildren(depth + 1);
	}
	//recalcBoundsAll();
	if (depth == 0) {
		glColor3f(0, 1, 0);
		glLineWidth(0.5f);
		glBegin(GL_LINE_LOOP);
		Bounds bb = bounds;
		bb.extend(4);
		for (int i = 0; i < 4; i++) {
			glVertex2fv(bb.getCorner(i));
		}
		glEnd();
	}
	glPopMatrix();
}



#endif
