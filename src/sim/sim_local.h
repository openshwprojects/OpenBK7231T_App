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

typedef unsigned char byte;
typedef int32_t i32;
typedef uint32_t u32;

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

#define DEG2RAD(angleInDegrees) ((angleInDegrees) * M_PI / 180.0)
#define RAD2DEG(angleInRadians) ((angleInRadians) * 180.0 / M_PI)

enum {
	EVE_NONE,
	EVE_LMB_HOLD,
	EVE_LMB_DOWN,
};

#define WINDOWS_MOUSE_MENUBAR_OFFSET 20

float drawText(class CStyle *style, float x, float y, const char* fmt, ...);
#include "Coord.h"
Coord roundToGrid(Coord c);
Coord GetMousePosWorld();
void FS_CreateDirectoriesForPath(const char *file_path);
char *FS_ReadTextFile(const char *fname);
bool FS_WriteTextFile(const char *data, const char *fname);
bool FS_Exists(const char *fname);

extern int WinWidth;
extern int WinHeight;
extern int gridSize;
extern class CSimulator *g_sim;

extern "C" {
	void CMD_ExpandConstantsWithinString(const char *in, char *out, int outLen);
	int UART_GetDataSize();
	void UART_AppendByteToReceiveRingBuffer(int rc);
	int CMD_ExecuteCommand(const char* s, int cmdFlags);
}

template <typename T>
class TArray : public std::vector<T> {
public:
	T pop() {
		if (!this->empty()) {
			T lastElement = this->back();
			this->pop_back();
			return lastElement;
		}
		return 0;
	}
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

class CColor {
	float r, g, b, a;
public:
	CColor() {
		r = g = b = a = 1;
	}
	CColor(float _r, float _g, float _b, float _a = 1) {
		r = _r;
		g = _g;
		b = _b;
		a = _a;
	}
	CColor(int _r, int _g, int _b, int _a = 255) {
		r = _r/255.0f;
		g = _g / 255.0f;
		b = _b / 255.0f;
		a = _a / 255.0f;
	}
	CColor operator-()const {
		return CColor(-r, -g, -b, -a);
	}
	CColor operator+(const CColor& rhs) const {
		return CColor(this->r + rhs.r, this->g + rhs.g, this->b + rhs.b, this->a + rhs.a);
	}
	CColor operator-(const CColor& rhs) const {
		return CColor(this->r - rhs.r, this->g - rhs.g, this->b - rhs.b, this->a - rhs.a);
	}
	CColor operator*(float f) const {
		return CColor(this->r * f, this->g * f, this->b * f, this->a * f);
	}
	friend CColor operator*(float f, const CColor &o) {
		return CColor(o.r * f, o.g* f, o.b* f, o.a* f);
	}
	operator float*() {
		return &r;
	}
	void fromRGB(const byte *rgb) {
		r = (rgb[0]) / 255.0f;
		g = (rgb[1]) / 255.0f;
		b = (rgb[2]) / 255.0f;
	}
	operator const float *() const  {
		return &r;
	}
};
class CStyle {
	CColor color;
	float lineWidth;
public:
	CStyle(const CColor &col, float lw) {
		color = col;
		lineWidth = lw;
	}
	void apply() {
		glColor3fv(color);
		glLineWidth(lineWidth);
	}
};
extern CStyle g_style_shapes;
extern CStyle g_style_wires;
extern CStyle g_style_text;
extern CStyle g_style_text_red;

enum SpecialGPIO {
	GPIO_INVALID = -1,
	GPIO_VDD = -2,
	GPIO_GND = -3,
	GPIO_EN = -4,
	GPIO_CEN = -5
};

#endif
#endif
