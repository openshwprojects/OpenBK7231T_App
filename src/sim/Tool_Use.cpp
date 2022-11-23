#ifdef WINDOWS
#include "Tool_Use.h"
#include "Simulator.h"
#include "Shape.h"
#include "CursorManager.h"
#include "Controller_Base.h"

void Tool_Use::onMouseDown(const Coord &pos, int button) {

}

void Tool_Use::onEnd() {
	sim->getCursorMgr()->setCursor(SDL_SYSTEM_CURSOR_ARROW);
}
void Tool_Use::drawTool() {
	currentTarget = sim->getShapeUnderCursor();
	if (currentTarget) {
		sim->getCursorMgr()->setCursor(SDL_SYSTEM_CURSOR_HAND);
	}
	else {
		sim->getCursorMgr()->setCursor(SDL_SYSTEM_CURSOR_ARROW);
	}
	if (currentTarget != 0) {
		CControllerBase *cntrl = currentTarget->getController();
		if (cntrl != 0) {
			if (sim->isMouseButtonHold(SDL_BUTTON_LEFT)) {
				cntrl->sendEvent(EVE_LMB_HOLD);
			}
		}
	}
}


#endif
