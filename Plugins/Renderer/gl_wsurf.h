#pragma once

#include <vector>

#define LIGHTMAP_NUMCOLUMNS		32
#define LIGHTMAP_NUMROWS		32

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
	vec3_t	s_tangent;
	vec3_t	t_tangent;

	float	texcoord[3];//texcoord[2]=1/texwidth
	float	lightmaptexcoord[3];//lightmaptexcoord[2]=lightmaptexturenum
	float	detailtexcoord[2];
	float	normaltexcoord[2];
	float	parallaxtexcoord[2];
}brushvertex_t;

typedef struct brushface_s
{
	int index;
	int start_vertex;
	int num_vertexes;

	vec3_t	normal;
	vec3_t	s_tangent;
	vec3_t	t_tangent;
}brushface_t;

typedef struct brushtexchain_s
{
	int iStartIndex;
	int iVertexCount;
	int iFaceCount;
	texture_t *pTexture;
	int iScroll;
}brushtexchain_t;

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

#define WSURF_REPLACE_TEXTURE		0
#define WSURF_DETAIL_TEXTURE		1
#define WSURF_NORMAL_TEXTURE		2
#define WSURF_PARALLAX_TEXTURE		3
#define WSURF_MAX_TEXTURE			4

typedef struct detail_texture_s
{
	detail_texture_s()
	{
		gltexturenum = 0;
		width = 0;
		height = 0;
		scaleX = 0;
		scaleY = 0;
	}
	int gltexturenum;
	int width, height;
	float scaleX, scaleY;
}detail_texture_t;

typedef struct detail_texture_cache_s
{
	std::string basetexture;
	detail_texture_t tex[WSURF_MAX_TEXTURE];
}detail_texture_cache_t;

typedef struct wsurf_model_s
{
	wsurf_model_s()
	{
		hEBO = 0;
	}

	GLuint				hEBO;

	std::vector<brushtexchain_t> vTextureChainStatic;
	std::vector<brushtexchain_t> vTextureChainScroll;
	std::vector<unsigned int> vIndicesBuffer;

}wsurf_model_t;

typedef struct r_worldsurf_s
{
	r_worldsurf_s()
	{
		hVBO = 0;

		vVertexBuffer = NULL;
		iNumVerts = 0;

		vFaceBuffer = NULL;
		iNumFaces = 0;

		bDiffuseTexture = false;
		bLightmapTexture = false;
		bDetailTexture = false;
		bNormalTexture = false;
		bParallaxTexture = false;

		pDetailTextureCache = NULL;
		pCurrentModel = NULL;

		iNumLightmapTextures = 0;
		iLightmapTextureArray = 0;

		iNumBSPEntities = 0;
		iS_Tangent = 0;
		iT_Tangent = 0;
	}

	GLuint				hVBO;

	brushvertex_t		*vVertexBuffer;
	int					iNumVerts;

	brushface_t			*vFaceBuffer;
	int					iNumFaces;

	bool				bDiffuseTexture;
	bool				bLightmapTexture;
	bool				bDetailTexture;
	bool				bNormalTexture;
	bool				bParallaxTexture;
	int					iS_Tangent;
	int					iT_Tangent;

	detail_texture_cache_t *pDetailTextureCache;
	wsurf_model_t		*pCurrentModel;

	int					iNumLightmapTextures;
	int					iLightmapTextureArray;

	int					iNumBSPEntities;
	bspentity_t			pBSPEntities[MAX_MAP_BSPENTITY];
}r_worldsurf_t;

typedef struct
{
	int program;
	int diffuseTex;
	int lightmapTexArray;
	int detailTex;
	int normalTex;
	int parallaxTex;
	int speed;
	int entitymatrix;
	int clipPlane;
	int viewpos;
	int parallaxScale;
	int s_tangent;
	int t_tangent;
}wsurf_program_t;

#define OFFSET(type, variable) ((const void*)&(((type*)NULL)->variable))

extern r_worldsurf_t	r_wsurf;
extern int r_wsurf_drawcall;
extern int r_wsurf_polys;
extern int r_wsurf_fogmode;
extern float r_wsurf_fogcontrol[2];
extern float r_wsurf_fogcolor[4];

void R_InitWSurf(void);
void R_VidInitWSurf(void);

//engine
extern byte *lightmaps;
extern int *lightmap_textures;
extern void *lightmap_rectchange;
extern int *lightmap_modified;
extern glpoly_t **lightmap_polys;
extern int *d_lightstylevalue;
extern dlight_t *cl_dlights;
extern int *r_dlightactive;
extern int *gDecalSurfCount;
extern msurface_t **gDecalSurfs;
extern decal_t *gDecalPool;
extern decalcache_t *gDecalCache;
extern int *r_detail_texid;
extern float *r_detail_texcoord;
extern float *r_polygon_offset;;

//cvar
extern cvar_t *r_wsurf_vbo;

void R_ClearBSPEntities(void);
void R_ParseBSPEntities(char *data);
char *ValueForKey(bspentity_t *ent, char *key);
void R_LoadBSPEntities(void);

void R_AddDynamicLights(msurface_t *surf);
void R_RenderDynamicLightmaps(msurface_t *fa);
void R_BuildLightMap(msurface_t *psurf, byte *dest, int stride);
void DrawGLPoly(glpoly_t *p);
void DrawGLPoly(msurface_t *fa);
void R_DrawDecals(qboolean bMultitexture);
void R_BeginDetailTexture(int texId);
void R_EndDetailTexture(void);

wsurf_model_t *R_PrepareWSurfVBO(model_t *mod);
void R_EnableWSurfVBO(wsurf_model_t *modcache);
void R_DrawWSurfVBO(wsurf_model_t *modcache);
void R_EnableWSurfVBOSolid(wsurf_model_t *modcache);
void R_DrawWSurfVBOSolid(wsurf_model_t *modcache);
void R_ShutdownWSurf(void);

wsurf_program_t *R_UseWSurfProgram(int state);

#define WSURF_DIFFUSE_ENABLED			1
#define WSURF_LIGHTMAP_ENABLED			2
#define WSURF_DETAILTEXTURE_ENABLED		4
#define WSURF_NORMALTEXTURE_ENABLED		8
#define WSURF_PARALLAXTEXTURE_ENABLED	0x10
#define WSURF_CLIP_ABOVE_ENABLED		0x20
#define WSURF_CLIP_UNDER_ENABLED		0x40
#define WSURF_LINEAR_FOG_ENABLED		0x80
#define WSURF_GBUFFER_ENABLED			0x100
#define WSURF_TRANSPARENT_ENABLED		0x200
#define WSURF_MAX_STATE					0x400