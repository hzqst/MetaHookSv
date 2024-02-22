#pragma once

#include <vector>

#define MAX_NUM_NODES 16

#define BINDING_POINT_SCENE_UBO 0
#define BINDING_POINT_DLIGHT_UBO 1

#define BINDING_POINT_SKYBOX_SSBO 2
#define BINDING_POINT_DECAL_SSBO 2
#define BINDING_POINT_TEXTURE_SSBO 2

#define BINDING_POINT_ENTITY_UBO 3
#define BINDING_POINT_STUDIO_UBO 3

#define BINDING_POINT_OIT_FRAGMENT_SSBO 4
#define BINDING_POINT_OIT_NUMFRAGMENT_SSBO 5
#define BINDING_POINT_OIT_COUNTER_SSBO 6

#define VERTEX_ATTRIBUTE_INDEX_POSITION 0
#define VERTEX_ATTRIBUTE_INDEX_NORMAL 1
#define VERTEX_ATTRIBUTE_INDEX_S_TANGENT 2
#define VERTEX_ATTRIBUTE_INDEX_T_TANGENT 3
#define VERTEX_ATTRIBUTE_INDEX_TEXCOORD 4
#define VERTEX_ATTRIBUTE_INDEX_LIGHTMAP_TEXCOORD 5
#define VERTEX_ATTRIBUTE_INDEX_REPLACETEXTURE_TEXCOORD 6
#define VERTEX_ATTRIBUTE_INDEX_DETAILTEXTURE_TEXCOORD 7
#define VERTEX_ATTRIBUTE_INDEX_NORMALTEXTURE_TEXCOORD 8
#define VERTEX_ATTRIBUTE_INDEX_PARALLAXTEXTURE_TEXCOORD 9
#define VERTEX_ATTRIBUTE_INDEX_SPECULARTEXTURE_TEXCOORD 10

#define VERTEX_ATTRIBUTE_INDEX_DECALINDEX 11
#define VERTEX_ATTRIBUTE_INDEX_TEXINDEX 11

#define VERTEX_ATTRIBUTE_INDEX_STYLES 12

#define WSURF_DIFFUSE_TEXTURE		0
#define WSURF_REPLACE_TEXTURE		1
#define WSURF_DETAIL_TEXTURE		2
#define WSURF_NORMAL_TEXTURE		3
#define WSURF_PARALLAX_TEXTURE		4
#define WSURF_SPECULAR_TEXTURE		5
#define WSURF_MAX_TEXTURE			6

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

typedef struct decalvertex_s
{
	vec3_t	pos;
	//for parallax mapping?
	vec3_t	normal;
	vec3_t	s_tangent;
	vec3_t	t_tangent;
	float	texcoord[3];//[2]=unused
	float	lightmaptexcoord[3];//[2]=lightmaptexnum
	float	replacetexcoord[2];
	float	detailtexcoord[2];
	float	normaltexcoord[2];
	float	parallaxtexcoord[2];
	float	speculartexcoord[2];
	int		decalindex;
	unsigned char styles[4];
}decalvertex_t;

typedef struct brushvertex_s
{
	vec3_t	pos;
	vec3_t	normal;
	vec3_t	s_tangent;
	vec3_t	t_tangent;

	float	texcoord[3];//texcoord[2]=1/texwidth
	float	lightmaptexcoord[3];//lightmaptexcoord[2]=lightmaptexturenum
	float	replacetexcoord[2];
	float	detailtexcoord[2];
	float	normaltexcoord[2];
	float	parallaxtexcoord[2];
	float	speculartexcoord[2];
	int		texindex;
	byte	styles[4];
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
#define TEXCHAIN_SKY 4

typedef struct brushtexchain_s
{
	struct brushtexchain_s()
	{
		iStartIndex = 0;
		iIndiceCount = 0;
		iPolyCount = 0;
		pTexture = 0;
		pDetailTextureCache = NULL;
		iDetailTextureFlags = 0;
		iType = 0;
	}
	int iStartIndex;
	int iIndiceCount;
	int iPolyCount;
	texture_t *pTexture;
	detail_texture_cache_t *pDetailTextureCache;
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


typedef struct wsurf_vbo_leaf_s
{
	struct wsurf_vbo_leaf_s()
	{
		hVAO = NULL;
		hEBO = NULL;
	}

	GLuint hVAO;
	GLuint hEBO;
	std::vector<brushtexchain_t> vTextureChain[WSURF_TEXCHAIN_MAX];
	std::vector<wsurf_vbo_batch_t *> vDrawBatch[WSURF_DRAWBATCH_MAX];
	std::vector<water_vbo_t *> vWaterVBO;
	brushtexchain_t TextureChainSky;
}wsurf_vbo_leaf_t;

typedef struct wsurf_vbo_s
{
	wsurf_vbo_s()
	{
		pModel = NULL;
		hEntityUBO = 0;
	}

	model_t	*pModel;
	GLuint	hEntityUBO;
	std::vector<wsurf_vbo_leaf_t *> vLeaves;
}wsurf_vbo_t;

#pragma pack(push, 16)

//viewport.z=linkListSize
typedef struct scene_ubo_s
{
	mat4 viewMatrix;
	mat4 projMatrix;
	mat4 invViewMatrix;
	mat4 invProjMatrix;
	mat4 shadowMatrix[3];
	uvec4 viewport;
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
	float fogDensity;
	float time;
	float r_g;
	float r_g3;
	float v_brightness;
	float v_lightgamma;
	float v_lambert;
	float v_gamma;
	float v_texgamma;
	float z_near;
	float z_far;
	float alphamin;
	float r_additive_shift;
	float r_lightscale;
	vec4 r_filtercolor;
	vec4 r_lightstylevalue[256 / 4];
}scene_ubo_t;

static_assert((sizeof(scene_ubo_t) % 16) == 0, "Size check");

typedef struct dlight_ubo_s
{
	vec4 origin_radius[256];
	vec4 color_minlight[256];
	uint32_t active_dlights[4];
}dlight_ubo_t;

static_assert((sizeof(dlight_ubo_t) % 16) == 0, "Size check");

typedef struct entity_ubo_s
{
	mat4 entityMatrix;
	vec4 color;
	float scrollSpeed;
	float padding1;
	float padding2;
	float padding3;
}entity_ubo_t;

static_assert((sizeof(entity_ubo_t) % 16) == 0, "Size check");

typedef struct studio_ubo_s
{
	float r_ambientlight;
	float r_shadelight;
	float r_scale;
	int r_numelight;
	vec4 r_plightvec;
	vec4 r_color;
	vec4 r_origin;
	vec4 entity_origin;
	vec4 r_elight_color[4];
	vec4 r_elight_origin[4];
	vec4 r_elight_radius;
	mat3x4 bonematrix[128];
}studio_ubo_t;

static_assert((sizeof(studio_ubo_t) % 16) == 0, "Size check");

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

//for decal drawing
typedef struct decal_drawbatch_s
{
	GLuint GLTextureId[MAX_DECALS];
	detail_texture_cache_t *DetailTextureCaches[MAX_DECALS];
	GLint StartIndex[MAX_DECALS];
	GLsizei VertexCount[MAX_DECALS];
	int BatchCount;
}decal_drawbatch_t;

typedef struct r_worldsurf_s
{
	r_worldsurf_s()
	{
		hSceneVBO = 0;
		hSceneUBO = 0;
		hDLightUBO = 0;
		hDecalVBO = 0;
		hDecalVAO = 0;
		hDecalSSBO = 0;
		hSkyboxSSBO = 0;
		hDetailSkyboxSSBO = 0;
		hWorldSSBO = 0;
		hOITFragmentSSBO = 0;
		hOITNumFragmentSSBO = 0;
		hOITAtomicSSBO = 0;

		bDiffuseTexture = false;
		bLightmapTexture = false;
		bShadowmapTexture = false;

		iNumLightmapTextures = 0;
		iLightmapTextureArray = 0;
		iLightmapUsedBits = 0;
		iLightmapLegacyDLights = 0;

		memset(vSkyboxTextureId, 0, sizeof(vSkyboxTextureId));
		memset(vSkyboxTextureHandles, 0, sizeof(vSkyboxTextureHandles));

		memset(vDecalGLTextures, 0, sizeof(vDecalGLTextures));
		memset(vDecalDetailTextures, 0, sizeof(vDecalDetailTextures));
		memset(vDecalStartIndex, 0, sizeof(vDecalStartIndex));
		memset(vDecalVertexCount, 0, sizeof(vDecalVertexCount));
	}

	GLuint				hSceneVBO;
	GLuint				hSceneUBO;
	GLuint				hDLightUBO;
	GLuint				hDecalVBO;
	GLuint				hDecalVAO;
	GLuint				hDecalSSBO;
	GLuint				hSkyboxSSBO;
	GLuint				hDetailSkyboxSSBO;
	GLuint				hWorldSSBO;
	GLuint				hOITFragmentSSBO;
	GLuint				hOITNumFragmentSSBO;
	GLuint				hOITAtomicSSBO;

	std::vector <brushface_t> vFaceBuffer;

	bool				bDiffuseTexture;
	bool				bLightmapTexture;
	bool				bShadowmapTexture;

	int					iNumLightmapTextures;
	int					iLightmapTextureArray;
	int					iLightmapUsedBits;
	int					iLightmapLegacyDLights;

	int vSkyboxTextureId[12];
	GLuint64 vSkyboxTextureHandles[12];

	GLuint vDecalGLTextures[MAX_DECALS];
	detail_texture_cache_t *vDecalDetailTextures[MAX_DECALS];
	GLint vDecalStartIndex[MAX_DECALS];
	GLsizei vDecalVertexCount[MAX_DECALS];

	std::vector<GLuint64> vBindlessTextureHandles;

	std::vector <bspentity_t> vBSPEntities;
}r_worldsurf_t;

typedef struct
{
	int program;
	int u_parallaxScale;
}wsurf_program_t;

#define OFFSET(type, variable) ((const void*)&(((type*)NULL)->variable))

extern r_worldsurf_t r_wsurf;
extern int r_wsurf_drawcall;
extern int r_wsurf_polys;
extern bool r_fog_enabled;
extern int r_fog_mode;
extern float r_fog_control[3];
extern float r_fog_color[4];
extern float r_shadow_matrix[3][16];
extern vec3_t r_frustum_origin[4];
extern vec3_t r_frustum_vec[4];
extern float r_world_matrix_inv[16];
extern float r_projection_matrix_inv[16];
extern float r_viewmodel_projection_matrix[16];
extern float r_viewmodel_projection_matrix_inv[16];
extern float r_znear;
extern float r_zfar;
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
extern cvar_t *r_wsurf_sky_fog;
extern cvar_t *r_wsurf_zprepass;

void FreeBSPEntity(bspentity_t *ent);
void R_ClearBSPEntities(void);
void R_ParseBSPEntities(const char *data, fnParseBSPEntity_Allocator fn);
bspentity_t *R_ParseBSPEntity_DefaultAllocator(void);
const char *ValueForKey(bspentity_t *ent, const char *key);
const char* ValueForKeyEx(bspentity_t* ent, const char* key, epair_t** ppLastEPair);
void ValueForKeyExArray(bspentity_t* ent, const char* key, std::vector<const char*> &strArray);
void R_LoadBSPEntities(void);
void R_FreeLightmapTextures(void);
void R_LoadExternalEntities(void);
void R_LoadBaseDecalTextures(void);
void R_LoadBaseDetailTextures(void);
void R_LoadMapDetailTextures(void);

void R_AddDynamicLights(msurface_t *surf);
void R_RenderDynamicLightmaps(msurface_t *fa);
void R_BuildLightMap(msurface_t *psurf, byte *dest, int stride);

void R_DrawDecals(cl_entity_t *ent);
void R_PrepareDecals(void);

detail_texture_cache_t *R_FindDecalTextureCache(const std::string &decalname);
detail_texture_cache_t *R_FindDetailTextureCache(int texId);
void R_BeginDetailTextureByGLTextureId(int gltexturenum, program_state_t *WSurfProgramState);
void R_BeginDetailTextureByDetailTextureCache(detail_texture_cache_t *cache, program_state_t *WSurfProgramState);
void R_EndDetailTexture(program_state_t WSurfProgramState);
void R_DrawSequentialPolyVBO(msurface_t *s);
wsurf_vbo_t *R_PrepareWSurfVBO(model_t *mod);
void R_DrawWSurfVBO(wsurf_vbo_t *modcache, cl_entity_t *ent);
void R_DrawWSurfVBOSolid(wsurf_vbo_t *modcache);
void R_ShutdownWSurf(void);
void R_Reload_f(void);
void R_GenerateSceneUBO(void);
void R_SaveWSurfProgramStates(void);
void R_LoadWSurfProgramStates(void);
void R_UseWSurfProgram(program_state_t state, wsurf_program_t *progOut);
void R_CreateBindlessTexturesForWorld(void);
void R_FreeBindlessTexturesForWorld(void);

water_vbo_t *R_CreateWaterVBO(msurface_t *surf, int direction, wsurf_vbo_leaf_t *leaf);
void R_DrawWaters(wsurf_vbo_leaf_t *vboleaf, cl_entity_t *ent);

#define WSURF_DIFFUSE_ENABLED				0x1ull
#define WSURF_LIGHTMAP_ENABLED				0x2ull
#define WSURF_REPLACETEXTURE_ENABLED		0x4ull
#define WSURF_DETAILTEXTURE_ENABLED			0x8ull
#define WSURF_NORMALTEXTURE_ENABLED			0x10ull
#define WSURF_PARALLAXTEXTURE_ENABLED		0x20ull
#define WSURF_SPECULARTEXTURE_ENABLED		0x40ull
#define WSURF_LINEAR_FOG_ENABLED			0x80ull
#define WSURF_EXP_FOG_ENABLED				0x100ull
#define WSURF_EXP2_FOG_ENABLED				0x200ull
#define WSURF_GBUFFER_ENABLED				0x400ull
#define WSURF_SHADOW_CASTER_ENABLED			0x1000ull
#define WSURF_SHADOWMAP_ENABLED				0x2000ull
#define WSURF_SHADOWMAP_HIGH_ENABLED		0x4000ull
#define WSURF_SHADOWMAP_MEDIUM_ENABLED		0x8000ull
#define WSURF_SHADOWMAP_LOW_ENABLED			0x10000ull
#define WSURF_BINDLESS_ENABLED				0x20000ull
#define WSURF_SKYBOX_ENABLED				0x40000ull
#define WSURF_DECAL_ENABLED					0x80000ull
#define WSURF_CLIP_ENABLED					0x100000ull
#define WSURF_CLIP_WATER_ENABLED			0x200000ull
#define WSURF_ALPHA_BLEND_ENABLED			0x400000ull
#define WSURF_ADDITIVE_BLEND_ENABLED		0x800000ull
#define WSURF_OIT_BLEND_ENABLED				0x1000000ull
#define WSURF_GAMMA_BLEND_ENABLED			0x2000000ull
#define WSURF_FULLBRIGHT_ENABLED			0x4000000ull
#define WSURF_COLOR_FILTER_ENABLED			0x8000000ull
#define WSURF_LIGHTMAP_INDEX_0_ENABLED		0x10000000ull
#define WSURF_LIGHTMAP_INDEX_1_ENABLED		0x20000000ull
#define WSURF_LIGHTMAP_INDEX_2_ENABLED		0x40000000ull
#define WSURF_LIGHTMAP_INDEX_3_ENABLED		0x80000000ull
#define WSURF_LEGACY_DLIGHT_ENABLED			0x100000000ull
#define WSURF_ALPHA_SOLID_ENABLED			0x200000000ull