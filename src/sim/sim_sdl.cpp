#ifdef WINDOWS

#include "sim_local.h"
#include "Texture.h"
#include "Shape.h"
#include "Object.h"
#include "Coord.h"
#include "Simulator.h"
#include "Simulation.h"
#include "Wire.h"
#include "Junction.h"
#include "Text.h"
#include "Line.h"
#include "Rect.h"
#include "Circle.h"
#include "Tool_Base.h"
#include "Tool_Wire.h"
#include "Tool_Use.h"
#include "Tool_Move.h"
#include "Tool_Delete.h"
#include "CursorManager.h"
#include "Controller_Button.h"

#pragma comment (lib, "SDL2.lib")
#pragma comment (lib, "Opengl32.lib")
#pragma comment (lib, "freeglut.lib")

int WinWidth = 1680;
int WinHeight = 940;
#undef main

int drawText(int x, int y, const char* fmt, ...) {
	va_list argList;
	char buffer[512];
	va_start(argList, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, argList);
	va_end(argList);
	glRasterPos2i(x, y);
	for (int i = 0; i < strlen(buffer); i++) {
		glutBitmapCharacter(GLUT_BITMAP_9_BY_15, (int)buffer[i]);
	}
	return y + 15;
}
int gridSize = 20;
float roundToGrid(float f) {
	float g = f / gridSize;
	g += 0.5;
	g = floor(g);
	return g * gridSize;
}
Coord roundToGrid(Coord c) {
	return Coord(roundToGrid(c.getX()), roundToGrid(c.getY()));
}

Coord GetMousePos() {
	Coord r;
	int mx, my;
	//SDL_GetGlobalMouseState(&mx, &my);
	SDL_GetMouseState(&mx, &my);
	r.set(mx, my);
	return r;
}



float CEdge::drawInformation2D(float x, float h) {
	h = CShape::drawInformation2D(x, h);
	h = a->drawInformation2D(x + 20, h);
	h = b->drawInformation2D(x + 20, h);
	return h;
}
void CEdge::translate(const Coord &o) {
	a->translate(o);
	b->translate(o);
}
const Coord &CEdge::getPositionA() const {
	return a->getPosition();
}
const Coord &CEdge::getPositionB() const {
	return b->getPosition();
}
int main(int argc, char **argv)
{
	glutInit(&argc, argv);
	CSimulator *sim = new CSimulator();
	sim->createWindow();

	while (1)
	{
		sim->drawWindow();
	}
	return 0;
}
#endif
