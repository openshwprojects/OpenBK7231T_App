#ifdef WINDOWS

#include "sim_local.h"
#include <vector>

class TextureManager {
	std::vector<class Texture2D *> loaded;

public:
	Texture2D *registerTexture(const char *fname);
};

#endif
