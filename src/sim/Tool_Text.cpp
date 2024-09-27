#ifdef WINDOWS
#include "Tool_Text.h"
#include "Simulator.h"
#include "Simulation.h"
#include "Shape.h"
#include "Text.h"
#include "CursorManager.h"

Tool_Text::Tool_Text() {
	currentTarget = 0;
}
void Tool_Text::onTextInput(const char *inputText) {

}
void Tool_Text::onKeyDown(int button) {


}
void Tool_Text::onMouseUp(const Coord &pos, int button) {

}
void Tool_Text::onEnd() {
}
float Tool_Text::drawTextStats(float h) {
	return h;
}
void Tool_Text::onMouseDown(const Coord &pos, int button) {
	if (currentTarget) {
		sim->beginEditingText(0);
		currentTarget = 0;
		return;
	}
	CShape *candid = sim->getShapeUnderCursor(true);
	if (candid != 0) {
		CText *txt = dynamic_cast<CText*>(candid);
		if (txt != 0) {
			currentTarget = txt;
			sim->beginEditingText(currentTarget);
		}
	}
	else {
		currentTarget = sim->getSim()->addText(pos, "TEXT");
		sim->beginEditingText(currentTarget);
	}
}
void Tool_Text::drawTool() {
	sim->getCursorMgr()->setCursor(SDL_SYSTEM_CURSOR_ARROW);
	

}



#endif



