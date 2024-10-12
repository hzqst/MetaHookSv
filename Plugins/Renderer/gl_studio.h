#pragma once

#include "gl_cvar.h"
#include "gl_draw.h"
#include <vector>
#include <set>
#include <unordered_map>

#define MAX_STUDIO_BONE_CACHES 1024

#define STUDIO_DIFFUSE_TEXTURE			0
#define STUDIO_REPLACE_TEXTURE			1
#define STUDIO_NORMAL_TEXTURE			2
#define STUDIO_PARALLAX_TEXTURE			3
#define STUDIO_SPECULAR_TEXTURE			4
#define STUDIO_MAX_TEXTURE				5

#define STUDIO_RESERVED_TEXTURE_STENCIL				6
#define STUDIO_RESERVED_TEXTURE_ANIMATED			7

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

class CStudioModelRenderVertex
{
public:
	CStudioModelRenderVertex(float *a, float *b, float s, float t, int d, int e)
	{
		memcpy(pos, a, sizeof(vec3_t));
		memcpy(normal, b, sizeof(vec3_t));
		texcoord[0] = s;
		texcoord[1] = t;
		vertbone = d;
		normbone = e;
	}
	CStudioModelRenderVertex()
	{
		memset(pos, 0, sizeof(vec3_t));
		memset(normal, 0, sizeof(vec3_t));
		memset(texcoord, 0, sizeof(vec2_t));
		vertbone = 0;
		normbone = 0;
	}
	vec3_t	pos;
	vec3_t	normal;
	vec2_t	texcoord;
	int		vertbone;
	int		normbone;
};

class CStudioModelRenderMesh
{
public:
	int iMeshIndex{-1};
	int iStartIndex{ -1 };
	int iIndiceCount{  };
	int iPolyCount{  };
};

class CStudioModelRenderSubModel
{
public:
	int iSubmodelIndex{-1};
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

typedef struct studio_celshade_control_s
{
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
}studio_celshade_control_t;

class CStudioModelRenderData
{
public:
	~CStudioModelRenderData();

	GLuint				hVBO{};
	GLuint				hEBO{};
	GLuint				hVAO{};
	GLuint				hStudioUBO{};

	//vbo_submodel_t Storage
	std::vector<CStudioModelRenderSubModel *> vSubmodels;

	//Memory Offset -> vbo_submodel_t Mapping Table
	std::unordered_map<int, CStudioModelRenderSubModel*> mSubmodels;

	model_t* BodyModel{};
	model_t* TextureModel{};

	studio_celshade_control_t celshade_control;

	bool bExternalFileLoaded{};
};

#if 0
class studio_skin_handle
{
public:
	bool operator == (const studio_skin_handle& a) const
	{
		return m_keynum == a.m_keynum && m_index == a.m_index;
	}

	int m_keynum{};
	int m_index{};
};

class studio_skin_hasher
{
public:
	std::size_t operator()(const studio_skin_handle& key) const
	{
		auto base = (std::size_t)(key.m_keynum << 8);

		base += ((std::size_t)key.m_index);

		return base;
	}
};
#endif

typedef struct studio_skin_cache_s
{
	skin_t skins[MAX_SKINS];
}studio_skin_cache_t;

class studio_bone_handle
{
public:
	studio_bone_handle(int vboindex, int sequence, int gaitsequence, float frame, vec3_t origin, vec3_t angles)
	{
		m_vboindex = vboindex;
		m_sequence = sequence;
		m_gaitsequence = gaitsequence;
		m_frame = frame;
		VectorCopy(origin, m_origin);
		VectorCopy(angles, m_angles);
	}

	bool operator == (const studio_bone_handle& a) const
	{
		return
			m_vboindex == a.m_vboindex &&
			m_sequence == a.m_sequence &&
			m_gaitsequence == a.m_gaitsequence &&
			m_frame == a.m_frame &&
			VectorCompare(m_origin, a.m_origin) &&
			VectorCompare(m_angles, a.m_angles);
	}

	int m_vboindex;
	int m_sequence;
	int m_gaitsequence;
	float m_frame;
	vec3_t m_origin;
	vec3_t m_angles;
};

class studio_bone_hasher
{
public:
	std::size_t operator()(const studio_bone_handle& key) const
	{
		auto base = (std::size_t)(key.m_vboindex << 24);

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

class studio_bone_cache
{
public:
	studio_bone_cache()
	{
		memset(m_bonetransform, 0, sizeof(m_bonetransform));
		memset(m_lighttransform, 0, sizeof(m_lighttransform));
		m_next = NULL;
	}
	studio_bone_cache(float* _bonetransform, float* _lighttransform)
	{
		memcpy(m_bonetransform, _bonetransform, sizeof(m_bonetransform));
		memcpy(m_lighttransform, _lighttransform, sizeof(m_lighttransform));
		m_next = NULL;
	}

	float m_bonetransform[MAXSTUDIOBONES][3][4];
	float m_lighttransform[MAXSTUDIOBONES][3][4];
	studio_bone_cache* m_next;
};

typedef struct studio_setupskin_context_s
{
public:
	struct studio_setupskin_context_s(program_state_t* State)
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
}studio_setupskin_context_t;

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
extern int *numlight;
extern int* r_topcolor;
extern int* r_bottomcolor;
#if 0
extern player_model_t(*DM_PlayerState)[MAX_CLIENTS];
extern skin_t(*DM_RemapSkin)[64][MAX_SKINS];
extern skin_t* (*pDM_RemapSkin)[2528][MAX_SKINS];
extern int* r_remapindex;
#endif
extern model_t *cl_sprite_white;
extern model_t *cl_shellchrome;

//renderer
extern int r_studio_drawcall;
extern int r_studio_polys;

extern MapConVar* r_studio_base_specular;
extern MapConVar* r_studio_celshade_specular;

void R_StudioBoneCaches_StartFrame();
CStudioModelRenderData* R_AllocateStudioVBO(model_t* mod, studiohdr_t* studiohdr);
void R_StudioLoadExternalFile(model_t *mod, studiohdr_t *studiohdr, CStudioModelRenderData *VBOData);
void R_StudioClearAllBoneCaches();
void R_StudioClearVBOCache(void);
void R_StudioReloadVBOCache(void);
void R_StudioFlushAllSkins();
void R_ShutdownStudio(void);
void R_InitStudio(void);
void R_SaveStudioProgramStates(void);
void R_LoadStudioProgramStates(void);
void R_GLStudioDrawPoints(void);
studiohdr_t* R_StudioGetTextureHeader(CStudioModelRenderData* VBOData);
void R_StudioLoadTextureModel(model_t* mod, studiohdr_t *studiohdr, CStudioModelRenderData* VBOData);
void R_StudioTextureAddReferences(model_t* mod, studiohdr_t* studiohdr, std::set<int>& textures);
void R_StudioFreeTextureCallback(gltexture_t* glt);
CStudioModelRenderMaterial* R_StudioGetVBOMaterialFromTextureId(int gltexturenum);
void studioapi_StudioDynamicLight(cl_entity_t *ent, alight_t *plight);
qboolean studioapi_StudioCheckBBox(void);
void studioapi_RestoreRenderer(void);
void __fastcall GameStudioRenderer_StudioRenderModel(void *pthis, int);
void __fastcall GameStudioRenderer_StudioRenderFinal(void *pthis, int);
void __fastcall GameStudioRenderer_StudioSetupBones(void *pthis, int);
void __fastcall GameStudioRenderer_StudioMergeBones(void *pthis, int, model_t *pSubModel);
void  R_StudioRenderModel(void);
void  R_StudioRenderFinal(void);

extern engine_studio_api_t IEngineStudio;
extern r_studio_interface_t **gpStudioInterface;

#define STUDIO_GBUFFER_ENABLED					0x80000ull
#define STUDIO_LINEAR_FOG_ENABLED				0x100000ull
#define STUDIO_EXP_FOG_ENABLED					0x200000ull
#define STUDIO_EXP2_FOG_ENABLED					0x400000ull
#define STUDIO_SHADOW_CASTER_ENABLED			0x800000ull
#define STUDIO_GLOW_SHELL_ENABLED				0x1000000ull
#define STUDIO_OUTLINE_ENABLED					0x2000000ull
#define STUDIO_HAIR_SHADOW_ENABLED				0x4000000ull
#define STUDIO_CLIP_WATER_ENABLED				0x8000000ull
#define STUDIO_CLIP_ENABLED						0x10000000ull
#define STUDIO_ALPHA_BLEND_ENABLED				0x20000000ull
#define STUDIO_ADDITIVE_BLEND_ENABLED			0x40000000ull
#define STUDIO_OIT_BLEND_ENABLED				0x80000000ull
#define STUDIO_GAMMA_BLEND_ENABLED				0x100000000ull
#define STUDIO_ADDITIVE_RENDER_MODE_ENABLED		0x200000000ull
#define STUDIO_NORMALTEXTURE_ENABLED			0x400000000ull
#define STUDIO_PARALLAXTEXTURE_ENABLED			0x800000000ull
#define STUDIO_SPECULARTEXTURE_ENABLED			0x1000000000ull
#define STUDIO_DEBUG_ENABLED					0x2000000000ull
#define STUDIO_PACKED_DIFFUSETEXTURE_ENABLED	0x4000000000ull
#define STUDIO_PACKED_NORMALTEXTURE_ENABLED		0x8000000000ull
#define STUDIO_PACKED_PARALLAXTEXTURE_ENABLED	0x10000000000ull
#define STUDIO_PACKED_SPECULARTEXTURE_ENABLED	0x20000000000ull
#define STUDIO_ANIMATED_TEXTURE_ENABLED			0x40000000000ull
#define STUDIO_REVERT_NORMAL_ENABLED			0x80000000000ull
#define STUDIO_STENCIL_TEXTURE_ENABLED			0x100000000000ull

#define STUDIO_PACKED_TEXTURE_ALLBITS	(STUDIO_PACKED_DIFFUSETEXTURE_ENABLED | STUDIO_PACKED_NORMALTEXTURE_ENABLED | STUDIO_PACKED_PARALLAXTEXTURE_ENABLED | STUDIO_PACKED_SPECULARTEXTURE_ENABLED)
