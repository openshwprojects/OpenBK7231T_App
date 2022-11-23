#ifdef WINDOWS
#include "Tool_Move.h"
#include "Simulator.h"
#include "Simulation.h"
#include "Shape.h"
#include "CursorManager.h"

Tool_Move::Tool_Move() {
	currentTarget = 0;
}
void Tool_Move::onMouseUp(const Coord &pos, int button) {
	//sim->getCursorMgr()->setCursor(SDL_SYSTEM_CURSOR_ARROW);

}
void Tool_Move::onEnd() {
	sim->getCursorMgr()->setCursor(SDL_SYSTEM_CURSOR_ARROW);
}
void Tool_Move::onMouseDown(const Coord &pos, int button) {
	if (button == SDL_BUTTON_LEFT) {
		currentTarget = sim->getShapeUnderCursor();
		if (currentTarget) {
			prevPos = GetMousePos();
			prevPos = roundToGrid(prevPos);
			//sim->getCursorMgr()->setCursor(SDL_SYSTEM_CURSOR_SIZEALL);
		}
		else {
			//sim->getCursorMgr()->setCursor(SDL_SYSTEM_CURSOR_ARROW);
		}
	}
	if (button == SDL_BUTTON_RIGHT) {
		if (currentTarget) {
			currentTarget->rotateDegreesAroundSelf(90);
		}
	}

}
void Tool_Move::drawTool() {
	if (sim->isMouseButtonHold(SDL_BUTTON_LEFT)) {
		if (currentTarget != 0) {
			Coord nowPos = GetMousePos();
			nowPos = roundToGrid(nowPos);
			Coord delta = nowPos - prevPos;
			if (delta.isNonZero()) {
				prevPos = nowPos;
				currentTarget->translate(delta);
				sim->getSim()->matchJunctionsOf_r(currentTarget);
			}

		}
	}
	else {
		CShape *o = sim->getShapeUnderCursor();
		if (o) {
			sim->getCursorMgr()->setCursor(SDL_SYSTEM_CURSOR_SIZEALL);
		}
		else {
			sim->getCursorMgr()->setCursor(SDL_SYSTEM_CURSOR_ARROW);
		}
	}

}



#endif



