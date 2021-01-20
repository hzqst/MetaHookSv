#pragma once

#define SPRITE_VERSION 2

//ckf3 hack
typedef struct
{
	float texcoord[4][2];
	int w, h;
	int tex;
}tgasprite_t;

#define kRenderAddColor 6

#define NL_PRESENT 0
#define NL_NEEDS_LOADED 1
#define NL_UNREFERENCED 2
#define NL_CLIENT 3

typedef struct auxvert_s
{
	float	fv[3];
}auxvert_t;

typedef struct alight_s
{
	int ambientlight;
	int shadelight;
	vec3_t color;
	float *plightvec;
}alight_t;