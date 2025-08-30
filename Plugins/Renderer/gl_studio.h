#pragma once

#include "gl_cvar.h"
#include "gl_draw.h"
#include <vector>
#include <set>
#include <unordered_map>
#include <memory>

#define MAX_STUDIO_BONE_CACHES 1024

#define STUDIO_DIFFUSE_TEXTURE			0
#define STUDIO_REPLACE_TEXTURE			1
#define STUDIO_NORMAL_TEXTURE			2
#define STUDIO_PARALLAX_TEXTURE			3
#define STUDIO_SPECULAR_TEXTURE			4
#define STUDIO_MAX_TEXTURE				5

#define STUDIO_VBO_BASE		0
#define STUDIO_VBO_TBN		1
#define STUDIO_VBO_MAX		2

#define STUDIO_VA_POSITION		0
#define STUDIO_VA_NORMAL		1
#define STUDIO_VA_TEXCOORD		2
#define STUDIO_VA_PACKEDBONE	3
#define STUDIO_VA_TANGENT		4
#define STUDIO_VA_BITANGENT		5
#define STUDIO_VA_SMOOTHNORMAL	6

typedef struct
{
	int program;
	int r_base_specular;
	int r_celshade_specular;
	int r_celshade_midpoint;
	int r_celshade_softness;
	int r_celshade_shadow_color;
	int r_celshade_head_offset;
	int r_celshade_lightdir_adjust;
	int r_rimlight_power;
	int r_rimlight_smooth;
	int r_rimlight_smooth2;
	int r_rimlight_color;
	int r_rimdark_power;
	int r_rimdark_smooth;
	int r_rimdark_smooth2;
	int r_rimdark_color;
	int r_outline_dark;
	int r_hair_specular_exp;
	int r_hair_specular_noise;
	int r_hair_specular_intensity;
	int r_hair_specular_exp2;
	int r_hair_specular_noise2;
	int r_hair_specular_intensity2;
	int r_hair_specular_smooth;
	int r_hair_shadow_offset;
	int r_uvscale;
	int r_packed_stride;
	int r_packed_index;
	int r_framerate_numframes;
}studio_program_t;

class studiovertexbase_t
{
public:
	studiovertexbase_t()
	{

	}

	vec3_t	pos{};
	vec3_t	normal{};
	vec2_t	texcoord{};
	byte	packedbone[4]{};
};

class studiovertextbn_t
{
public:
	studiovertextbn_t()
	{

	}

	vec3_t	tangent{};
	vec3_t	bitangent{};
	vec3_t	smoothnormal{};
};

class CStudioModelRenderMesh
{
public:
	int iMeshIndex{-1};
	int iStartIndex{ -1 };
	int iIndiceCount{ 0 };
	int iPolyCount{ 0 };
	size_t nMeshOffset{ 0 };
};

class CStudioModelRenderSubModel
{
public:
	CStudioModelRenderSubModel(studiohdr_t* studiohdr, mstudiomodel_t* submodel) : m_SubmodelOffset((byte*)submodel - (byte*)studiohdr)
	{

	}
	
	size_t m_SubmodelOffset{};
	std::vector<CStudioModelRenderMesh> vMesh;
};

class CStudioModelRenderTexture
{
public:
	int gltexturenum{};
	int numframes{};
	float framerate{};
	int width{}, height{};
	float scaleX{}, scaleY{};
};

class CStudioModelRenderMaterial
{
public:
	CStudioModelRenderTexture textures[STUDIO_MAX_TEXTURE - STUDIO_DIFFUSE_TEXTURE];
};

class CStudioCelshadeControl
{
public:
	StudioConVar base_specular;
	StudioConVar celshade_specular;
	StudioConVar celshade_midpoint;
	StudioConVar celshade_softness;
	StudioConVar celshade_shadow_color;
	StudioConVar celshade_head_offset;
	StudioConVar celshade_lightdir_adjust;
	StudioConVar outline_size;
	StudioConVar outline_dark;
	StudioConVar rimlight_power;
	StudioConVar rimlight_smooth;
	StudioConVar rimlight_smooth2;
	StudioConVar rimlight_color;
	StudioConVar rimdark_power;
	StudioConVar rimdark_smooth;
	StudioConVar rimdark_smooth2;
	StudioConVar rimdark_color;
	StudioConVar hair_specular_exp;
	StudioConVar hair_specular_intensity;
	StudioConVar hair_specular_noise;
	StudioConVar hair_specular_exp2;
	StudioConVar hair_specular_intensity2;
	StudioConVar hair_specular_noise2;
	StudioConVar hair_specular_smooth;
	StudioConVar hair_shadow_offset;
};

class CStudioLowerBodyControl
{
public:
	StudioConVar model_scale;
	StudioConVar model_origin;
	StudioConVar duck_model_origin;
};

class CStudioModelRenderData
{
public:
	CStudioModelRenderData(model_t* mod) : BodyModel(mod)
	{

	}

	~CStudioModelRenderData();

	void AsyncUploadResouce()
	{
		if (m_pGameResourceAsyncLoadTask)
		{
			m_pGameResourceAsyncLoadTask->UploadResource();
			m_pGameResourceAsyncLoadTask.reset();
		}
	}

	void ReleaseAsyncLoadTask()
	{
		if (m_pGameResourceAsyncLoadTask)
		{
			if (m_pGameResourceAsyncLoadTask->m_hThreadWorkItem)
			{
				g_pMetaHookAPI->WaitForWorkItemToComplete(m_pGameResourceAsyncLoadTask->m_hThreadWorkItem);
				g_pMetaHookAPI->DeleteWorkItem(m_pGameResourceAsyncLoadTask->m_hThreadWorkItem);
				m_pGameResourceAsyncLoadTask->m_hThreadWorkItem = nullptr;
			}
			m_pGameResourceAsyncLoadTask.reset();
		}
	}

	GLuint				hVBO[STUDIO_VBO_MAX]{};
	GLuint				hEBO{};
	GLuint				hVAO{};

	//CStudioModelRenderSubModel Storage
	std::vector<CStudioModelRenderSubModel *> vSubmodels;

	//Memory Offset -> CStudioModelRenderSubModel Mapping Table
	std::unordered_map<int, CStudioModelRenderSubModel*> mSubmodels;

	//Material Storage
	std::unordered_map<uint32_t, std::shared_ptr<CStudioModelRenderMaterial>> mStudioMaterials;

	model_t* BodyModel{};
	model_t* TextureModel{};

	CStudioCelshadeControl CelshadeControl;

	CStudioLowerBodyControl LowerBodyControl;

	std::atomic_bool bIsClosing{ false };

	std::shared_ptr<CGameResourceAsyncLoadTask> m_pGameResourceAsyncLoadTask;
};

class CStudioSkinCache
{
public:
	skin_t skins[MAX_SKINS];
};

class CStudioBoneCacheHandle
{
public:
	CStudioBoneCacheHandle(int modelindex, int sequence, int gaitsequence, float frame, const float *origin, const float* angles)
	{
		m_modelindex = modelindex;
		m_sequence = sequence;
		m_gaitsequence = gaitsequence;
		m_frame = frame;
		VectorCopy(origin, m_origin);
		VectorCopy(angles, m_angles);
	}

	bool operator == (const CStudioBoneCacheHandle& a) const
	{
		return
			m_modelindex == a.m_modelindex &&
			m_sequence == a.m_sequence &&
			m_gaitsequence == a.m_gaitsequence &&
			m_frame == a.m_frame &&
			VectorCompare(m_origin, a.m_origin) &&
			VectorCompare(m_angles, a.m_angles);
	}

	int m_modelindex;
	int m_sequence;
	int m_gaitsequence;
	float m_frame;
	vec3_t m_origin;
	vec3_t m_angles;
};

class CStudioBoneCacheHasher
{
public:
	std::size_t operator()(const CStudioBoneCacheHandle& key) const
	{
		auto base = (std::size_t)(key.m_modelindex << 24);

		base += ((std::size_t)key.m_sequence << 16);
		base += ((std::size_t)key.m_gaitsequence << 8);
		base += (std::size_t)(key.m_frame * 128.0);
		base += (std::size_t)(key.m_origin[0] * 128.0);
		base += (std::size_t)(key.m_origin[1] * 128.0);
		base += (std::size_t)(key.m_origin[2] * 128.0);
		base += (std::size_t)(key.m_angles[0] * 128.0);
		base += (std::size_t)(key.m_angles[1] * 128.0);
		base += (std::size_t)(key.m_angles[2] * 128.0);

		return base;
	}
};

class CStudioBoneCache
{
public:
	CStudioBoneCache()
	{
		memset(m_bonetransform, 0, sizeof(m_bonetransform));
		memset(m_lighttransform, 0, sizeof(m_lighttransform));
		m_next = NULL;
	}
	CStudioBoneCache(float* _bonetransform, float* _lighttransform)
	{
		memcpy(m_bonetransform, _bonetransform, sizeof(m_bonetransform));
		memcpy(m_lighttransform, _lighttransform, sizeof(m_lighttransform));
		m_next = NULL;
	}

	float m_bonetransform[MAXSTUDIOBONES][3][4];
	float m_lighttransform[MAXSTUDIOBONES][3][4];
	CStudioBoneCache* m_next;
};

class CStudioSetupSkinContext
{
public:
	CStudioSetupSkinContext(program_state_t* State)
	{
		StudioProgramState = State;
		packedDiffuseIndex = -1;
		packedNormalIndex = -1;
		packedParallaxIndex = -1;
		packedSpecularIndex = -1;
		packedCount = 0;
		packedStride = 0;
		width = 0; 
		height = 0;
		s = 0;
		t = 0;
		framerate = 0;
		numframes = 0;
	}

	program_state_t* StudioProgramState;
	int packedDiffuseIndex;
	int packedNormalIndex;
	int packedParallaxIndex;
	int packedSpecularIndex;
	int packedCount;
	float packedStride;
	float width, height;
	float s, t;
	float framerate;
	float numframes;
};

//engine
extern mstudiomodel_t **psubmodel;
extern mstudiobodyparts_t **pbodypart;
extern studiohdr_t **pstudiohdr;
extern model_t **r_model;
extern float *r_blend;
extern auxvert_t **pauxverts;
extern float **pvlightvalues;
extern auxvert_t(*auxverts)[MAXSTUDIOVERTS];
extern vec3_t(*lightvalues)[MAXSTUDIOVERTS];
extern float (*pbonetransform)[MAXSTUDIOBONES][3][4];
extern float (*plighttransform)[MAXSTUDIOBONES][3][4];
extern float(*rotationmatrix)[3][4];
extern int (*g_NormalIndex)[MAXSTUDIOVERTS];
extern int (*chromeage)[MAXSTUDIOBONES];
extern int(*chrome)[MAXSTUDIOVERTS][2];
extern cl_entity_t *cl_viewent;
extern int *g_ForcedFaceFlags;
extern int *lightgammatable;
extern byte *texgammatable;
extern float *g_ChromeOrigin;
extern int *r_ambientlight;
extern float *r_shadelight;
extern vec3_t *r_blightvec;
extern vec3_t *r_plightvec;
extern float *r_colormix;
extern int *r_smodels_total;
extern int *r_amodels_drawn;
extern dlight_t *(*locallight)[3];
extern int *numlights;
extern int* r_topcolor;
extern int* r_bottomcolor;
#if 0
extern player_model_t(*DM_PlayerState)[MAX_CLIENTS];
extern skin_t(*DM_RemapSkin)[64][MAX_SKINS];
extern skin_t* (*pDM_RemapSkin)[2528][MAX_SKINS];
extern int* r_remapindex;
#endif
extern model_t *cl_sprite_white;
extern model_t *cl_sprite_shell;

//renderer
extern int r_studio_drawcall;
extern int r_studio_polys;

extern MapConVar* r_studio_base_specular;
extern MapConVar* r_studio_celshade_specular;

std::shared_ptr<CStudioModelRenderData> R_CreateStudioRenderData(model_t* mod, studiohdr_t* studiohdr);
std::shared_ptr<CStudioModelRenderData> R_GetStudioRenderDataFromModel(model_t* mod);
void R_StudioClearVanillaBonesCaches();
void R_StudioClearAllBoneCaches();
void R_StudioSaveBoneCache(studiohdr_t* studiohdr, int modelindex, int sequence, int gaitsequence, float frame, const float* origin, const float* angles);
bool R_StudioLoadBoneCache(studiohdr_t* studiohdr, int modelindex, int sequence, int gaitsequence, float frame, const float* origin, const float* angles);
void R_FreeStudioRenderData(model_t* mod);
void R_FreeUnreferencedStudioRenderData(void);
void R_StudioFlushAllSkins();
void R_ShutdownStudio(void);
void R_InitStudio(void);
void R_StudioStartFrame(void);
void R_StudioEndFrame(void);
void R_SaveStudioProgramStates(void);
void R_LoadStudioProgramStates(void);
void R_GLStudioDrawPoints(void);
void R_StudioLoadTextureModel(model_t* mod, studiohdr_t *studiohdr, CStudioModelRenderData* pRenderData);

void studioapi_StudioDynamicLight(cl_entity_t *ent, alight_t *plight);
qboolean studioapi_StudioCheckBBox(void);
void studioapi_RestoreRenderer(void);
void UpdatePlayerPitch(cl_entity_t* ent, float a2);

int __fastcall GameStudioRenderer_StudioDrawPlayer(void* pthis, int dummy, int flags, struct entity_state_s* pplayer);
void __fastcall GameStudioRenderer_StudioRenderModel(void *pthis, int);
void __fastcall GameStudioRenderer_StudioRenderFinal(void *pthis, int);
void __fastcall GameStudioRenderer_StudioSetupBones(void *pthis, int);
void __fastcall GameStudioRenderer_StudioSaveBones(void* pthis, int);
void __fastcall GameStudioRenderer_StudioMergeBones(void *pthis, int, model_t *pSubModel);

int R_StudioDrawPlayer(int flags, struct entity_state_s* pplayer);
void R_StudioRenderModel(void);
void R_StudioRenderFinal(void);
void R_StudioSetupBones(void);
void R_StudioMergeBones(model_t* pSubModel);
void R_StudioSaveBones(void);

extern engine_studio_api_t IEngineStudio;
extern r_studio_interface_t **gpStudioInterface;

#define STUDIO_GBUFFER_ENABLED					0x80000ull
#define STUDIO_LINEAR_FOG_ENABLED				0x100000ull
#define STUDIO_EXP_FOG_ENABLED					0x200000ull
#define STUDIO_EXP2_FOG_ENABLED					0x400000ull
#define STUDIO_LINEAR_FOG_SHIFT_ENABLED			0x800000ull
#define STUDIO_SHADOW_CASTER_ENABLED			0x1000000ull
#define STUDIO_GLOW_SHELL_ENABLED				0x2000000ull
#define STUDIO_OUTLINE_ENABLED					0x4000000ull
#define STUDIO_HAIR_SHADOW_ENABLED				0x8000000ull
#define STUDIO_CLIP_WATER_ENABLED				0x10000000ull
#define STUDIO_CLIP_ENABLED						0x20000000ull
#define STUDIO_ALPHA_BLEND_ENABLED				0x40000000ull
#define STUDIO_ADDITIVE_BLEND_ENABLED			0x80000000ull
#define STUDIO_OIT_BLEND_ENABLED				0x100000000ull
#define STUDIO_GAMMA_BLEND_ENABLED				0x200000000ull
#define STUDIO_ADDITIVE_RENDER_MODE_ENABLED		0x400000000ull
#define STUDIO_NORMALTEXTURE_ENABLED			0x800000000ull
#define STUDIO_PARALLAXTEXTURE_ENABLED			0x1000000000ull
#define STUDIO_SPECULARTEXTURE_ENABLED			0x2000000000ull
#define STUDIO_DEBUG_ENABLED					0x4000000000ull
#define STUDIO_PACKED_DIFFUSETEXTURE_ENABLED	0x8000000000ull
#define STUDIO_PACKED_NORMALTEXTURE_ENABLED		0x10000000000ull
#define STUDIO_PACKED_PARALLAXTEXTURE_ENABLED	0x20000000000ull
#define STUDIO_PACKED_SPECULARTEXTURE_ENABLED	0x40000000000ull
#define STUDIO_ANIMATED_TEXTURE_ENABLED			0x80000000000ull
#define STUDIO_REVERT_NORMAL_ENABLED			0x100000000000ull
#define STUDIO_STENCIL_TEXTURE_ENABLED			0x200000000000ull
#define STUDIO_SHADOW_DIFFUSE_TEXTURE_ENABLED	0x400000000000ull
#define STUDIO_CLIP_BONE_ENABLED				0x800000000000ull
#define STUDIO_LEGACY_DLIGHT_ENABLED			0x1000000000000ull
#define STUDIO_LEGACY_ELIGHT_ENABLED			0x2000000000000ull
#define STUDIO_CLIP_VIEW_MODEL_PIXEL_ENABLED	0x4000000000000ull

#define STUDIO_PACKED_TEXTURE_ALLBITS	(STUDIO_PACKED_DIFFUSETEXTURE_ENABLED | STUDIO_PACKED_NORMALTEXTURE_ENABLED | STUDIO_PACKED_PARALLAXTEXTURE_ENABLED | STUDIO_PACKED_SPECULARTEXTURE_ENABLED)
