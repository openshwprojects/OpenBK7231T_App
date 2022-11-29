#ifndef __SAVELOAD_H__
#define __SAVELOAD_H__

#include "sim_local.h"

class CProject {
	CString created;
	CString lastModified;
public:
	const char *getLastModified() const {
		return lastModified.c_str();
	}
	const char *getCreated() const {
		return created.c_str();
	}
	void setLastModified(const char *s) {
		lastModified = s;
	}
	void setCreated(const char *s) {
		created = s;
	}
};


class CSaveLoad {
	class CSimulator *sim;
public:
	void setSimulator(class CSimulator*ss) {
		this->sim = ss;
	}
	class CSimulation *loadSimulationFromFile(const char *fname);
	void saveSimulationToFile(class CSimulation *simToSave, const char *fname);
	class CProject *loadProjectFile(const char *fname);
	void saveProjectToFile(class CProject *projToSave, const char *fname);
};




#endif
