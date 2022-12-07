#ifdef WINDOWS

#include "Texture.h"

#include <IL/IL.h>
#include <IL/ILU.h>
#include <IL/ILUT.h>

#pragma comment (lib, "DevIL.lib")
#pragma comment (lib, "ILU.lib")
#pragma comment (lib, "ILUT.lib")


void Texture2D::loadTexture(const char *fname)
{
	unsigned int image_ID;
	static bool il_ready = false;

	if (il_ready == false) {
		ilInit();
		iluInit();
		ilutInit();
		il_ready = true;
	}

	ilutRenderer(ILUT_OPENGL);

	image_ID = ilutGLLoadImage((char*)fname);

	handle = image_ID;
	w = ilGetInteger(IL_IMAGE_WIDTH);
	h = ilGetInteger(IL_IMAGE_HEIGHT);
}

#endif
