#ifdef WINDOWS
#include "SaveLoad.h"
#include "Simulator.h"
#include "Shape.h"
#include "Junction.h"
#include "Wire.h"
#include "Simulation.h"
#include "PrefabManager.h"
#include "Text.h"
#include "Controller_Base.h"
#include "../cJSON/cJSON.h"

class CProject *CSaveLoad::loadProjectFile(const char *fname) {
	CProject *p;
	p = new CProject();
	char *jsonData;
	jsonData = FS_ReadTextFile(fname);
	if (jsonData == 0) {
		return 0;
	}
	cJSON *n_jProj = cJSON_Parse(jsonData);
	if (0==n_jProj) {
		printf("Warning: failed to parse JSON data %s\n", fname);
	}
	else {
		cJSON *n_jProjCreated = cJSON_GetObjectItemCaseSensitive(n_jProj, "created");
		cJSON *n_jProjModified = cJSON_GetObjectItemCaseSensitive(n_jProj, "lastModified");
		if (n_jProjCreated) {
			p->setCreated(n_jProjCreated->valuestring);
		}
		else {
			printf("Warning: missing 'created' node in %s\n", fname);
		}
		if (n_jProjModified) {
			p->setLastModified(n_jProjModified->valuestring);
		}
		else {
			printf("Warning: missing 'lastModified' node in %s\n", fname);
		}
	}
	free(jsonData);
	return p;
}
void CSaveLoad::saveProjectToFile(class CProject *projToSave, const char *fname) {

	cJSON *root_proj = cJSON_CreateObject();
	cJSON_AddStringToObject(root_proj, "created", projToSave->getCreated());
	cJSON_AddStringToObject(root_proj, "lastModified", projToSave->getLastModified());

	char *msg = cJSON_Print(root_proj);

	FS_WriteTextFile(msg, fname);

	free(msg);
}
class CSimulation *CSaveLoad::loadSimulationFromFile(const char *fname) {
	CSimulation *s;
	char *jsonData;
	jsonData = FS_ReadTextFile(fname);
	if (jsonData == 0) {
		return 0;
	}
	s = new CSimulation();
	s->setSimulator(sim);
	cJSON *n_jSim = cJSON_Parse(jsonData);
	cJSON *n_jSimSim = cJSON_GetObjectItemCaseSensitive(n_jSim, "simulation");
	cJSON *n_jWires = cJSON_GetObjectItemCaseSensitive(n_jSimSim, "wires");
	cJSON *n_jObjects = cJSON_GetObjectItemCaseSensitive(n_jSimSim, "objects");
	cJSON *jObject;
	cJSON *jWire;
	cJSON_ArrayForEach(jObject, n_jObjects)
	{
		cJSON *jX = cJSON_GetObjectItemCaseSensitive(jObject, "x");
		cJSON *jY = cJSON_GetObjectItemCaseSensitive(jObject, "y");
		cJSON *jClassName = cJSON_GetObjectItemCaseSensitive(jObject, "classname");
		cJSON *jName = cJSON_GetObjectItemCaseSensitive(jObject, "name");
		cJSON *jText = cJSON_GetObjectItemCaseSensitive(jObject, "text");
		cJSON *rot = cJSON_GetObjectItemCaseSensitive(jObject, "rotation");
		int rotInteger = rot->valuedouble;
		rotInteger %= 360;

		CShape *o = sim->getPfbs()->instantiatePrefab(jName->valuestring);
		if (o == 0) {
			o = sim->allocByClassName(jClassName->valuestring);
		}
		if (o == 0) {
			printf("Failed to alloc object of class %s\n", jClassName->valuestring);
		}
		else {
			CText *as_text = dynamic_cast<CText*>(o);
			o->setPosition(jX->valuedouble, jY->valuedouble);
			s->addObject(o);
			o->rotateDegreesAroundSelf(rotInteger);
			if (jText != 0 && as_text != 0) {
				as_text->setText(jText->valuestring);
			}
			class CControllerBase *cb = o->getController();
			if (cb) {
				cb->loadFrom(jObject);
			}
		}
	}
	cJSON_ArrayForEach(jWire, n_jWires)
	{
		cJSON *jX0 = cJSON_GetObjectItemCaseSensitive(jWire, "x0");
		cJSON *jY0 = cJSON_GetObjectItemCaseSensitive(jWire, "y0");
		cJSON *jX1 = cJSON_GetObjectItemCaseSensitive(jWire, "x1");
		cJSON *jY1 = cJSON_GetObjectItemCaseSensitive(jWire, "y1");
		s->addWire(jX0->valuedouble, jY0->valuedouble, jX1->valuedouble, jY1->valuedouble);
	}
	s->matchAllJunctions();
	s->recalcBounds();
	free(jsonData);
	return s;
}
void CSaveLoad::saveSimulationToFile(class CSimulation *simToSave, const char *fname) {

	//sim->saveTo(simPath.c_str());
	cJSON *root_sim = cJSON_CreateObject();
	cJSON *main_sim = cJSON_AddObjectToObject(root_sim, "simulation");
	cJSON *main_objects = cJSON_AddObjectToObject(main_sim, "objects");
	for (int i = 0; i < simToSave->getObjectsCount(); i++) {
		CShape *obj = simToSave->getObject(i);
		cJSON *j_obj = cJSON_AddObjectToObject(main_objects, "object");
		const Coord &pos = obj->getPosition();
		float rot = obj->getRotationAccum();
		const char *name = obj->getName();
		const char *classname = obj->getClassName();
		CText *as_text = dynamic_cast<CText*>(obj);
		cJSON_AddStringToObject(j_obj, "name", name);
		cJSON_AddStringToObject(j_obj, "classname", classname);
		cJSON_AddNumberToObject(j_obj, "rotation", rot);
		cJSON_AddNumberToObject(j_obj, "x", pos.getX());
		cJSON_AddNumberToObject(j_obj, "y", pos.getY());
		if (as_text) {
			cJSON_AddStringToObject(j_obj, "text", as_text->getText());
		}
		class CControllerBase *cb = obj->getController();
		if (cb) {
			cb->saveTo(j_obj);
		}
	}
	cJSON *main_wires = cJSON_AddObjectToObject(main_sim, "wires");
	for (int i = 0; i < simToSave->getWiresCount(); i++) {
		CWire *wire = simToSave->getWires(i);
		cJSON *j_wire = cJSON_AddObjectToObject(main_wires, "wire");
		CJunction *jA = wire->getJunction(0);
		CJunction *jB = wire->getJunction(1);
		cJSON_AddNumberToObject(j_wire, "x0", jA->getX());
		cJSON_AddNumberToObject(j_wire, "y0", jA->getY());
		cJSON_AddNumberToObject(j_wire, "x1", jB->getX());
		cJSON_AddNumberToObject(j_wire, "y1", jB->getY());
	}
	char *msg = cJSON_Print(root_sim);

	FS_WriteTextFile(msg, fname);

	free(msg);


}


#endif
