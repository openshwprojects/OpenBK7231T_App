#ifndef __PREFABMANAGER_H__
#define __PREFABMANAGER_H__

#include "sim_local.h"

class PrefabManager {
	class CSimulator *sim;
	TArray<class CShape*> prefabs;

	class CShape *generateLED_CW();
	class CShape *generateLED_RGB();
	class CShape *generateStrip_SingleColor();
	class CShape *generateStrip_CW();
	class CShape *generateStrip_RGB();
	class CShape *generateStrip_RGBCW();
	class CShape *generateBulb();
	class CShape *generateWB3S();
	class CShape *generateButton();
	class CShape *generateSwitch();
	class CShape *generateTest();
	class CShape *generateBL0942();
	class CShape *generateGND();
	class CShape *generateVDD();
	class CShape *generatePot();
	class CShape *generateWS2812B();
	class CShape *generateDHT11();
public:
	PrefabManager(CSimulator *ps) {
		sim = ps;
	}
	void createDefaultPrefabs();
	void addPrefab(CShape *o);
	CShape *findPrefab(const char *name);
	CShape *instantiatePrefab(const char *name);
	unsigned int size() const {
		return prefabs.size();
	}
	CShape *get(int i) {
		return prefabs[i];
	}

};
#endif
