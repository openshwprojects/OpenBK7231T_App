#ifndef __CONTROLLER_DHT11_H__
#define __CONTROLLER_DHT11_H__

#include "sim_local.h"
#include "Controller_Base.h"
#include "Coord.h"

class CControllerDHT11 : public CControllerBase {
	class CJunction *dat;
	class CText *txt_temperature, *txt_humidity;

	void setShapesActive(bool b);
	void setShapesFillColor(const CColor &c);
	float realTemperature;
	float realHumidity;
public:
	CControllerDHT11() {
		dat = 0;
		txt_temperature = txt_humidity = 0;
		realTemperature = 19.8f;
		realHumidity = 68.0f;
	}
	CControllerDHT11(class CJunction *_dat, CText *_txt_temperature, CText *_txt_humidity) {
		dat = _dat;
		txt_temperature = _txt_temperature;
		txt_humidity = _txt_humidity;
		realTemperature = 19.8f;
		realHumidity = 68.0f;
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
	class CJunction *getDataPin() {
		return dat;
	}
};

#endif // __CONTROLLER_DHT11_H__
