#ifdef WINDOWS
#include "Text.h"

CShape *CText::cloneShape() {
	CText *r = new CText();
	r->pos = this->pos;
	r->txt = this->txt;
	this->cloneShapeTo(r);
	return r;
}


void CText::drawShape() {
	drawText(getX(), getY(), txt.c_str());
}



float CText::drawPrivateInformation2D(float x, float h) {
	h = drawText(x, h, "Text: %s", this->txt.c_str());
	return h;
}


void CText::rotateDegreesAround_internal(float f, const Coord &p) {
	//pos = pos.rotateDegreesAround(f, p);
}



#endif

