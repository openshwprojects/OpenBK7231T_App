#ifdef WINDOWS
#include "Text.h"

void CText::drawShape() {
	drawText(getX(), getY(), txt.c_str());
}




void CText::rotateDegreesAround_internal(float f, const Coord &p) {
	//pos = pos.rotateDegreesAround(f, p);
}



#endif

