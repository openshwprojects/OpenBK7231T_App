#ifdef WINDOWS
#include "Simulator.h"
#include "Simulation.h"
#include "Junction.h"
#include "Solver.h"
#include "sim_local.h"
#include "Controller_DHT11.h"
#include "Controller_SimulatorLink.h"
#include "Shape.h"
#include "Junction.h"
#include "Text.h"
#include "../cJSON/cJSON.h"

extern "C" bool SIM_ReadDHT11(int pin, byte *data) {
	data[0] = data[1] = data[2] = data[3] = data[4] = 0;
	// temp 19
	data[2] = 19;
	// humidity 67
	data[0] = 67;
	data[1] = 0;
	if (g_sim == 0)
		return true;
	CSimulation *s = g_sim->getSim();
	if (s == 0)
		return true;
	TArray<CControllerDHT11 *> dhts = s->findControllersOfType<CControllerDHT11>();
	for (int d = 0; d < dhts.size(); d++) {
		CControllerDHT11 *dht = dhts[d];
		CControllerSimulatorLink *wifi = s->findFirstControllerOfType<CControllerSimulatorLink>();
		if (wifi == 0)
			return true;
		CJunction *p = wifi->findJunctionByGPIOIndex(pin);
		if (p == 0)
			return true;
		data[2] = (byte)dht->getTemperature();
		data[0] = (byte)dht->getHumidity();
		if (g_sim->getSolver()->hasPath(dht->getDataPin(), p)) {
			return true;
		}
	}
	return true;
}
void CControllerDHT11::saveTo(struct cJSON *j_obj) {
	cJSON_AddStringToObject(j_obj, "temperature", this->txt_temperature->getText());
	cJSON_AddStringToObject(j_obj, "humidity", this->txt_humidity->getText());

}
void CControllerDHT11::loadFrom(struct cJSON *j_obj) {
	cJSON *temp = cJSON_GetObjectItemCaseSensitive(j_obj, "temperature");
	if (temp) {
		this->txt_temperature->setText(temp->valuestring);
	}
	cJSON *hum = cJSON_GetObjectItemCaseSensitive(j_obj, "humidity");
	if (hum) {
		this->txt_humidity->setText(hum->valuestring);
	}
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
