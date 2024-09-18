#ifdef WINDOWS
#include "Tool_Wire.h"
#include "Simulator.h"
#include "Simulation.h"
#include "CursorManager.h"
#include "Junction.h"
#include "Wire.h"

Tool_Wire::Tool_Wire() {
	newWire = 0;
	bActive = false;
}
void Tool_Wire::onKeyDown(int button) {
	if (button == SDLK_ESCAPE) {
		bActive = false;
		newWire = 0;
	}
}
void Tool_Wire::onMouseDown(const Coord &pos, int button) {
	if (button == SDL_BUTTON_RIGHT) {
		bSideness = !bSideness;
	}
	if (button == SDL_BUTTON_LEFT) {
		Coord curPos = roundToGrid(GetMousePosWorld());
		if (bActive) {
#if 1				
			sim->markAsModified();
			CWire *newWireOld = sim->getSim()->addWire(a, b);
			newWire = sim->getSim()->addWire(b, c);
			if (newWire != 0) {
				CJunction *jun = newWire->getJunctionForPos(c);
				if (jun->getLinksCount() > 0) {
					// hit target, stop wire
					bActive = false;
				}
			}
			else if (newWireOld != 0) {
				CJunction *jun = newWireOld->getJunctionForPos(b);
				if (jun->getLinksCount() > 0) {
					// hit target, stop wire
					bActive = false;
				}
			}
#else
			if (newWire == 0) {
				newWire = sim->getSim()->addWire(a, b);
				newWire->addPoint(c);
			}
			else {
				newWire->addPoint(b);
				newWire->addPoint(c);
			}
#endif
			basePos = curPos;
		}
		else {
			basePos = curPos;
			bActive = true;
		}
	}
}
void Tool_Wire::drawTool() {
	Coord m;
	sim->getCursorMgr()->setCursor(SDL_SYSTEM_CURSOR_CROSSHAIR);
	m = roundToGrid(GetMousePosWorld());
	if (bActive) {
		if (0) {
			glBegin(GL_LINES);
			glVertex2fv(basePos);
			glVertex2fv(m);
			glEnd();
		}
		a = basePos;
		b = basePos;
		if (bSideness) {
			b.setX(m.getX());
		}
		else {
			b.setY(m.getY());
		}
		c = m;
		glBegin(GL_LINE_STRIP);
		glVertex2fv(a);
		glVertex2fv(b);
		glVertex2fv(c);
		glEnd();

	}
}


#endif


