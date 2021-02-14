#pragma once

#define MAX_MAP_HULLS 4

#define MAX_MAP_MODELS 400
#define MAX_MAP_BRUSHES 4096
#define MAX_MAP_ENTITIES 1024
#define MAX_MAP_ENTSTRING (128 * 1024)

#define MAX_MAP_PLANES 32767
#define MAX_MAP_NODES 32767
#define MAX_MAP_CLIPNODES 32767
#define MAX_MAP_LEAFS 8192
#define MAX_MAP_VERTS 65535
#define MAX_MAP_FACES 65535
#define MAX_MAP_MARKSURFACES 65535
#define MAX_MAP_TEXINFO 8192
#define MAX_MAP_EDGES 256000
#define MAX_MAP_SURFEDGES 512000
#define MAX_MAP_TEXTURES 512
#define MAX_MAP_MIPTEX 0x200000
#define MAX_MAP_LIGHTING 0x200000
#define MAX_MAP_VISIBILITY 0x200000
#define MAX_MAP_BSPENTITY 0x1000

#define MAX_MAP_PORTALS 65536

#define MAX_KEY 32
#define MAX_VALUE 1024

#define BSPVERSION 30
#define QUAKE_BSPVERSION 30

typedef struct
{
	int fileofs, filelen;
}
lump_t;

#define LUMP_ENTITIES 0
#define LUMP_PLANES 1
#define LUMP_TEXTURES 2
#define LUMP_VERTEXES 3
#define LUMP_VISIBILITY 4
#define LUMP_NODES 5
#define LUMP_TEXINFO 6
#define LUMP_FACES 7
#define LUMP_LIGHTING 8
#define LUMP_CLIPNODES 9
#define LUMP_LEAFS 10
#define LUMP_MARKSURFACES 11
#define LUMP_EDGES 12
#define LUMP_SURFEDGES 13
#define LUMP_MODELS 14

#define HEADER_LUMPS 15

typedef struct
{
	float mins[3], maxs[3];
	float origin[3];
	int headnode[MAX_MAP_HULLS];
	int visleafs;
	int firstface, numfaces;
}
dmodel_t;

typedef struct
{
	int version;
	lump_t lumps[HEADER_LUMPS];
}
dheader_t;

typedef struct
{
	int nummiptex;
	int dataofs[4];
}
dmiptexlump_t;

#define MIPLEVELS 4

typedef struct miptex_s
{
	char name[16];
	unsigned width, height;
	unsigned offsets[MIPLEVELS];
}
miptex_t;

typedef struct
{
	float point[3];
}
dvertex_t;

#define PLANE_X 0
#define PLANE_Y 1
#define PLANE_Z 2

#define PLANE_ANYX 3
#define PLANE_ANYY 4
#define PLANE_ANYZ 5

typedef struct
{
	float normal[3];
	float dist;
	int type;
}
dplane_t;

#define CONTENTS_EMPTY -1
#define CONTENTS_SOLID -2
#define CONTENTS_WATER -3
#define CONTENTS_SLIME -4
#define CONTENTS_LAVA -5
#define CONTENTS_SKY -6
#define CONTENTS_ORIGIN -7
#define CONTENTS_CLIP -8

#define CONTENTS_CURRENT_0 -9
#define CONTENTS_CURRENT_90 -10
#define CONTENTS_CURRENT_180 -11
#define CONTENTS_CURRENT_270 -12
#define CONTENTS_CURRENT_UP -13
#define CONTENTS_CURRENT_DOWN -14

#define CONTENTS_TRANSLUCENT -15

typedef struct
{
	int planenum;
	short children[2];
	short mins[3];
	short maxs[3];
	unsigned short firstface;
	unsigned short numfaces;
}
dnode_t;

typedef struct
{
	int planenum;
	short children[2];
}
dclipnode_t;

typedef struct texinfo_s
{
	float vecs[2][4];
	int miptex;
	int flags;
}
texinfo_t;

#define TEX_SPECIAL 1

typedef struct
{
	unsigned short v[2];
}
dedge_t;

#define MAXLIGHTMAPS 4

typedef struct
{
	short planenum;
	short side;

	int firstedge;
	short numedges;	
	short texinfo;

	byte styles[MAXLIGHTMAPS];
	int lightofs;
}
dface_t;


#define AMBIENT_WATER 0
#define AMBIENT_SKY 1
#define AMBIENT_SLIME 2
#define AMBIENT_LAVA 3

#define NUM_AMBIENTS 4

typedef struct
{
	int contents;
	int visofs;

	short mins[3];
	short maxs[3];

	unsigned short firstmarksurface;
	unsigned short nummarksurfaces;

	byte ambient_level[NUM_AMBIENTS];
}
dleaf_t;