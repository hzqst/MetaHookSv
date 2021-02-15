#pragma once

#define LIGHTMAP_NUMCOLUMNS		8
#define LIGHTMAP_NUMROWS		8

#define LIGHTMAP_BYTES		4
#define	BLOCK_WIDTH			128
#define	BLOCK_HEIGHT		128
#define BLOCKLIGHTS_SIZE	(18*18)

#define	MAX_LIGHTMAPS			64
#define	MAX_LIGHTSTYLES			64
#define	MAX_STYLESTRING			64

#define MAX_DETAIL_TEXTURES		MAX_MAP_TEXTURES
#define BACKFACE_EPSILON	0.01

#define MAX_DECALSURFS 500
#define MAX_MODELS 512
#define COLINEAR_EPSILON 0.001

#define FDECAL_PERMANENT			0x01		// This decal should not be removed in favor of any new decals
#define FDECAL_REFERENCE			0x02		// This is a decal that's been moved from another level
#define FDECAL_CUSTOM               0x04        // This is a custom clan logo and should not be saved/restored
#define FDECAL_HFLIP				0x08		// Flip horizontal (U/S) axis
#define FDECAL_VFLIP				0x10		// Flip vertical (V/T) axis
#define FDECAL_CLIPTEST				0x20		// Decal needs to be clip-tested
#define FDECAL_NOCLIP				0x40		// Decal is not clipped by containing polygon


typedef struct brushvertex_s
{
	vec3_t	pos;
	vec3_t	normal;

	float	fogcoord;
	float	texcoord[2];
	float	detailtexcoord[2];
	float	lightmaptexcoord[2];

	byte	pad[12];
}brushvertex_t;

typedef struct
{
	texture_t *basetex;
	int detailtex;
	int replacetex;
	int normaltex;
	float detailscale[2];
	float replacescale[2];
	float replacealpha;
	qboolean loaded;
}maptexture_t;

typedef struct brushface_s
{
	int index;
	int start_vertex;
	int num_vertexes;
	maptexture_t *maptex;

	vec3_t	normal;
	vec3_t	s_tangent;
	vec3_t	t_tangent;
}brushface_t;

typedef struct
{
	char basetex[32];
	char detailtex[64];
	char replacetex[64];
	char normaltex[64];
	float detailscale[2];
	float replacescale[2];
	qboolean loaded;
}extratexture_t;

typedef struct
{
	extratexture_t		*pTextures;
	int					iNumTextures;
}extratexture_mgr_t;

typedef struct epair_s
{
   struct epair_s *next;
   char  *key;
   char  *value;
} epair_t;

typedef struct
{
   vec3_t      origin;
   epair_t     *epairs;
   char		*classname;
}bspentity_t;

typedef struct
{
	GLuint				hVBO;

	brushvertex_t		*pVertexBuffer;
	int					iNumVerts;
	int					iNumTris;

	brushface_t			*pFaceBuffer;
	int					iNumFaces;

	extratexture_mgr_t	ExtraTextures;
	extratexture_mgr_t	LocalExtraTextures;

	maptexture_t		MapTextures[MAX_MAP_TEXTURES];

	int					iSkyTextures[6];

	texture_t			DecalTextures[MAX_MAP_TEXTURES];
	int					iNumDecalTextures;

	int					iNumBSPEntities;
	bspentity_t			pBSPEntities[MAX_MAP_BSPENTITY];
}r_worldsurf_t;

#define OFFSET(type, variable) ((const void*)&(((type*)NULL)->variable))

extern r_worldsurf_t	r_wsurf;

void R_InitWSurf(void);
void R_VidInitWSurf(void);

//engine
extern byte *lightmaps;
extern int *lightmap_textures;
extern void *lightmap_rectchange;
extern int *lightmap_modified;
extern int *c_brush_polys;
extern int *d_lightstylevalue;
extern dlight_t *cl_dlights;
extern int *r_dlightactive;
extern int *gDecalSurfCount;
extern msurface_t **gDecalSurfs;
extern decal_t *gDecalPool;
extern decalcache_t *gDecalCache;

//cvar
extern cvar_t *r_wsurf_replace;
extern cvar_t *r_wsurf_sky;

void R_ClearBSPEntities(void);
void R_ParseBSPEntities(char *data);
char *ValueForKey(bspentity_t *ent, char *key);
void R_LoadBSPEntities(void);

void R_LinkDecalTexture(texture_t *t);
void R_LoadExtraTextureFile(qboolean loadmap);
void R_InitDetailTextures(void);
void R_RenderDynamicLightmaps(msurface_t *fa);
void R_AddDynamicLights(msurface_t *surf);
void R_BuildLightMap(msurface_t *psurf, byte *dest, int stride);
void R_DecalMPoly(float *v, texture_t *ptexture, msurface_t *psurf, int vertCount);
void R_DrawDecals(qboolean bMultitexture);