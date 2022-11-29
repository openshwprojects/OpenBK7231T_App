#ifdef WINDOWS
#include "Tool_Text.h"
#include "Simulator.h"
#include "Simulation.h"
#include "Shape.h"
#include "CursorManager.h"

Tool_Text::Tool_Text() {
	currentTarget = 0;
}
void Tool_Text::onMouseUp(const Coord &pos, int button) {

}
void Tool_Text::onEnd() {
}
int Tool_Text::drawTextStats(int h) {
	return h;
}
void Tool_Text::onMouseDown(const Coord &pos, int button) {
	currentTarget = sim->getSim()->addText(pos, "TEXT");

}
void Tool_Text::drawTool() {
	

}



#endif



