#ifndef __BOUNDS_H__
#define __BOUNDS_H__

#include "sim_local.h"
#include "Coord.h"

class Bounds {
	Coord mins, maxs;
public:
	Bounds() {
		clear();
	}
	void clear() {
		this->mins.set(99999, 999999);
		this->maxs.set(-99999, -99999);
	}
	bool isInside(const Coord &p) const {
		if (p.getX() < mins.getX())
			return false;
		if (p.getY() < mins.getY())
			return false;
		if (p.getX() > maxs.getX())
			return false;
		if (p.getY() > maxs.getY())
			return false;
		return true;
	}
	void addPoint(const Coord &p) {
		if (p.getX() < mins.getX())
			mins.setX(p.getX());
		if (p.getY() < mins.getY())
			mins.setY(p.getY());
		if (p.getX() > maxs.getX())
			maxs.setX(p.getX());
		if (p.getY() > maxs.getY())
			maxs.setY(p.getY());
	}
	void addPoint(float _x, float _y) {
		addPoint(Coord(_x, _y));
	}
	void translate(const Coord &ofs) {
		mins += ofs;
		maxs += ofs;
	}
	void addBounds(const Bounds &b, const Coord &ofs) {
		if (b.isValid() == false)
			return;
		Bounds bt = b;
		bt.translate(ofs);
		addBounds(bt);
	}
	void addBounds(const Bounds &b) {
		if (b.isValid() == false)
			return;
		if (mins.getX() > b.mins.getX())
			mins.setX(b.mins.getX());
		if (mins.getY() > b.mins.getY())
			mins.setY(b.mins.getY());
		if (maxs.getX() < b.maxs.getX())
			maxs.setX(b.maxs.getX());
		if (maxs.getY() < b.maxs.getY())
			maxs.setY(b.maxs.getY());
	}
	void extend(float f) {
		mins -= Coord(f, f);
		maxs += Coord(f, f);
	}
	bool isValid() const {
		if (mins.getX() > maxs.getX())
			return false;
		if (mins.getY() > maxs.getY())
			return false;
		return true;
	}
	Coord getCorner(int i) const {
		if (i == 0)
			return mins;
		if (i == 1)
			return Coord(mins.getX(), maxs.getY());
		if (i == 2)
			return maxs;
		return Coord(maxs.getX(), mins.getY());
	}
};

#endif

