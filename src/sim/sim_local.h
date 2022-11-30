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

#define WINDOWS_MOUSE_MENUBAR_OFFSET 20

int drawText(int x, int y, const char* fmt, ...);
#include "Coord.h"
Coord roundToGrid(Coord c);
Coord GetMousePos();
void FS_CreateDirectoriesForPath(const char *file_path);
char *FS_ReadTextFile(const char *fname);
bool FS_WriteTextFile(const char *data, const char *fname);
bool FS_Exists(const char *fname);

extern int WinWidth;
extern int WinHeight;
extern int gridSize;

template <typename T>
class TArray : public std::vector<T> {
public:
	void remove(T o) {
		this->erase(std::remove(this->begin(), this->end(), o), this->end());
	}
	void removeAt(int i) {
		this->erase(this->begin() + i);
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

class CString : public std::string {

public:
	CString() {

	}
	CString(const char *s) : std::string(s) {
	}
	void stripExtension() {
		int lst = this->find_last_of(".");
		int pathSep = this->find_last_of("/\\");
		if (lst > 0 && lst > pathSep) {
			this->resize(lst);
		}
	}
	bool hasExtension() const {
		int lst = this->find_last_of(".");
		int pathSep = this->find_last_of("/\\");
		if (lst > 0 && lst > pathSep) {
			return true;
		}
		return false;
	}
	static CString getWithoutExtension(const char *s) {
		CString r = s;
		r.stripExtension();
		return r;
	}
	static CString constructPathByStrippingExt(const char *dir, const char *subfile) {
		CString r = dir;
		r.stripExtension();
		r.append("/");
		r.append(subfile);
		return r;
	}
};
#endif
#endif
