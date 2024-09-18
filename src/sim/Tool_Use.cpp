#ifdef WINDOWS
#include "Tool_Use.h"
#include "Simulator.h"
#include "Shape.h"
#include "CursorManager.h"
#include "Controller_Base.h"

void Tool_Use::onMouseDown(const Coord &pos, int button) {
	currentTarget = sim->getShapeUnderCursor();
	prevPos = GetMousePosWorld();

	if (currentTarget != 0) {
		Coord curPos = GetMousePosWorld();
		Coord delta = curPos - prevPos;
		prevPos = curPos;
		CControllerBase *cntrl = currentTarget->getController();
		if (cntrl != 0) {
			cntrl->sendEvent(EVE_LMB_DOWN, delta);
		}
	}
}
void Tool_Use::onMouseUp(const Coord &pos, int button) {
	currentTarget = 0;

}

void Tool_Use::onEnd() {
	sim->getCursorMgr()->setCursor(SDL_SYSTEM_CURSOR_ARROW);
}
void Tool_Use::drawTool() {
	if (currentTarget) {
		sim->getCursorMgr()->setCursor(SDL_SYSTEM_CURSOR_HAND);
	}
	else {
		CShape *currentUnder = sim->getShapeUnderCursor();
		if (currentUnder) {
			sim->getCursorMgr()->setCursor(SDL_SYSTEM_CURSOR_HAND);
		}
		else {
			sim->getCursorMgr()->setCursor(SDL_SYSTEM_CURSOR_ARROW);
		}
	}
	if (currentTarget != 0) {
		Coord curPos = GetMousePosWorld();
		Coord delta = curPos - prevPos;
		prevPos = curPos;
		CControllerBase *cntrl = currentTarget->getController();
		if (cntrl != 0) {
			if (sim->isMouseButtonHold(SDL_BUTTON_LEFT)) {
				cntrl->sendEvent(EVE_LMB_HOLD, delta);
			}
		}
	}
}


#endif
