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
float Tool_Move::drawTextStats(float h) {
	if (currentTarget) {
		if (bMovingButtonHeld) {
			h = drawText(NULL, 20, h, "Moving %s", currentTarget->getClassName());
		}
		else {
			h = drawText(NULL, 20, h, "Last target %s", currentTarget->getClassName());
		}
	}
	else {
		h = drawText(NULL, 20, h, "No target");
	}
	return h;
}
void Tool_Move::onMouseDown(const Coord &pos, int button) {
	if (button == SDL_BUTTON_LEFT) {
		currentTarget = sim->getShapeUnderCursor();
		if (currentTarget) {
			prevPos = GetMousePosWorld();
			prevPos = roundToGrid(prevPos);
			//sim->getCursorMgr()->setCursor(SDL_SYSTEM_CURSOR_SIZEALL);
		}
		else {
			//sim->getCursorMgr()->setCursor(SDL_SYSTEM_CURSOR_ARROW);
		}
	}
	if (button == SDL_BUTTON_RIGHT) {
		if (currentTarget) {
			sim->markAsModified();
			currentTarget->rotateDegreesAroundSelf(90);
		}
	}

}
void Tool_Move::drawTool() {
	bMovingButtonHeld = sim->isMouseButtonHold(SDL_BUTTON_LEFT);
	if (bMovingButtonHeld) {
		if (currentTarget != 0) {
			Coord nowPos = GetMousePosWorld();
			nowPos = roundToGrid(nowPos);
			Coord delta = nowPos - prevPos;
			if (delta.isNonZero()) {
				prevPos = nowPos;
				sim->markAsModified();
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



