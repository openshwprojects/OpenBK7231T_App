#ifdef WINDOWS

#ifndef __SIM_LOCAL_H__
#define __SIM_LOCAL_H__

#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <vector>
#include <algorithm>
#include <string>
#include <SDL.h>
#include <SDL_opengl.h>
#include <GL/gl.h>
#include <GL/glut.h>

typedef int32_t i32;
typedef uint32_t u32;

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

#define DEG2RAD(angleInDegrees) ((angleInDegrees) * M_PI / 180.0)
#define RAD2DEG(angleInRadians) ((angleInRadians) * 180.0 / M_PI)

enum {
	EVE_NONE,
	EVE_LMB_HOLD,
};

int drawText(int x, int y, const char* fmt, ...);
#include "Coord.h"
Coord roundToGrid(Coord c);
Coord GetMousePos();

extern int WinWidth;
extern int WinHeight;
extern int gridSize;

template <typename T>
class TArray : public std::vector<T> {
public:
	void remove(T o) {
		this->erase(std::remove(this->begin(), this->end(), o), this->end());
	}
	bool contains(const T&o) const {
		for (unsigned int i = 0; i < size(); i++) {
			if ((*this)[i] == o)
				return true;
		}
		return false;
	}
	void add_unique(const T&o) {
		if (contains(o) == false) {
			this->push_back(o);
		}
	}
};

#endif
#endif
