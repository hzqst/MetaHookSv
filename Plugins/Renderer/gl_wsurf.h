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

#define MAX_DECALVERTS 32
#define MAX_DECALS 4096


#define FDECAL_PERMANENT			0x01		// This decal should not be removed in favor of any new decals
#define FDECAL_REFERENCE			0x02		// This is a decal that's been moved from another level
#define FDECAL_CUSTOM               0x04        // This is a custom clan logo and should not be saved/restored
#define FDECAL_HFLIP				0x08		// Flip horizontal (U/S) axis
#define FDECAL_VFLIP				0x10		// Flip vertical (V/T) axis
#define FDECAL_CLIPTEST				0x20		// Decal needs to be clip-tested
#define FDECAL_NOCLIP				0x40		// Decal is not clipped by containing polygon
#define FDECAL_VBO					0x1000		// Decalvertex is bufferred in VBO

#define MAX_NUM_NODES 16

#define BINDING_POINT_SCENE_UBO 0
#define BINDING_POINT_DECAL_SSBO 1
#define BINDING_POINT_TEXTURE_SSBO 1
#define BINDING_POINT_ENTITY_UBO 2
#define BINDING_POINT_STUDIO_UBO 2
#define BINDING_POINT_OIT_FRAGMENT_SSBO 3
#define BINDING_POINT_OIT_NUMFRAGMENT_SSBO 4
#define BINDING_POINT_OIT_COUNTER_SSBO 5

typedef struct decalvertex_s
{
	vec3_t	pos;
	float	texcoord[3];//[2]=unused
	float	lightmaptexcoord[3];//[2]=lightmaptexnum
	int		decalindex;
}decalvertex_t;

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
	float	speculartexcoord[2];
}brushvertex_t;

typedef struct brushface_s
{
	int index;
	int flags;
	std::vector<int> start_vertex;
	std::vector<int> num_vertexes;
	int num_polys;

	vec3_t	normal;
	vec3_t	s_tangent;
	vec3_t	t_tangent;
}brushface_t;

#define TEXCHAIN_STATIC 1
#define TEXCHAIN_SCROLL 2
#define TEXCHAIN_RANDOM 2
#define TEXCHAIN_ANIMATION 3
#define TEXCHAIN_SKY 4

typedef struct brushtexchain_s
{
	struct brushtexchain_s()
	{
		iStartIndex = 0;
		iIndiceCount = 0;
		iPolyCount = 0;
		pTexture = 0;
		iDetailTextureFlags = 0;
		iType = 0;
	}
	int iStartIndex;
	int iIndiceCount;
	int iPolyCount;
	texture_t *pTexture;
	int iDetailTextureFlags;
	int iType;
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

typedef bspentity_t *(*fnParseBSPEntity_Allocator)(void);

#define WSURF_REPLACE_TEXTURE		0
#define WSURF_DETAIL_TEXTURE		1
#define WSURF_NORMAL_TEXTURE		2
#define WSURF_PARALLAX_TEXTURE		3
#define WSURF_SPECULAR_TEXTURE		4
#define WSURF_MAX_TEXTURE			5

#define WSURF_TEXCHAIN_STATIC		0
#define WSURF_TEXCHAIN_ANIM			1
#define WSURF_TEXCHAIN_MAX			2

#define WSURF_DRAWBATCH_STATIC		0
#define WSURF_DRAWBATCH_SOLID		1
#define WSURF_DRAWBATCH_MAX			2

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

typedef struct wsurf_vbo_batch_s
{
	wsurf_vbo_batch_s()
	{
		iDetailTextureFlags = 0;
		iBaseDrawId = 0;
		iDrawCount = 0;
		iPolyCount = 0;
	}
	int iDetailTextureFlags;
	int iBaseDrawId;
	int iDrawCount;
	int iPolyCount;
	std::vector<void *> vStartIndex;
	std::vector<GLsizei> vIndiceCount;
}wsurf_vbo_batch_t;

typedef struct wsurf_vbo_s
{
	wsurf_vbo_s()
	{
		pModel = NULL;
		hEBO = 0;
		hEntityUBO = 0;
		hTextureSSBO = 0;
		hDecalEBO = 0;
	}

	model_t	*pModel;
	GLuint	hEBO;
	GLuint	hEntityUBO;
	GLuint	hTextureSSBO;
	GLuint	hDecalEBO;
	std::vector<brushtexchain_t> vTextureChain[WSURF_TEXCHAIN_MAX];
	std::vector<wsurf_vbo_batch_t *> vDrawBatch[WSURF_DRAWBATCH_MAX];
	brushtexchain_t TextureChainSky;
}wsurf_vbo_t;

#pragma pack(push, 16)

typedef struct scene_ubo_s
{
	mat4 viewMatrix;
	mat4 projMatrix;
	mat4 invViewMatrix;
	mat4 invProjMatrix;
	mat4 shadowMatrix[3];
	uvec4 viewport;//viewport.z=linkListSize
	vec4 frustumpos[4];
	vec4 viewpos;
	vec4 vpn;
	vec4 vright;
	vec4 vup;
	vec4 shadowDirection;
	vec4 shadowColor;
	vec4 shadowFade;
	vec4 clipPlane;
	vec4 fogColor;
	float fogStart;
	float fogEnd;
	float time;
	float r_g;
	float r_g3;
	float v_brightness;
	float v_lightgamma;
	float v_lambert;
	float v_gamma;
	float v_texgamma;
}scene_ubo_t;

typedef struct entity_ubo_s
{
	mat4 entityMatrix;
	vec4 color;
	float scrollSpeed;
}entity_ubo_t;

typedef struct studio_ubo_s
{
	float r_ambientlight;
	float r_shadelight;
	float r_blend;
	float r_scale;
	vec4 r_plightvec;
	vec4 r_colormix;
	vec4 r_origin;
	vec4 entity_origin;
	mat3x4 bonematrix[128];
}studio_ubo_t;

#pragma pack(pop)

typedef struct texture_ssbo_s
{
	GLuint64 handles[5 * 1];
}texture_ssbo_t;

// A fragment node stores rendering information about one specific fragment
typedef struct FragmentNode_s
{
	uint32_t color;
	float depth;
	uint32_t next;
}FragmentNode;

typedef struct r_worldsurf_s
{
	r_worldsurf_s()
	{
		hSceneVBO = 0;
		hSceneUBO = 0;
		hDecalVBO = 0;
		hDecalSSBO = 0;
		hOITFragmentSSBO = 0;
		hOITNumFragmentSSBO = 0;
		hOITAtomicSSBO = 0;

		bDiffuseTexture = false;
		bLightmapTexture = false;
		bShadowmapTexture = false;
		bDetailTexture = false;
		bNormalTexture = false;
		bParallaxTexture = false;
		bSpecularTexture = false;

		iNumLightmapTextures = 0;
		iLightmapTextureArray = 0;
	}

	GLuint				hSceneVBO;
	GLuint				hSceneUBO;
	GLuint				hDecalVBO;
	GLuint				hDecalSSBO;
	GLuint				hOITFragmentSSBO;
	GLuint				hOITNumFragmentSSBO;
	GLuint				hOITAtomicSSBO;

	std::vector <brushvertex_t> vVertexBuffer;
	std::vector <brushface_t> vFaceBuffer;

	bool				bDiffuseTexture;
	bool				bLightmapTexture;
	bool				bShadowmapTexture;
	bool				bDetailTexture;
	bool				bNormalTexture;
	bool				bParallaxTexture;
	bool				bSpecularTexture;

	int					iNumLightmapTextures;
	int					iLightmapTextureArray;

	std::vector <bspentity_t> vBSPEntities;

	std::vector <int> vDecalGLTextures;
	std::vector <GLuint64> vDecalGLTextureHandles;
	std::vector <GLint> vDecalStartIndex;
	std::vector <GLsizei> vDecalVertexCount;
}r_worldsurf_t;

typedef struct
{
	int program;
	int u_parallaxScale;
	int u_baseDrawId;
}wsurf_program_t;

#define OFFSET(type, variable) ((const void*)&(((type*)NULL)->variable))

extern r_worldsurf_t r_wsurf;
extern int r_wsurf_drawcall;
extern int r_wsurf_polys;
extern int r_fog_mode;
extern float r_fog_control[2];
extern float r_fog_color[4];
extern float r_shadow_matrix[3][16];
extern vec3_t r_frustum_origin[4];
extern vec3_t r_frustum_vec[4];
extern float r_world_matrix_inv[16];
extern float r_proj_matrix_inv[16];
extern float r_near_z;
extern float r_far_z;
extern bool r_ortho;

extern cl_entity_t *g_OITBlendObjects[512];
extern int g_iNumOITBlendObjects;

void R_InitWSurf(void);
void R_NewMapWSurf(void);

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

//cvar

extern cvar_t *r_wsurf_parallax_scale;
extern cvar_t *r_wsurf_sky_occlusion;
extern cvar_t *r_wsurf_zprepass;
extern cvar_t *r_wsurf_detail;

void FreeBSPEntity(bspentity_t *ent);
void R_ClearBSPEntities(void);
void R_ParseBSPEntities(char *data, fnParseBSPEntity_Allocator fn);
bspentity_t *R_ParseBSPEntity_DefaultAllocator(void);
char *ValueForKey(bspentity_t *ent, char *key);
void R_LoadBSPEntities(void);
void R_LoadExternalEntities(void);
void R_AddDynamicLights(msurface_t *surf);
void R_RenderDynamicLightmaps(msurface_t *fa);
void R_BuildLightMap(msurface_t *psurf, byte *dest, int stride);
void R_DrawDecals(void);
detail_texture_cache_t *R_FindDetailTextureCache(int texId);
void R_BeginDetailTexture(int texId);
void R_EndDetailTexture(void);
void R_DrawSequentialPolyVBO(msurface_t *s);
wsurf_vbo_t *R_PrepareWSurfVBO(model_t *mod);
void R_EnableWSurfVBO(wsurf_vbo_t *modcache);
void R_DrawWSurfVBO(wsurf_vbo_t *modcache, cl_entity_t *ent);
void R_DrawWSurfVBOSolid(wsurf_vbo_t *modcache);
void R_ShutdownWSurf(void);
void R_Reload_f(void);

void R_UseWSurfProgram(int state, wsurf_program_t *progOut);

#define WSURF_DIFFUSE_ENABLED				1
#define WSURF_LIGHTMAP_ENABLED				2
#define WSURF_DETAILTEXTURE_ENABLED			4
#define WSURF_NORMALTEXTURE_ENABLED			8
#define WSURF_PARALLAXTEXTURE_ENABLED		0x10
#define WSURF_SPECULARTEXTURE_ENABLED		0x20
#define WSURF_LINEAR_FOG_ENABLED			0x40
#define WSURF_GBUFFER_ENABLED				0x80
#define WSURF_TRANSPARENT_ENABLED			0x100
#define WSURF_SHADOW_CASTER_ENABLED			0x200
#define WSURF_SHADOWMAP_ENABLED				0x400
#define WSURF_SHADOWMAP_HIGH_ENABLED		0x800
#define WSURF_SHADOWMAP_MEDIUM_ENABLED		0x1000
#define WSURF_SHADOWMAP_LOW_ENABLED			0x2000
#define WSURF_BINDLESS_ENABLED				0x4000
#define WSURF_LEGACY_ENABLED				0x8000
#define WSURF_DECAL_ENABLED					0x10000
#define WSURF_CLIP_ENABLED					0x20000
#define WSURF_OIT_ALPHA_BLEND_ENABLED		0x40000
#define WSURF_OIT_ADDITIVE_BLEND_ENABLED	0x80000