#ifndef __PREFABMANAGER_H__
#define __PREFABMANAGER_H__

#include "sim_local.h"
#include "PrefabManager.h"

class PrefabManager {
	class CSimulator *sim;
	TArray<class CObject*> prefabs;

	class CObject *generateLED_CW();
	class CObject *generateLED_RGB();
	class CObject *generateBulb();
	class CObject *generateWB3S();
	class CObject *generateButton();
	class CObject *generateTest();
	class CObject *generateGND();
	class CObject *generateVDD();
public:
	PrefabManager(CSimulator *ps) {
		sim = ps;
	}
	void createDefaultPrefabs();
	void addPrefab(CObject *o);
	CObject *findPrefab(const char *name);
	CObject *instantiatePrefab(const char *name);

};
#endif
