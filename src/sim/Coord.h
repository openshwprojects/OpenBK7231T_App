#ifndef __COORD_H__
#define __COORD_H__

#include "sim_local.h"

class Coord {
	float x, y;
public:
	Coord() {
		this->x = 0;
		this->y = 0;
	}
	Coord(float _x, float _y) {
		this->x = _x;
		this->y = _y;
	}
	void set(float _x, float _y) {
		this->x = _x;
		this->y = _y;
	}
	void setX(float _x) {
		this->x = _x;
	}
	void setY(float _y) {
		this->y = _y;
	}
	operator float*() {
		return &x;
	}

	operator const float *() {
		return &x;
	}
	float getX() const {
		return x;
	}
	float getY() const {
		return y;
	}
};

#endif
