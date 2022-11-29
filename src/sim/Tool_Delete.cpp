#ifdef WINDOWS
#include "Tool_Delete.h"
#include "Simulator.h"
#include "Shape.h"
#include "CursorManager.h"


Tool_Delete::Tool_Delete() {
	currentTarget = 0;
}
void Tool_Delete::onMouseDown(const Coord &pos, int button) {
	if (currentTarget != 0) {
		sim->markAsModified();
		sim->destroyObject(currentTarget);
	}
}
void Tool_Delete::onEnd() {
	sim->getCursorMgr()->setCursor(SDL_SYSTEM_CURSOR_ARROW);
}
void Tool_Delete::drawTool() {
	currentTarget = sim->getShapeUnderCursor();
	if (currentTarget) {
		sim->getCursorMgr()->setCursor(SDL_SYSTEM_CURSOR_NO);
	}
	else {
		sim->getCursorMgr()->setCursor(SDL_SYSTEM_CURSOR_ARROW);
	}
}


#endif

