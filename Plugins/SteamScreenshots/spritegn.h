#ifdef INCLUDELIBS

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#include "cmdlib.h"
#include "scriplib.h"
#include "lbmlib.h"

#endif

#define SPRITE_VERSION 2

#ifndef SYNCTYPE_T
#define SYNCTYPE_T
typedef enum
{
	ST_SYNC = 0,
	ST_RAND
}
synctype_t;
#endif

typedef struct
{
	int ident;
	int version;
	int type;
	int texFormat;
	float boundingradius;
	int width;
	int height;
	int numframes;
	float beamlength;
	synctype_t synctype;
}
dsprite_t;

#define SPR_VP_PARALLEL_UPRIGHT 0
#define SPR_FACING_UPRIGHT 1
#define SPR_VP_PARALLEL 2
#define SPR_ORIENTED 3
#define SPR_VP_PARALLEL_ORIENTED 4

#define SPR_NORMAL 0
#define SPR_ADDITIVE 1
#define SPR_INDEXALPHA 2
#define SPR_ALPHTEST 3

typedef struct
{
	int origin[2];
	int width;
	int height;
}
dspriteframe_t;

typedef struct
{
	int numframes;
}
dspritegroup_t;

typedef struct
{
	float interval;
}
dspriteinterval_t;

typedef enum
{
	SPR_SINGLE = 0,
	SPR_GROUP
}
spriteframetype_t;

typedef struct
{
	spriteframetype_t type;
}
dspriteframetype_t;

#define IDSPRITEHEADER (('P' << 24) + ('S' << 16) + ('D' << 8) + 'I')