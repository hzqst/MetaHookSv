#pragma once

#include <vector>
#include <memory>

#define MAX_NUM_NODES 16

#define BINDING_POINT_SCENE_UBO 0
#define BINDING_POINT_CAMERA_UBO 1
#define BINDING_POINT_DLIGHT_UBO 2

#define BINDING_POINT_SKYBOX_SSBO 3
#define BINDING_POINT_DECAL_SSBO 3
#define BINDING_POINT_TEXTURE_SSBO 3

#define BINDING_POINT_ENTITY_UBO 4
#define BINDING_POINT_STUDIO_UBO 4

#define BINDING_POINT_OIT_FRAGMENT_SSBO 5
#define BINDING_POINT_OIT_NUMFRAGMENT_SSBO 6
#define BINDING_POINT_OIT_COUNTER_SSBO 7

#define WSURF_VERTEX_ATTRIBUTE_INDEX_POSITION 0
#define WSURF_VERTEX_ATTRIBUTE_INDEX_NORMAL 1
#define WSURF_VERTEX_ATTRIBUTE_INDEX_S_TANGENT 2
#define WSURF_VERTEX_ATTRIBUTE_INDEX_T_TANGENT 3
#define WSURF_VERTEX_ATTRIBUTE_INDEX_TEXCOORD 4
#define WSURF_VERTEX_ATTRIBUTE_INDEX_LIGHTMAP_TEXCOORD 5
#define WSURF_VERTEX_ATTRIBUTE_INDEX_REPLACETEXTURE_TEXCOORD 6
#define WSURF_VERTEX_ATTRIBUTE_INDEX_DETAILTEXTURE_TEXCOORD 7
#define WSURF_VERTEX_ATTRIBUTE_INDEX_NORMALTEXTURE_TEXCOORD 8
#define WSURF_VERTEX_ATTRIBUTE_INDEX_PARALLAXTEXTURE_TEXCOORD 9
#define WSURF_VERTEX_ATTRIBUTE_INDEX_SPECULARTEXTURE_TEXCOORD 10
#define WSURF_VERTEX_ATTRIBUTE_INDEX_STYLES 11

#define WSURF_DIFFUSE_TEXTURE		0
#define WSURF_REPLACE_TEXTURE		1
#define WSURF_DETAIL_TEXTURE		2
#define WSURF_NORMAL_TEXTURE		3
#define WSURF_PARALLAX_TEXTURE		4
#define WSURF_SPECULAR_TEXTURE		5
#define WSURF_MAX_TEXTURE			6

/*

layout(binding = 0) uniform sampler2D diffuseTex;
layout(binding = 1) uniform sampler2D detailTex;
layout(binding = 2) uniform sampler2D normalTex;
layout(binding = 3) uniform sampler2D parallaxTex;
layout(binding = 4) uniform sampler2D specularTex;
layout(binding = 5) uniform sampler2DArray shadowmapTexArray;
layout(binding = 6) uniform sampler2DArray lightmapTexArray;

*/

#define WSURF_BIND_DIFFUSE_TEXTURE 0
#define WSURF_BIND_DETAIL_TEXTURE 1
#define WSURF_BIND_NORMAL_TEXTURE 2
#define WSURF_BIND_PARALLAX_TEXTURE 3
#define WSURF_BIND_SPECULAR_TEXTURE 4
#define WSURF_BIND_SHADOWMAP_TEXTURE 5
#define WSURF_BIND_LIGHTMAP_TEXTURE_0 6
#define WSURF_BIND_LIGHTMAP_TEXTURE_1 7
#define WSURF_BIND_LIGHTMAP_TEXTURE_2 8
#define WSURF_BIND_LIGHTMAP_TEXTURE_3 9

#define WSURF_TEXCHAIN_LIST_STATIC		0
#define WSURF_TEXCHAIN_LIST_ANIM		1
#define WSURF_TEXCHAIN_LIST_MAX			2

#define WSURF_TEXCHAIN_SPECIAL_SKY				0
#define WSURF_TEXCHAIN_SPECIAL_SOLID			1
#define WSURF_TEXCHAIN_SPECIAL_SOLID_WITH_SKY	2
#define WSURF_TEXCHAIN_SPECIAL_MAX				3

#define WSURF_VBO_POSITION		0
#define WSURF_VBO_DIFFUSE		1
#define WSURF_VBO_LIGHTMAP		2
#define WSURF_VBO_NORMAL		3
#define WSURF_VBO_DETAIL		4
#define WSURF_VBO_MAX			5

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

//GPU Resource
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

//GPU Resource

typedef struct brushvertexpos_s
{
	vec3_t	pos;
	vec3_t	normal;
}brushvertexpos_t;

typedef struct brushvertexdiffuse_s
{
	float	texcoord[3];//texcoord[2]=1.0f/texwidth, for SURF_DRAWTILED
}brushvertexdiffuse_t;

typedef struct brushvertexlightmap_s
{
	float	lightmaptexcoord[3]; //lightmaptexcoord[2]=lightmaptexnum
	byte	styles[4];
}brushvertexlightmap_t;

typedef struct brushvertexnormal_s
{
	vec3_t	s_tangent;
	vec3_t	t_tangent;
	float	normaltexcoord[2];
}brushvertexnormal_t;

typedef struct brushvertexdetail_s
{
	float	replacetexcoord[2];
	float	detailtexcoord[2];
	float	normaltexcoord[2];
	float	parallaxtexcoord[2];
	float	speculartexcoord[2];
}brushvertexdetail_t;

//CPU Resource that is for generating EBO
class CWorldSurfaceBrushFace
{
public:
	int index{};
	int flags{};

	vec3_t	normal{};
	vec3_t	s_tangent{};
	vec3_t	t_tangent{};

	uint32_t poly_count{};
	uint32_t start_index{};
	uint32_t index_count{};
	uint32_t reverse_start_index{};
	uint32_t reverse_index_count{};
	float totalSquare{};
};

//CPU Resource

#define TEXCHAIN_STATIC 1
#define TEXCHAIN_SCROLL 2
#define TEXCHAIN_SKY 3

class CWorldSurfaceBrushTexChain
{
public:
	int type{};
	texture_t* texture{};
	uint32_t startDrawOffset{};
	uint32_t drawCount{};
	uint32_t polyCount{};

	detail_texture_cache_t* detailTextureCache{};
};

class CWorldSurfaceModel;

class CWorldSurfaceLeaf
{
public:
	~CWorldSurfaceLeaf();

	GLuint hABO{};
	std::vector<CWorldSurfaceBrushTexChain> vTextureChainList[WSURF_TEXCHAIN_LIST_MAX];
	std::vector<CWaterSurfaceModel *> vWaterSurfaceModels;
	CWorldSurfaceBrushTexChain TextureChainSpecial[WSURF_TEXCHAIN_SPECIAL_MAX];
	CWorldSurfaceModel* pModel{};
};

class CWorldSurfaceWorldModel
{
public:
	~CWorldSurfaceWorldModel();

	std::unordered_map<int, GLuint > VAOMap;
	GLuint hVBO[WSURF_VBO_MAX]{};
	GLuint hEBO{};
	model_t* mod{};
	std::vector<CWorldSurfaceBrushFace> vFaceBuffer;
};

class CWorldSurfaceModel
{
public:
	~CWorldSurfaceModel();

	model_t* mod{};
	CWorldSurfaceWorldModel* pWorldModel{};
	std::vector<CWorldSurfaceLeaf *> vLeaves;
};

#pragma pack(push, 16)

typedef struct camera_ubo_s
{
	mat4 viewMatrix{};
	mat4 projMatrix{};
	mat4 invViewMatrix{};
	mat4 invProjMatrix{};
	vec4_t viewport{};
	vec4_t frustum[4]{};
	vec4_t viewpos{};
	vec4_t vpn{};
	vec4_t vright{};
	vec4_t vup{};
	vec4_t r_origin{};
}camera_ubo_t;

//viewport.z=linkListSize
typedef struct scene_ubo_s
{
	mat4 shadowMatrix[3];
	vec4 shadowDirection;
	vec4 shadowColor;
	vec4 shadowFade;
	vec4 clipPlane;
	vec4 fogColor;
	float fogStart;
	float fogEnd;
	float fogDensity;
	float cl_time;
	float r_g;
	float r_g3;
	float v_brightness;
	float v_lightgamma;
	float v_lambert;
	float v_gamma;
	float v_texgamma;
	float z_near;
	float z_far;
	float r_alphamin;
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
	vec4 r_elight_origin_radius[4];
	mat3x4 bonematrix[128];
	uvec4 r_clipbone;
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
	GLuint IndiceCount[MAX_DECALS];
	int BatchCount;
}decal_drawbatch_t;

//for decal drawing
typedef struct cached_decal_s
{
	vec3_t origin{};
	GLuint gltexturenum{};
	GLuint gltexturewidth{};
	GLuint gltextureheight{};
	detail_texture_cache_t* pDetailTextures{};
	GLint startIndex{};
	GLuint indiceCount{};
}cached_decal_t;

class CWorldSurfaceRenderer
{
public:
	GLuint				hSceneUBO{};
	GLuint				hCameraUBO{};
	GLuint				hDLightUBO{};
	GLuint				hEntityUBO{};
	GLuint				hDecalVBO{};
	GLuint				hDecalEBO{};
	GLuint				hDecalVAO{};
	GLuint				hDecalSSBO{};
	GLuint				hSkyboxSSBO{};
	GLuint				hDetailSkyboxSSBO{};
	GLuint				hWorldSSBO{};
	GLuint				hOITFragmentSSBO{};
	GLuint				hOITNumFragmentSSBO{};
	GLuint				hOITAtomicSSBO{};

	bool				bDiffuseTexture{};
	bool				bLightmapTexture{};
	bool				bShadowmapTexture{};

	int					iLightmapUsedBits{};
	int					iNumLegacyDLights{};

	int					iNumLightmapTextures{};
	int					iLightmapTextureArray[MAXLIGHTMAPS]{};

	int					vSkyboxTextureId[12]{};

	cached_decal_t		vCachedDecals[MAX_DECALS]{};

	std::vector<model_t *> vWorldModels;
};

typedef struct
{
	int program;
	int u_parallaxScale;
}wsurf_program_t;

#define OFFSET(type, variable) ((const void*)&(((type*)NULL)->variable))

extern CWorldSurfaceRenderer g_WorldSurfaceRenderer;
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
void R_FreeWorldResources(void);
void R_LoadWorldResources(void);
void R_FreeUnreferencedWorldSurfaceModels(void);
void R_FreeWorldSurfaceModels(model_t* mod);
void R_FreeWorldSurfaceWorldModels(model_t* mod);
void R_ClearWorldSurfaceModels(void);
void R_ClearWorldSurfaceWorldModels(void);

//engine
extern byte *lightmaps;
extern int *lightmap_textures;
extern void *lightmap_rectchange;
extern int *lightmap_modified;
extern glpoly_t **lightmap_polys;
extern int *d_lightstylevalue;
extern dlight_t *cl_dlights;
extern dlight_t* cl_elights;
extern int *r_dlightactive;
extern int *gDecalSurfCount;
extern msurface_t **gDecalSurfs;
extern decal_t *gDecalPool;
extern decalcache_t *gDecalCache;

//cvar

extern cvar_t *r_wsurf_parallax_scale;
extern cvar_t *r_wsurf_sky_fog;
extern cvar_t *r_wsurf_zprepass;

mleaf_t* R_GetWorldLeafByIndex(model_t* mod, int index);
int R_GetWorldLeafIndex(model_t* mod, mleaf_t* leaf);
int R_GetWorldSurfaceIndex(model_t* mod, msurface_t* surf);
msurface_t* R_GetWorldSurfaceByIndex(model_t* mod, int index);
model_t* R_FindWorldModelBySurface(msurface_t* psurf);
model_t* R_FindWorldModelByNode(mnode_t* pnode);
model_t* R_FindWorldModelByModel(model_t* m);

class bspentity_t;

typedef struct epair_s epair_t;

void R_ClearBSPEntities();
void R_ParseBSPEntities(const char *data, std::vector<bspentity_t*>& vBSPEntities);
const char *ValueForKey(bspentity_t *ent, const char *key);
const char* ValueForKeyEx(bspentity_t* ent, const char* key, epair_t** ppLastEPair);
void ValueForKeyExArray(bspentity_t* ent, const char* key, std::vector<const char*> &strArray);
void R_LoadBSPEntities(std::vector<bspentity_t*>& vBSPEntities);
void R_LoadExternalEntities(std::vector<bspentity_t*>& vBSPEntities);
void R_FreeLightmapTextures(void);
void R_LoadBaseDecalTextures(void);
void R_LoadBaseDetailTextures(void);
void R_LoadMapDetailTextures(void);

void R_AddDynamicLights(msurface_t *surf);
void R_RenderDynamicLightmaps(msurface_t *fa);

void R_DrawDecals(cl_entity_t *ent);
void R_PrepareDecals(void);

CWorldSurfaceWorldModel* R_GetWorldSurfaceWorldModel(model_t* mod);
CWorldSurfaceModel* R_GetWorldSurfaceModel(model_t* mod);

detail_texture_cache_t *R_FindDecalTextureCache(const std::string &decalname);
detail_texture_cache_t *R_FindDetailTextureCache(int texId);
void R_BeginDetailTextureByGLTextureId(int gltexturenum, program_state_t *WSurfProgramState);
void R_BeginDetailTextureByDetailTextureCache(detail_texture_cache_t *cache, program_state_t *WSurfProgramState);
void R_EndDetailTexture(program_state_t WSurfProgramState);
void R_ShutdownWSurf(void);
void R_GenerateSceneUBO(void);
void R_SaveWSurfProgramStates(void);
void R_LoadWSurfProgramStates(void);
void R_UseWSurfProgram(program_state_t state, wsurf_program_t *progOut);

CWaterSurfaceModel* R_GetWaterSurfaceModel(model_t* mod, msurface_t *surf, int direction, CWorldSurfaceWorldModel* pWorldModel, CWorldSurfaceLeaf *pLeaf);

void R_DrawWaterSurfaceModel(
	CWorldSurfaceModel* pModel,
	CWorldSurfaceLeaf* pLeaf,
	CWaterSurfaceModel* pWaterModel,
	water_reflect_cache_t* pReflectCache,
	cl_entity_t* ent);

GLuint R_BindVAOForWorldSurfaceWorldModel(CWorldSurfaceWorldModel* pWorldModel, int VBOStates);

void R_PolygonToTriangleList(const std::vector<vertex3f_t>& vPolyVertices, std::vector<uint32_t>& vOutIndiceBuffer);

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