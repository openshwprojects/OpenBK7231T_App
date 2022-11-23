#ifdef WINDOWS

#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <vector>
#include <algorithm>
#include <string>
#include <SDL.h>
#include <SDL_opengl.h>
#include <GL/gl.h>
#include <GL/glut.h>

typedef int32_t i32;
typedef uint32_t u32;

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

#define DEG2RAD(angleInDegrees) ((angleInDegrees) * M_PI / 180.0)
#define RAD2DEG(angleInRadians) ((angleInRadians) * 180.0 / M_PI)

#endif
