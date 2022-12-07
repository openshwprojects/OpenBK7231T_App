#ifndef __RECENTLIST_H__
#define __RECENTLIST_H__

#include "sim_local.h"

#define RECENTS_FILE_PATH "recents.json"

class CRecentList {
	TArray<CString> recents;

	void remove(const char *s);
public:
	int size() const {
		return recents.size();
	}
	const char *get(int i) const {
		return recents[i].c_str();
	}
	void push_back(const char *s) {
		recents.push_back(s);
	}
	int getSizeCappedAt(int mx) const {
		if (mx < recents.size())
			return mx;
		return recents.size();
	}
	void clear() {
		recents.clear();
	}
	void registerAndSave(const char *s);
	bool load(const char *path = RECENTS_FILE_PATH);
	void save(const char *path = RECENTS_FILE_PATH);

};

#endif 
