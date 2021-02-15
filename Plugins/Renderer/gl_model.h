#ifndef __MODEL__
#define __MODEL__

#include "modelgen.h"
#include "spritegn.h"

typedef struct
{
	vec3_t position;
}
mvertex_t;

#define SIDE_FRONT 0
#define SIDE_BACK 1
#define SIDE_ON 2

typedef struct mplane_s
{
	vec3_t normal;
	float dist;
	byte type;
	byte signbits;
	byte pad[2];
}
mplane_t;

typedef struct texture_s
{
	char name[16];
	unsigned width, height;
	int gl_texturenum;
	struct msurface_s *texturechain;
	int anim_total;
	int anim_min, anim_max;
	struct texture_s *anim_next;
	struct texture_s *alternate_anims;
	unsigned offsets[MIPLEVELS];
	unsigned char *pPal;
}texture_t;

#define SURF_PLANEBACK 2
#define SURF_DRAWSKY 4
#define SURF_DRAWSPRITE 8
#define SURF_DRAWTURB 0x10
#define SURF_DRAWTILED 0x20
#define SURF_DRAWBACKGROUND 0x40
#define SURF_UNDERWATER 0x80
#define SURF_DONTWARP 0x100

typedef struct
{
	unsigned short v[2];
	unsigned int cachededgeoffset;
}
medge_t;

typedef struct
{
	float vecs[2][4];
	float mipadjust;
	texture_t *texture;
	int flags;
}
mtexinfo_t;

#define VERTEXSIZE 7

typedef struct glpoly_s
{
	struct glpoly_s *next;
	struct glpoly_s *chain;
	int numverts;
	int flags;
	float verts[4][VERTEXSIZE];
}glpoly_t;

typedef struct msurface_s
{
	int visframe;
	mplane_t *plane;
	int flags;
	int firstedge;
	int numedges;
	short texturemins[2];
	short extents[2];
	int light_s, light_t;
	glpoly_t *polys;
	struct msurface_s *texturechain;
	mtexinfo_t *texinfo;
	int dlightframe;
	int dlightbits;
	int lightmaptexturenum;
	byte styles[MAXLIGHTMAPS];
	int cached_light[MAXLIGHTMAPS];
	qboolean cached_dlight;
	byte *samples;
	struct decal_s *pdecals;
}
msurface_t;

typedef struct decal_s
{
	struct decal_s *pnext;
	struct msurface_s *psurface;
	float dx;
	float dy;
	float scale;
	short texture;
	short flags;
	short entityIndex;
}
decal_t;

typedef struct decalcache_s
{
	int		decalIndex;
	float	decalVert[4][VERTEXSIZE];
} decalcache_t;

typedef struct mnode_s
{
	int contents;
	int visframe;
	float minmaxs[6];
	struct mnode_s *parent;
	mplane_t *plane;
	struct mnode_s *children[2];
	unsigned short firstsurface;
	unsigned short numsurfaces;
}
mnode_t;

typedef struct mleaf_s
{
	int contents;
	int visframe;
	float minmaxs[6];
	struct mnode_s *parent;
	byte *compressed_vis;
	struct efrag_s *efrags;
	msurface_t **firstmarksurface;
	int nummarksurfaces;
	int key;
	byte ambient_sound_level[NUM_AMBIENTS];
}
mleaf_t;

typedef struct hull_s
{
	dclipnode_t *clipnodes;
	mplane_t *planes;
	int firstclipnode;
	int lastclipnode;
	vec3_t clip_mins;
	vec3_t clip_maxs;
}
hull_t;

typedef struct mspriteframe_s
{
	int width;
	int height;
	float up, down, left, right;
	int gl_texturenum;
}
mspriteframe_t;

typedef struct
{
	int numframes;
	float *intervals;
	mspriteframe_t *frames[1];
}
mspritegroup_t;

typedef struct
{
	spriteframetype_t type;
	mspriteframe_t *frameptr;
}
mspriteframedesc_t;

typedef struct msprite_s
{
	short type;
	short texFormat;
	int maxwidth;
	int maxheight;
	int numframes;
	int paloffset;
	float beamlength;
	void *cachespot;
	mspriteframedesc_t frames[1];
}
msprite_t;

typedef struct
{
	int firstpose;
	int numposes;
	float interval;
	trivertx_t bboxmin;
	trivertx_t bboxmax;
	int frame;
	char name[16];
}
maliasframedesc_t;

typedef struct
{
	trivertx_t bboxmin;
	trivertx_t bboxmax;
	int frame;
}
maliasgroupframedesc_t;

typedef struct
{
	int numframes;
	int intervals;
	maliasgroupframedesc_t frames[1];
}
maliasgroup_t;

typedef struct mtriangle_s
{
	int facesfront;
	int vertindex[3];
}
mtriangle_t;

#define MAX_SKINS 32

typedef struct
{
	int ident;
	int version;
	vec3_t scale;
	vec3_t scale_origin;
	float boundingradius;
	vec3_t eyeposition;
	int numskins;
	int skinwidth;
	int skinheight;
	int numverts;
	int numtris;
	int numframes;
	synctype_t synctype;
	int flags;
	float size;
	int numposes;
	int poseverts;
	int posedata;
	int commands;
	int gl_texturenum[MAX_SKINS];
	maliasframedesc_t frames[1];
}
aliashdr_t;

#define MAXALIASVERTS 1024
#define MAXALIASFRAMES 256
#define MAXALIASTRIS 2048

extern aliashdr_t *pheader;
extern stvert_t stverts[MAXALIASVERTS];
extern mtriangle_t triangles[MAXALIASTRIS];
extern trivertx_t *poseverts[MAXALIASFRAMES];

typedef enum
{
	mod_brush,
	mod_sprite,
	mod_alias,
	mod_studio
}
modtype_t;

#define FMODEL_ROCKET 0x1
#define FMODEL_GRENADE 0x2
#define FMODEL_GIB 0x4
#define FMODEL_ROTATE 0x8
#define FMODEL_TRACER 0x10
#define FMODEL_ZOMGIB 0x20
#define FMODEL_TRACER2 0x40
#define FMODEL_TRACER3 0x80
#define FMODEL_DYNAMIC_LIGHT 0x100
#define FMODEL_TRACE_HITBOX 0x200

typedef struct model_s
{
	char name[MAX_QPATH];
	qboolean needload;
	modtype_t type;
	int numframes;
	synctype_t synctype;
	int flags;
	vec3_t mins, maxs;
	float radius;
	int firstmodelsurface, nummodelsurfaces;
	int numsubmodels;
	dmodel_t *submodels;
	int numplanes;
	mplane_t *planes;
	int numleafs;
	mleaf_t *leafs;
	int numvertexes;
	mvertex_t *vertexes;
	int numedges;
	medge_t *edges;
	int numnodes;
	mnode_t *nodes;
	int numtexinfo;
	mtexinfo_t *texinfo;
	int numsurfaces;
	msurface_t *surfaces;
	int numsurfedges;
	int *surfedges;
	int numclipnodes;
	dclipnode_t *clipnodes;
	int nummarksurfaces;
	msurface_t **marksurfaces;
	hull_t hulls[MAX_MAP_HULLS];
	int numtextures;
	texture_t **textures;
	byte *visdata;
	byte *lightdata;
	char *entities;
	cache_user_t cache;
}
model_t;

void Mod_Init(void);
void Mod_ClearAll(void);
model_t *Mod_ForName(const char *name, qboolean crash, qboolean trackCRC);
void *Mod_Extradata(model_t *mod);
void Mod_TouchModel(char *name);

mleaf_t *Mod_PointInLeaf(float *p, model_t *model);
byte *Mod_LeafPVS(mleaf_t *leaf, model_t *model);

#endif