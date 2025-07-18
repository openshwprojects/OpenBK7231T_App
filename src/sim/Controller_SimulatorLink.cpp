#ifdef WINDOWS
#include "Controller_SimulatorLink.h"
#include "Shape.h"
#include "Junction.h"
#include "sim_import.h"


CControllerSimulatorLink::CControllerSimulatorLink() {
	bPowered = false;
}
bool CControllerSimulatorLink::isPowered() const {
	return bPowered;
}
CJunction *CControllerSimulatorLink::findJunctionByGPIOIndex(int idx) {
	for (int i = 0; i < related.size(); i++) {
		if (related[i]->getGPIO() == idx)
			return related[i];
	}
	return 0;
}
class CControllerBase *CControllerSimulatorLink::cloneController(class CShape *origOwner, class CShape *newOwner) {
	CControllerSimulatorLink *r = new CControllerSimulatorLink();
	for (int i = 0; i < related.size(); i++) {
		r->addRelatedJunction(newOwner->findShapeByName_r(related[i]->getName())->asJunction());
	}
	return r;
}
void CControllerSimulatorLink::onPostSolveVoltages() {
	CJunction *vdd = findJunctionByGPIOIndex(GPIO_VDD);
	CJunction *gnd = findJunctionByGPIOIndex(GPIO_GND);
	if (vdd->hasVoltage(3.3f) && gnd->hasVoltage(0.0f)) {
		bPowered = true;
	}
	else {
		bPowered = false;
	}
}
void CControllerSimulatorLink::onDrawn() {
	for (int i = 0; i < related.size(); i++) {
		CJunction *ju = related[i];
		int gpio = ju->getGPIO();
		float v = ju->getVoltage();
		if (gpio < 0)
			continue;
		if (SIM_IsPinADC(gpio)) {
			SIM_SetVoltageOnADCPin(gpio, v);
			ju->setCurrentSource(false);
			ju->setVisitCount(0);
		}
		else if (SIM_IsPinPWM(gpio)) {
			int pwm = SIM_GetPWMValue(gpio);
			ju->setCurrentSource(true);
			ju->setVoltage(3.3f);
			ju->setDuty(pwm);
		} else if (!SIM_IsPinInput(gpio)) {
			bool bVal = SIM_GetSimulatedPinValue(gpio);
			ju->setCurrentSource(true);
			if (bVal)
				ju->setVoltage(3.3f);
			else
				ju->setVoltage(0.0f);
			ju->setDuty(100.0f);
		}
		else {
			if (ju->getVisitCount() == 0)
				v = 3.3f;
			SIM_SetSimulatedPinValue(gpio, v > 1.5f);
			ju->setCurrentSource(false);
			ju->setDuty(100.0f);
		}
	}
}

#endif
