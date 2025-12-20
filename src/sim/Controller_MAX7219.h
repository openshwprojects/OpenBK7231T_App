#ifndef __CONTROLLER_MAX7219_H__
#define __CONTROLLER_MAX7219_H__

#include "sim_local.h"
#include "Controller_Base.h"
#include "Coord.h"

class CControllerMAX7219 : public CControllerBase {
	void setShapesActive(bool b);
	void setShapesFillColor(const CColor &c);
	float realTemperature;
	float realHumidity;
public:
	byte pixels[512]; // TODO
	int numDevices;

	CControllerMAX7219() {
	}
	virtual void onDrawn();
	virtual class CControllerBase *cloneController(class CShape *origOwner, class CShape *newOwner);

	virtual void saveTo(struct cJSON *j_obj);
	virtual void loadFrom(struct cJSON *j_obj);

	float getTemperature() const {
		return realTemperature;
	}
	float getHumidity() const {
		return realHumidity;
	}
};

#endif // __CONTROLLER_MAX7219_H__
