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
	void add(float _x, float _y) {
		this->x += _x;
		this->y += _y;
	}
	void setX(float _x) {
		this->x = _x;
	}
	void setY(float _y) {
		this->y = _y;
	}
	bool isNonZero() const {
		if (abs(x) > 0.001f)
			return true;
		if (abs(y) > 0.001f)
			return true;
		return false;
	}
	Coord& operator+=(const Coord& rhs) {
		this->x += rhs.x;
		this->y += rhs.y;
		return *this;
	}
	Coord& operator-=(const Coord& rhs) {
		this->x -= rhs.x;
		this->y -= rhs.y;
		return *this;
	}
	Coord operator+(const Coord& rhs) const {
		return Coord(this->x + rhs.getX(), this->y + rhs.getY());
	}
	Coord operator-(const Coord& rhs) const {
		return Coord(this->x - rhs.getX(), this->y - rhs.getY());
	}
	Coord operator*(float f) const {
		return Coord(this->x * f, this->y * f);
	}
	Coord moveTowards(const Coord &tg, float dt) {
		Coord dir = tg - *this;
		float len = dir.normalize();
		if (len < dt)
			return tg;
		return *this + dir * dt;
	}
	float normalize() {
		float l = len();
		if (abs(l) < 0.0001f)
			return 0;
		x /= l;
		y /= l;
		return l;
	}
	float lenSq() const {
		return x * x + y * y;
	}
	float len() const {
		return sqrt(lenSq());
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
