#ifdef WINDOWS
#include "Simulator.h"
#include "Simulation.h"
#include "sim_local.h"
#include "Controller_DHT11.h"
#include "Shape.h"
#include "Junction.h"
#include "Text.h"

extern "C" bool SIM_ReadDHT11(int pin, byte *data) {
	CSimulation *s = g_sim->getSim();
	data[0] = data[1] = data[2] = data[3] = data[4] = 0;
	// temp 19
	data[2] = 19;
	// humidity 67
	data[0] = 67;
	data[1] = 0;
	if (s == 0)
		return true;
	CControllerDHT11 *dht = s->findFirstControllerOfType<CControllerDHT11>();
	if (dht == 0)
		return true;
	data[2] = dht->getTemperature();
	data[0] = dht->getHumidity();

	return true;
}

void CControllerDHT11::onDrawn() {
	if (txt_temperature->isBeingEdited() == false) {
		realTemperature = txt_temperature->getFloat();
	}
	if (txt_humidity->isBeingEdited() == false) {
		realHumidity = txt_humidity->getFloat();
	}
}
class CControllerBase *CControllerDHT11::cloneController(class CShape *origOwner, class CShape *newOwner) {
	CControllerDHT11 *r = new CControllerDHT11();
	if (dat) {
		r->dat = newOwner->findShapeByName_r(dat->getName())->asJunction();
	}
	if (txt_temperature) {
		r->txt_temperature = newOwner->findShapeByName_r(txt_temperature->getName())->asText();
	}
	if (txt_humidity) {
		r->txt_humidity = newOwner->findShapeByName_r(txt_humidity->getName())->asText();
	}
	return r;
}

#endif
