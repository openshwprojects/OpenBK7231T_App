#ifdef WINDOWS

#include "sim_local.h"

class Texture2D {
	std::string name;
	unsigned int handle;
	int w, h;
public:
	void loadTexture(const char *fname);

	const char *getName() const {
		return name.c_str();
	}
	unsigned int getHandle() {
		return handle;
	}
};

#endif
