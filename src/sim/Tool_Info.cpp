#ifdef WINDOWS
#include "Tool_Info.h"
#include "Simulator.h"
#include "Shape.h"
#include "CursorManager.h"


Tool_Info::Tool_Info() {
	currentTarget = 0;
}
void Tool_Info::onMouseDown(const Coord &pos, int button) {

}
void Tool_Info::drawTool() {
	currentTarget = sim->getShapeUnderCursor();
	if (currentTarget) {
		sim->getCursorMgr()->setCursor(SDL_SYSTEM_CURSOR_NO);
	}
	else {
		sim->getCursorMgr()->setCursor(SDL_SYSTEM_CURSOR_ARROW);
	}
	Coord pos = GetMousePosWorld();
	float h = pos.getY();
	if (currentTarget) {
		h = currentTarget->drawInformation2D(pos.getX(), h);
		//h = drawString(pos.getX(), h, "Nothing");
	}
	else {
		h = drawText(NULL, pos.getX(), h, "Nothing");
	}
}


#endif

