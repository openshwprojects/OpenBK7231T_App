#ifdef WINDOWS
#include "Controller_Bulb.h"
#include "Shape.h"
#include "Junction.h"


void CControllerBulb::onDrawn() {
	if (a->shouldLightUpBulb(b)) {
		shape->setActive(true);
	}
	else {
		shape->setActive(false);
	}

}

#endif
