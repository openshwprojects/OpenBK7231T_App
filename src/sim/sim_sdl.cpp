#ifdef WINDOWS

#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <SDL.h>
#include <SDL_opengl.h>
#include <GL/gl.h>

#pragma comment (lib, "SDL2.lib")
#pragma comment (lib, "Opengl32.lib")

typedef int32_t i32;
typedef uint32_t u32;
typedef int32_t b32;

#define WinWidth 640
#define WinHeight 480
#undef main

SDL_Window *Window = 0;
SDL_GLContext Context = 0;
u32 WindowFlags = SDL_WINDOW_OPENGL;
b32 Running = 1;
b32 FullScreen = 0;

#if 0
#include <vector>
#include <string>
class CSimulation {
	std::vector<CObject*> objects;
	std::vector<CWire*> wires;

public:
};
class CBaseObject {

};
class CObject : CBaseObject {
	std::vector<CJunction*> interfaces;
};
class CJunction {
	CBaseObject *owner;
	int x, y;
	std::string label;

};
class CWire : CBaseObject {
	std::vector<CJunction*> junctions;
};
#endif
void SIM_CreateWindow() {
	Window = SDL_CreateWindow("OpenGL Test", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WinWidth, WinHeight, WindowFlags);
	assert(Window);
	Context = SDL_GL_CreateContext(Window);
}
void SIM_DrawWindow() {
	SDL_Event Event;
	while (SDL_PollEvent(&Event))
	{
		if (Event.type == SDL_KEYDOWN)
		{
			switch (Event.key.keysym.sym)
			{
			case SDLK_ESCAPE:
				Running = 0;
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

	glColor3f(1, 1, 0);
	glLineWidth(10);
	glBegin(GL_LINE_STRIP);
	glVertex3f(100, 100, 0);
	glVertex3f(400, 400, 0);
	glVertex3f(100, 400, 0);
	glEnd();

	glBegin(GL_QUADS);
	glVertex2f(10, 10);
	glVertex2f(10, 50);
	glVertex2f(50, 50);
	glVertex2f(50, 10);
	glEnd();
	SDL_GL_SwapWindow(Window);
}
int main(int ArgCount, char **Args)
{
	SIM_CreateWindow();

	while (Running)
	{
		SIM_DrawWindow();
	}
	return 0;
}
#endif
