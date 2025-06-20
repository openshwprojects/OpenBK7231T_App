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
	void addX(float _x) {
		this->x += _x;
	}
	void addY(float _y) {
		this->y += _y;
	}
	bool isNonZero() const {
		if (abs(x) > 0.001f)
			return true;
		if (abs(y) > 0.001f)
			return true;
		return false;
	}
	Coord lerp(const Coord &b, float f) const {
		Coord delta = b - *this;
		return *this + delta * f;
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
	Coord operator-()const {
		return Coord(-x, -y);
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
	Coord operator/(float f) const {
		return Coord(this->x / f, this->y / f);
	}
	friend Coord operator*(float f, const Coord &o)  {
		return Coord(o.x * f, o.y * f);
	}
	static float dot(const Coord &a, const Coord &b) {
		return a.x*b.x + a.y*b.y;
	}
	bool isWithinDist(const Coord &tg, float d) const {
		Coord delta = tg - *this;
		return delta.len() < d;
	}
	Coord rotateDegreesAround(float rot, const Coord &center) const {
		Coord p = *this;
		float angle = DEG2RAD(rot);
		float s = sin(angle);
		float c = cos(angle);

		// translate point back to origin:
		p.x -= center.x;
		p.y -= center.y;

		// rotate point
		float xnew = p.x * c - p.y * s;
		float ynew = p.x * s + p.y * c;

		// translate point back:
		p.x = xnew + center.x;
		p.y = ynew + center.y;
		return p;
	}
	float distToSegment(const Coord &v, const Coord &w) const {
		// Return minimum distance between line segment vw and point p
		const float l2 = v.distSq(w);  // i.e. |w-v|^2 -  avoid a sqrt
		if (l2 == 0.0)
			return this->dist(v);   // v == w case
		// Consider the line extending the segment, parameterized as v + t (w - v).
		// We find projection of point p onto the line. 
		// It falls where t = [(p-v) . (w-v)] / |w-v|^2
		// We clamp t from [0,1] to handle points outside the segment vw.
		float t = MAX(0, MIN(1, dot(*this - v, w - v) / l2));
		const Coord projection = v + t * (w - v);  // Projection falls on the segment
		return this->dist(projection);
	}
	bool isWithinDist(const Coord &v, const Coord &w, float d) const {
		float realD = distToSegment(v, w);
		if (realD < d)
			return true;
		return false;
	}
	Coord moveTowards(const Coord &tg, float dt) const {
		Coord dir = tg - *this;
		float len = dir.normalize();
		if (len < dt)
			return tg;
		return *this + dir * dt;
	}
	float moveMeTowards(const Coord &tg, float dt) {
		*this = moveTowards(tg, dt);
		return this->dist(tg);
	}
	float normalize() {
		float l = len();
		if (abs(l) < 0.0001f)
			return 0;
		x /= l;
		y /= l;
		return l;
	}
	float dist(const Coord &o) const {
		Coord d = o - *this;
		return d.len();
	}
	float distSq(const Coord &o) const {
		Coord d = o - *this;
		return d.lenSq();
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
