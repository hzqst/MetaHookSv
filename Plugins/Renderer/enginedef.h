#pragma once

#include "bspfile.h"

#define	MAX_QPATH 64


#define NL_PRESENT 0
#define NL_NEEDS_LOADED 1
#define NL_UNREFERENCED 2
#define NL_CLIENT 3

//wad

#pragma pack(1)
typedef struct lumpinfo_s
{
	int filepos;
	int disksize;
	int size;
	char type;
	char compression;
	char pad1, pad2;
	char name[16];
}lumpinfo_t;
#pragma pack()

typedef struct
{
	char name[MAX_QPATH];
	cache_user_t cache;
}cacheentry_t;

typedef void (*PFNCACHE)(struct cachewad_s *wad, byte *data);

typedef struct cachewad_s
{
	char *name;
	cacheentry_t *cache;
	int cacheCount;
	int cacheMax;
	lumpinfo_t *lumps;
	int lumpCount;
	int cacheExtra;
	PFNCACHE pfnCacheBuild;
	int numpaths;
	char **basedirs;
	int *lumppathindices;
	qboolean tempWad;
}cachewad_t;

//r_trans

typedef struct
{
	cl_entity_t *pEnt;
	float distance;
}transObjRef;

//gl_rsurf

typedef struct glRect_s
{
	int l, t, w, h;
}glRect_t;

typedef struct vrect_s
{
	int x, y, width, height;
}vrect_t;

typedef struct
{
	int r, g, b;
}mcolor24_t;

typedef struct refdef_SvEngine_s
{
	vrect_t vrect;
	vec3_t vieworg;
	vec3_t viewangles;
	color24 ambientlight;
	byte padding;
	qboolean onlyClientDraws;
	qboolean useCamera;
	vec3_t r_camera_origin;
}refdef_SvEngine_t;

typedef struct refdef_GoldSrc_s
{
	vrect_t vrect;
	char padding[96];
	vec3_t vieworg;
	vec3_t viewangles;
	color24 ambientlight;
	byte padding2;
	qboolean onlyClientDraws;
}refdef_GoldSrc_t;

typedef struct skybox_s
{
	float v[2][6];
}skybox_t;

//client

#define MAX_CLIENTS 32
#define MAX_WEAPONS 64

#define MULTIPLAYER_BACKUP 64

#define MAX_PACKET_ENTITIES 256

#include <entity_state.h>
#include <weaponinfo.h>

typedef struct
{
	int num_entities;
	unsigned char flags[32];
	entity_state_t *entities;
}
packet_entities_t;

typedef struct
{
	double receivedtime;
	double latency;
	qboolean invalid;
	qboolean choked;
	entity_state_t playerstate[MAX_CLIENTS];
	double time;
	clientdata_t clientdata;
	weapon_data_t weapondata[64];
	packet_entities_t packet_entities;
	unsigned short clientbytes;
	unsigned short playerinfobytes;
	unsigned short packetentitybytes;
	unsigned short tentitybytes;
	unsigned short soundbytes;
	unsigned short eventbytes;
	unsigned short usrbytes;
	unsigned short voicebytes;
	unsigned short msgbytes;
}frame_t;

#define TEX_TYPE_NONE	0
#define TEX_TYPE_ALPHA	1
#define TEX_TYPE_LUM	2
#define TEX_TYPE_ALPHA_GRADIENT 3
#define TEX_TYPE_RGBA	4

#define TEX_IS_ALPHA(type)		((type)==TEX_TYPE_ALPHA||(type)==TEX_TYPE_ALPHA_GRADIENT||(type)==TEX_TYPE_RGBA)

//gl_draw

#define MAX_GLTEXTURES 4800

#pragma pack(1)
typedef struct gltexture_s
{
	int texnum;
	short servercount;
	short paletteIndex;
	int width;
	int height;
	qboolean mipmap;
	char identifier[64];
}gltexture_t;
#pragma pack()

typedef enum
{
	GLT_SYSTEM,
	GLT_DECAL,
	GLT_HUDSPRITE,
	GLT_STUDIO,
	GLT_WORLD,
	GLT_SPRITE
}GL_TEXTURETYPE;

//gl_studio

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

#define STUDIO_RENDER 1
#define STUDIO_EVENTS 2

// lighting options
#define STUDIO_NF_FLATSHADE		0x0001
#define STUDIO_NF_CHROME		0x0002
#define STUDIO_NF_FULLBRIGHT	0x0004
#define STUDIO_NF_NOMIPS        0x0008
#define STUDIO_NF_ALPHA         0x0010
#define STUDIO_NF_ADDITIVE      0x0020
#define STUDIO_NF_MASKED        0x0040
#define STUDIO_NF_CELSHADE		0x1000
#define STUDIO_NF_CELSHADE_FACE	0x2000

#define EF_ROCKET			1			//! leave a trail
#define EF_GRENADE			2			//! leave a trail
#define EF_GIB				4			//! leave a trail
#define EF_ROTATE			8			//! rotate (bonus items)
#define EF_TRACER			0x10			//! green split trail
#define EF_ZOMGIB			0x20			//! small blood trail
#define EF_TRACER2			0x40			//! orange split trail + rotate
#define EF_TRACER3			0x80			//! purple trail
#define EF_NOSHADELIGHT		0x100			//! No shade lighting
#define EF_HITBOXCOLLISIONS	0x200			//! Use hitbox collisions
#define EF_FORCESKYLIGHT	0x400		//! Forces the model to be lit by skybox lighting

#define EF_OUTLINE			0x1000

#define kRenderFxOutline 100
#define kRenderFxShadowCaster 101

//gl_model

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
}msurface_t;

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
	//int ssbo_index;//Added by hooking Hunk_Alloc
}mspriteframe_t;

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
}mspriteframedesc_t;

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
}model_t;

typedef void(*cvar_callback_t)(cvar_t *pcvar);

typedef struct cvar_callback_entry_s
{
	cvar_callback_t callback;
	cvar_t *pcvar;
	struct cvar_callback_entry_s *next;
}cvar_callback_entry_t;

typedef struct overviewInfo_s
{
	vec3_t origin;
	float z_min, z_max;
	float zoom;
	qboolean rotated;
}overviewInfo_t;