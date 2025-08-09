#pragma once

#include <vector>
#include <memory>

#define WSURF_DIFFUSE_TEXTURE		0
#define WSURF_REPLACE_TEXTURE		1
#define WSURF_DETAIL_TEXTURE		2
#define WSURF_NORMAL_TEXTURE		3
#define WSURF_PARALLAX_TEXTURE		4
#define WSURF_SPECULAR_TEXTURE		5
#define WSURF_MAX_TEXTURE			6

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

#define TEXCHAIN_PASS_SOLID 0
#define TEXCHAIN_PASS_SOLID_WITH_SKY 1

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
	std::vector<std::shared_ptr<CWaterSurfaceModel>> m_vWaterSurfaceModels;
	CWorldSurfaceBrushTexChain TextureChainSpecial[WSURF_TEXCHAIN_SPECIAL_MAX];
	std::weak_ptr<CWorldSurfaceModel> m_pModel{};

	std::atomic_bool m_bIsClosing{ false };
	ThreadWorkItemHandle_t m_hThreadWorkItem{};
};

class CWorldSurfaceWorldModel
{
public:
	CWorldSurfaceWorldModel(model_t* mod) : m_model(mod)
	{

	}

	~CWorldSurfaceWorldModel();

	model_t* m_model{};
	GLuint hVBO[WSURF_VBO_MAX]{};
	GLuint hEBO{};
	std::unordered_map<int, GLuint> VAOMap;
	std::vector<CWorldSurfaceBrushFace> m_vFaceBuffer;
};

class CWorldSurfaceModel
{
public:
	CWorldSurfaceModel(model_t* mod) : m_model(mod)
	{

	}

	~CWorldSurfaceModel()
	{

	}

	std::shared_ptr<CWorldSurfaceLeaf> GetLeafByIndex(int index) const
	{
		if (index < 0 || index >= m_vLeaves.size())
			return nullptr;

		return m_vLeaves[index];
	}

	model_t* m_model{};
	std::weak_ptr<CWorldSurfaceWorldModel> m_pWorldModel{};
	std::vector<std::shared_ptr<CWorldSurfaceLeaf>> m_vLeaves;
};

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
int R_FindTextureIdByTexture(model_t* mod, texture_t* ptex);

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

std::shared_ptr<CWorldSurfaceWorldModel> R_GetWorldSurfaceWorldModel(model_t* mod);
std::shared_ptr<CWorldSurfaceModel> R_GetWorldSurfaceModel(model_t* mod);

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

std::shared_ptr<CWaterSurfaceModel> R_GetWaterSurfaceModel(model_t* mod, msurface_t *surf, int direction, CWorldSurfaceWorldModel* pWorldModel, CWorldSurfaceLeaf *pLeaf);

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
#define WSURF_LINEAR_FOG_SHIFT_ENABLED		0x400000000ull