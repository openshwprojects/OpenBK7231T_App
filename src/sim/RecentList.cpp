#ifdef WINDOWS
#include "RecentList.h"
#include "../cJSON/cJSON.h"

void CRecentList::remove(const char *s) {
	for (int i = recents.size() - 1; i >= 0; i--) {
		if (!stricmp(recents[i].c_str(), s)) {
			recents.removeAt(i);
		}
	}

}
void CRecentList::registerAndSave(const char *s) {
	remove(s);
	recents.insert(recents.begin(), s);
	save();
}
bool CRecentList::load(const char *fname) {
	char *jsonData;
	jsonData = FS_ReadTextFile(fname);
	if (jsonData == 0) {
		return 0;
	}
	cJSON *n_jSim = cJSON_Parse(jsonData);
	cJSON *n_jObjects = cJSON_GetObjectItemCaseSensitive(n_jSim, "recents");
	cJSON *jObject;
	clear();
	cJSON_ArrayForEach(jObject, n_jObjects)
	{
		cJSON *cPath = cJSON_GetObjectItemCaseSensitive(jObject, "path");
		push_back(cPath->valuestring);
	}
	free(jsonData);
	return true;
}
void CRecentList::save(const char *fname) {
	cJSON *root_sim = cJSON_CreateObject();
	cJSON *main_objects = cJSON_AddObjectToObject(root_sim, "recents");
	for (int i = 0; i < size(); i++) {
		const char *path = get(i);
		cJSON *j_obj = cJSON_AddObjectToObject(main_objects, "recent");
		cJSON_AddStringToObject(j_obj, "path", path);
	}
	char *msg = cJSON_Print(root_sim);

	FS_WriteTextFile(msg, fname);

	free(msg);
}

#endif

