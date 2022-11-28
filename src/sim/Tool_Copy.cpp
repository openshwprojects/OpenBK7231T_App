#ifdef WINDOWS

#include "Tool_Copy.h"
#include "Simulator.h"
#include "Shape.h"
#include "Object.h"
#include "CursorManager.h"
#include "Simulation.h"



Tool_Copy::Tool_Copy() {
	copyingObject = 0;
}
Tool_Copy::~Tool_Copy() {
	if (copyingObject) {
		delete copyingObject;
	}
}

void Tool_Copy::onMouseDown(const Coord &pos, int button) {
	if (copyingObject == 0) {
		CShape *s = sim->getShapeUnderCursor();
		CObject *o = dynamic_cast<CObject*>(s);
		if (o != 0) {
			copyingObject = o->cloneObject();
		}
	}
	else {
		copyingObject->snapToGrid();
		sim->getSim()->addObject(copyingObject);
		copyingObject = 0;
	}

}
void Tool_Copy::drawTool() {
	sim->getCursorMgr()->setCursor(SDL_SYSTEM_CURSOR_ARROW);
	Coord nowMousePos = GetMousePos();
	Coord delta = nowMousePos - prevMousePos;
	if (copyingObject) {
		copyingObject->translate(delta);
		copyingObject->drawWithChildren(0);
	}
	prevMousePos = nowMousePos;
}


#endif

