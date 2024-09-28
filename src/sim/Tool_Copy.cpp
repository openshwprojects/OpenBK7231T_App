#ifdef WINDOWS

#include "Tool_Copy.h"
#include "Simulator.h"
#include "Shape.h"
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
		if (s != 0) {
			// can it be copied? you can't copy wires
			if (s->isWireJunction() == false && s->isWire() == false) {
				copyingObject = s->cloneShape();
			}
		}
	}
	else {
		sim->markAsModified();
		copyingObject->snapToGrid();
		sim->getSim()->addObject(copyingObject);
		copyingObject = 0;
	}

}
void Tool_Copy::drawTool() {
	sim->getCursorMgr()->setCursor(SDL_SYSTEM_CURSOR_ARROW);
	Coord nowMousePos = GetMousePosWorld();
	Coord delta = nowMousePos - prevMousePos;
	if (copyingObject) {
		copyingObject->translate(delta);
		copyingObject->drawWithChildren(0);
		sim->markAsModified();
	}
	prevMousePos = nowMousePos;
}


#endif

