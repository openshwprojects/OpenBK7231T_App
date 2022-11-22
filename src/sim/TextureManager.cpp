#ifdef WINDOWS

#include "TextureManager.h"
#include "Texture.h"

Texture2D *TextureManager::registerTexture(const char *fname) {
	for (int i = 0; i < loaded.size(); i++) {
		if (!stricmp(loaded[i]->getName(), fname))
			return loaded[i];
	}
	Texture2D *t = new Texture2D();
	t->loadTexture(fname);
	loaded.push_back(t);
	return t;
}


#endif
