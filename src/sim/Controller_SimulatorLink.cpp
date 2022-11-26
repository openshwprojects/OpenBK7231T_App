#ifdef WINDOWS
#include "Controller_SimulatorLink.h"
#include "Shape.h"
#include "Junction.h"

extern "C" {
	void SIM_SetSimulatedPinValue(int pinIndex, bool bHigh);
	bool SIM_GetSimulatedPinValue(int pinIndex);
	bool SIM_IsPinInput(int index);
};


CControllerSimulatorLink::CControllerSimulatorLink() {

}
void CControllerSimulatorLink::onDrawn() {
	for (int i = 0; i < related.size(); i++) {
		CJunction *ju = related[i];
		int gpio = ju->getGPIO();
		float v = ju->getVoltage();
		if (gpio < 0)
			continue;
		if (!SIM_IsPinInput(gpio)) {
			bool bVal = SIM_GetSimulatedPinValue(gpio);
			ju->setCurrentSource(true);
			if (bVal)
				ju->setVoltage(3.3f);
			else
				ju->setVoltage(0.0f);
		}
		else {
			if (ju->getVisitCount() == 0)
				v = 3.3f;
			SIM_SetSimulatedPinValue(gpio, v > 1.5f);
			ju->setCurrentSource(false);
		}
	}
}

#endif
