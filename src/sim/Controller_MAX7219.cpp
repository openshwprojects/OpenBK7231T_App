#ifdef WINDOWS
#include "Simulator.h"
#include "Simulation.h"
#include "Junction.h"
#include "Solver.h"
#include "sim_local.h"
#include "Controller_MAX7219.h"
#include "Controller_SimulatorLink.h"
#include "Shape.h"
#include "Junction.h"
#include "Text.h"
#include "../cJSON/cJSON.h"

static CControllerMAX7219 *g_hack = 0;

// backlog startDriver MAX72XX; MAX72XX_Setup 10 8 9 16; MAX72XX_Clear; MAX72XX_Print 12345; MAX72XX_refresh; 
void CControllerMAX7219::saveTo(struct cJSON *j_obj) {
	
}
void CControllerMAX7219::loadFrom(struct cJSON *j_obj) {

}
void CControllerMAX7219::onDrawn() {
	g_hack = this;
	//int devices = 4;
	//int single = 8 * 8;
	for (int i = 0; i < 8 * 8 * 4; i++) {
		int idx = i / 8;   // byte index (row)
		int bit = i % 8;   // bit index

		// reverse bit to match MSB-left
		bool bOn = (pixels[idx] & (0x80 >> bit)) != 0;
		CShape *s = baseShape->getShape(i);
		s->setActive(bOn);
	}
}
class CControllerBase *CControllerMAX7219::cloneController(class CShape *origOwner, class CShape *newOwner) {
	CControllerMAX7219 *c = new CControllerMAX7219();
	return c;
}

extern "C"  void SIM_SetMAX7219Pixels(byte *data, int numDevices) {
	if (g_hack == 0) {
		return;
	}
	int totalRows = 8 * numDevices; // 4 cascaded devices, 8 rows each
	g_hack->numDevices = numDevices;
	for (int i = 0; i < totalRows; i++) {
		g_hack->pixels[i] = data[i]; // MSB-left
	}

}

#endif
