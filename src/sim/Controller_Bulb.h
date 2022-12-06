#ifndef __CONTROLLER_BULB_H__
#define __CONTROLLER_BULB_H__

#include "sim_local.h"
#include "Controller_Base.h"
#include "Coord.h"

enum BulbMode {
	BM_NONE,
	BM_BULB,
	BM_CW,
	BM_RGB,
	BM_RGBCW
};
class CControllerBulb : public CControllerBase {
	TArray<class CShape *>shapes;
	class CJunction *a, *b;
	class CJunction *red, *green, *blue, *warm, *cool;
	class CJunction *gnd;
	BulbMode mode;

	void setShapesActive(bool b);
	void setShapesFillColor(const CColor &c);
public:
	CControllerBulb() {
		mode = BM_NONE;
		a = b = 0;
		red = green = blue = warm = cool = gnd = 0;
	}
	CControllerBulb(class CJunction *_a, class CJunction *_b) {
		mode = BM_BULB;
		a = _a;
		b = _b;
		red = green = blue = warm = cool = gnd = 0;
	}
	CControllerBulb(class CJunction *_c, class CJunction *_w, class CJunction *_gnd) {
		mode = BM_CW;
		red = green = blue = warm = cool = gnd = 0;
		a = b = 0;
		gnd = _gnd;
		cool = _c;
		warm = _w;
	}
	CControllerBulb(class CJunction *_r, class CJunction *_g, class CJunction *_b, class CJunction *_gnd) {
		mode = BM_RGB;
		warm = cool = 0;
		a = b = 0;
		gnd = _gnd;
		red = _r;
		green = _g;
		blue = _b;
	}
	CControllerBulb(class CJunction *_r, class CJunction *_g, class CJunction *_b, 
		class CJunction *_c, class CJunction *_w, class CJunction *_gnd) {
		mode = BM_RGBCW;
		a = b = 0;
		gnd = _gnd;
		red = _r;
		green = _g;
		blue = _b;
		cool = _c;
		warm = _w;
	}
	void setShape(CShape *p) {
		shapes.clear();
		shapes.push_back(p);
	}
	void addShape(CShape *p) {
		shapes.push_back(p);
	}
	virtual void onDrawn();
	virtual class CControllerBase *cloneController(class CShape *origOwner, class CShape *newOwner);
};

#endif // __CONTROLLER_BULB_H__
