#ifdef WINDOWS

#include "sim_local.h"
#include "Texture.h"
#include "Shape.h"
#include "Object.h"
#include "Coord.h"
#include "Simulator.h"
#include "Simulation.h"

#pragma comment (lib, "SDL2.lib")
#pragma comment (lib, "Opengl32.lib")
#pragma comment (lib, "freeglut.lib")

int WinWidth = 1680;
int WinHeight = 940;
#undef main

void drawText(int x, int y, const char* text) {
	glRasterPos2i(x, y);
	for (int i = 0; i < strlen(text); i++) {
		glutBitmapCharacter(GLUT_BITMAP_9_BY_15, (int)text[i]);
	}
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

class CWire  {
	std::vector<class CJunction*> junctions;

public:
	CWire(const Coord &a, const Coord &b);
	void drawWire();
	void addPoint(const Coord &p);
};
class CJunction : public CShape {
	std::string name;
public:
	CJunction(int _x, int _y, const char *s) {
		this->x = _x;
		this->y = _y;
		this->name = s;
	}
	virtual void drawShape();
};


void CJunction::drawShape() {
	glPointSize(8.0f);
	glColor3f(0, 1, 0);
	glBegin(GL_POINTS);
	glVertex2f(x, y);
	glEnd();
	glColor3f(1, 1, 1);
}
void CSimulation::drawSim() {
	for (int i = 0; i < objects.size(); i++) {
		objects[i]->drawWithChildren();
	}
	for (int i = 0; i < wires.size(); i++) {
		wires[i]->drawWire();
	}
}
void CWire::drawWire() {
	glBegin(GL_LINE_STRIP);
	for (int i = 0; i < junctions.size(); i++) {
		CJunction *j = junctions[i];
		glVertex2f(j->getX(), j->getY());
	}
	glEnd();
}
class CWire *CSimulation::addWire(const class Coord &a, const class Coord &b) {
	class CWire *cw = new CWire(a, b);
	wires.push_back(cw);
	return cw;
}
CObject * CSimulation::addObject(CObject *o) {
	objects.push_back(o);
	return o;
}
void CSimulation::createDemo() {
	addObject(generateWB3S())->setPosition(300, 200);
	addObject(generateButton())->setPosition(500, 200);
}
class CBaseObject {

};
class CRectangle : public CShape {
	int w, h;
};
class CLine : public CShape {
	int x2, y2;
public:
	CLine(int _x, int _y, int _x2, int _y2) {
		this->x = _x;
		this->y = _y;
		this->x2 = _x2;
		this->y2 = _y2;
	}
	virtual void drawShape() {
		glBegin(GL_LINES);
		glVertex2f(x, y);
		glVertex2f(x2, y2);
		glEnd();
	}
}; 
class CText : public CShape {
	std::string txt;
public:
	CText(int _x, int _y, const char *s) {
		this->x = _x;
		this->y = _y;
		this->txt = s;
	}
	virtual void drawShape() {
		drawText(x, y, txt.c_str());
	}
};
class CRect : public CShape {
	int w, h;
public:
	CRect(int _x, int _y, int _w, int _h) {
		this->x = _x;
		this->y = _y;
		this->w = _w;
		this->h = _h;
	}
	virtual void drawShape() {
		glBegin(GL_LINE_LOOP);
		glVertex2f(x, y);
		glVertex2f(x+w, y);
		glVertex2f(x+w, y+h);
		glVertex2f(x, y+h);
		glEnd();
	}
};
class CObject *CSimulation::generateButton() {
	CObject *o = new CObject();

	o->addJunction(-40, -10);
	o->addJunction(40, -10);
	o->addLine(40, -10, 20, -10);
	o->addLine(-40, -10, -20, -10);
	o->addLine(20, 10, -20, 10);
	o->addLine(0, 20, 0, 10);
	return o;
}
class CObject *CSimulation::generateWB3S() {

	CObject *o = new CObject();
	o->addText(-40, -25, "WB3S");
	o->addRect(-50, -20, 100, 180);
	const char *wb3sPinsRight[] = {
		"TXD1",
		"RXD1",
		"PWM2",
		"PWM3",
		"RXD2",
		"TXD2",
		"PWM1",
		"GND"
	};
	const char *wb3sPinsLeft[] = {
		"CEN",
		"ADC3",
		"EN",
		"P14",
		"PWM5",
		"PWM4",
		"PWM3",
		"VCC"
	};
	for (int i = 0; i < 8; i++) {
		int y = i * 20;
		o->addLine(50, y, 80, y);
		o->addLine(-50, y, -80, y);
		o->addJunction(-80, y)->addText(-5, -5, wb3sPinsLeft[i]);
		o->addJunction(80, y)->addText(-25, -5, wb3sPinsRight[i]);
	}
	return o;
}
class Tool_Base {
protected:
	CSimulator *sim;
public:
	void setSimulator(CSimulator *s) {
		this->sim = s;
	}
	virtual void onKeyDown(int button) {

	}
	virtual void onMouseDown(Coord pos, int button) {

	}
	virtual void drawTool() {

	}
};
class Tool_Move : public Tool_Base {

};
Coord GetMousePos() {
	Coord r;
	int mx, my;
	//SDL_GetGlobalMouseState(&mx, &my);
	SDL_GetMouseState(&mx, &my);
	r.set(mx, my);
	return r;
}
class Tool_Wire : public Tool_Base {
	Coord basePos;
	bool bActive;
	bool bSideness;
	class CWire *newWire;
	Coord a, b, c;
public:
	Tool_Wire() {
		newWire = 0;
		bActive = false;
	}
	virtual void onKeyDown(int button) {
		if (button == SDLK_ESCAPE) {
			bActive = false;
		}
	}
	virtual void onMouseDown(Coord pos, int button) {
		if (button == 3) {
			bSideness = !bSideness;
		}
		if (button == 1) {
			Coord curPos = roundToGrid(GetMousePos());
			if (bActive) {
				if (newWire == 0) {
					newWire = sim->getSim()->addWire(a, b);
					newWire->addPoint(c);
				} else {
					newWire->addPoint(b);
					newWire->addPoint(c);
				}
				basePos = curPos;
			}
			else {
				basePos = curPos;
				bActive = true;
			}
		}
	}
	virtual void drawTool() {
		Coord m;
		m = roundToGrid(GetMousePos());
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
};


class CShape* CShape::addLine(int x, int y, int x2, int y2) {
	CLine *n = new CLine(x, y, x2, y2);
	shapes.push_back(n);
	return n;
}
class CShape* CShape::addJunction(int x, int y, const char *name) {
	CJunction *n = new CJunction(x, y, name);
	shapes.push_back(n);
	return n;
}
class CShape* CShape::addRect(int x, int y, int w, int h) {
	CRect *n = new CRect(x, y, w, h);
	shapes.push_back(n);
	return n;
}
class CShape* CShape::addText(int x, int y, const char *s) {
	CText *n = new CText(x, y, s);
	shapes.push_back(n);
	return n;
}
void CShape::drawWithChildren() {
	drawShape();
	glPushMatrix();
	glTranslatef(x, y, 0);
	for (int i = 0; i < shapes.size(); i++) {
		shapes[i]->drawWithChildren();
	}
	glPopMatrix();
}
CSimulator::CSimulator() {
	Window = 0;
	Context = 0;
	WindowFlags = SDL_WINDOW_OPENGL;
	Running = 1;
	FullScreen = 0;
	activeTool = new Tool_Wire();
	sim = new CSimulation();
	activeTool->setSimulator(this);
	sim->createDemo();
}

CWire::CWire(const Coord &a, const Coord &b) {
	addPoint(a);
	addPoint(b);
}

void CWire::addPoint(const Coord &p) {
	CJunction *j = new CJunction(p.getX(),p.getY(),"");
	junctions.push_back(j);
}



void CSimulator::createWindow() {
	Window = SDL_CreateWindow("OpenGL Test", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WinWidth, WinHeight, WindowFlags);
	assert(Window);
	Context = SDL_GL_CreateContext(Window);
}

void CSimulator::drawWindow() {
	SDL_Event Event;
	while (SDL_PollEvent(&Event))
	{
		if (Event.type == SDL_KEYDOWN)
		{
			if (activeTool) {
				activeTool->onKeyDown(Event.key.keysym.sym);
			}
			switch (Event.key.keysym.sym)
			{
			case SDLK_ESCAPE:
				//Running = 0;
				break;
			case 'f':
				FullScreen = !FullScreen;
				if (FullScreen)
				{
					SDL_SetWindowFullscreen(Window, WindowFlags | SDL_WINDOW_FULLSCREEN_DESKTOP);
				}
				else
				{
					SDL_SetWindowFullscreen(Window, WindowFlags);
				}
				break;
			default:
				break;
			}
		}
		else if (Event.type == SDL_MOUSEBUTTONDOWN)
		{
			int x = Event.button.x;
			int y = Event.button.y;
			int which = Event.button.button;
			if (activeTool) {
				activeTool->onMouseDown(Coord(x, y), which);
			}
		}
		else if (Event.type == SDL_QUIT)
		{
			Running = 0;
		}
	}

	glViewport(0, 0, WinWidth, WinHeight);
	glClearColor(1.f, 0.f, 1.f, 0.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0f, WinWidth, WinHeight, 0.0f, 0.0f, 1.0f);

	glColor3f(0.7f, 0.7f, 0.7f);
	glLineWidth(0.25f);
	glBegin(GL_LINES);
	for (int i = 0; i < WinWidth; i += gridSize) {
		glVertex2f(i, 0);
		glVertex2f(i, WinHeight);
	}
	for (int i = 0; i < WinHeight; i += gridSize) {
		glVertex2f(0, i);
		glVertex2f(WinWidth, i);
	}
	glEnd();
	glColor3f(1, 1, 0);
	glLineWidth(2);
	//glBegin(GL_LINE_STRIP);
	//glVertex3f(100, 100, 0);
	//glVertex3f(400, 400, 0);
	//glVertex3f(100, 400, 0);
	//glEnd();

	sim->drawSim();

	if (activeTool) {
		activeTool->drawTool();
	}
	//static Texture2D *t = 0;
	//if (t == 0) {
	//	t = new Texture2D();
	//	t->loadTexture("WB3S.png");
	//}
	//glColor3f(1, 1, 1);
	//glEnable(GL_TEXTURE_2D);
	//glBindTexture(GL_TEXTURE_2D, t->getHandle());
	//glBegin(GL_QUADS);
	//glTexCoord2f(0, 1);
	//glVertex2f(10, 10);
	//glTexCoord2f(0, 0);
	//glVertex2f(10, 150);
	//glTexCoord2f(1, 0);
	//glVertex2f(150, 150);
	//glTexCoord2f(1, 1);
	//glVertex2f(150, 10);
	//glEnd();
	//glDisable(GL_TEXTURE_2D);
	SDL_GL_SwapWindow(Window);
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
