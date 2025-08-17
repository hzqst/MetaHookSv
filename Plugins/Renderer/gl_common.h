#pragma once

#include "qgl.h"

typedef struct FBO_Container_s
{
	GLuint s_hBackBufferFBO;
	GLuint s_hBackBufferCB;
	GLuint s_hBackBufferDB;
	GLuint s_hBackBufferTex;
	GLuint s_hBackBufferTex2;
	GLuint s_hBackBufferTex3;
	GLuint s_hBackBufferTex4;
	GLuint s_hBackBufferDepthTex;
	GLuint s_hBackBufferStencilView;
	int iWidth;
	int iHeight;
	int iTextureColorFormat;
	int iTextureDepthFormat;
}FBO_Container_t;

class CDrawArrayAttrib
{
public:
	uint32_t NumVertices{};
	uint32_t NumInstances{ 1 };
	uint32_t StartVertexLocation{};
	uint32_t FirstInstanceLocation{};
};

class CDrawIndexAttrib
{
public:
	uint32_t NumIndices{};
	uint32_t NumInstances{ 1 };
	uint32_t FirstIndexLocation{};
	uint32_t BaseVertex{};
	uint32_t FirstInstanceLocation{};
};

class CIndirectDrawAttrib
{
public:
	uint64_t DrawArgsOffset{};
	uint32_t DrawCount{ 1 };
};

#define TRIAPI_VA_POSITION		0
#define TRIAPI_VA_TEXCOORD		1
#define TRIAPI_VA_COLOR			2

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

#define WSURF_VA_POSITION 0
#define WSURF_VA_NORMAL 1
#define WSURF_VA_S_TANGENT 2
#define WSURF_VA_T_TANGENT 3
#define WSURF_VA_TEXCOORD 4
#define WSURF_VA_LIGHTMAP_TEXCOORD 5
#define WSURF_VA_REPLACETEXTURE_TEXCOORD 6
#define WSURF_VA_DETAILTEXTURE_TEXCOORD 7
#define WSURF_VA_NORMALTEXTURE_TEXCOORD 8
#define WSURF_VA_PARALLAXTEXTURE_TEXCOORD 9
#define WSURF_VA_SPECULARTEXTURE_TEXCOORD 10
#define WSURF_VA_STYLES 11

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

#define STUDIO_BIND_TEXTURE_DIFFUSE				0
#define STUDIO_BIND_TEXTURE_NORMAL				2
#define STUDIO_BIND_TEXTURE_PARALLAX			3
#define STUDIO_BIND_TEXTURE_SPECULAR			4
#define STUDIO_BIND_TEXTURE_STENCIL				5
#define STUDIO_BIND_TEXTURE_ANIMATED			6
#define STUDIO_BIND_TEXTURE_SHADOW_DIFFUSE		7
#define STUDIO_BIND_TEXTURE_VIEW_MODEL_STENCIL	8

#define STENCIL_MASK_ALL						0xFF
#define STENCIL_MASK_NONE						0
#define STENCIL_MASK_WORLD						1
#define STENCIL_MASK_VIEW_MODEL					1
#define STENCIL_MASK_NO_SHADOW					2
#define STENCIL_MASK_NO_BLOOM					4
#define STENCIL_MASK_HAS_FLATSHADE				0x8
#define STENCIL_MASK_HAS_OUTLINE				0x10//temp mask
#define STENCIL_MASK_HAS_DECAL					0x20//temp mask
#define STENCIL_MASK_HAS_SHADOW					0x40//temp mask
#define STENCIL_MASK_HAS_FACE					0x80//temp mask

#define STENCIL_MASK_HAS_FOG					STENCIL_MASK_WORLD

#define WATER_BIND_BASE_TEXTURE				0
#define WATER_BIND_NORMAL_TEXTURE			1
#define WATER_BIND_REFRACT_TEXTURE			2
#define WATER_BIND_REFRACT_DEPTH_TEXTURE	3
#define WATER_BIND_REFLECT_TEXTURE			4
#define WATER_BIND_REFLECT_DEPTH_TEXTURE	5

#define DSHADE_BIND_DIFFUSE_TEXTURE			0
#define DSHADE_BIND_LIGHTMAP_TEXTURE		1
#define DSHADE_BIND_WORLDNORM_TEXTURE		2
#define DSHADE_BIND_SPECULAR_TEXTURE		3
#define DSHADE_BIND_DEPTH_TEXTURE			4
#define DSHADE_BIND_STENCIL_TEXTURE			5
#define DSHADE_BIND_CONE_TEXTURE			6
#define DSHADE_BIND_SHADOWMAP_TEXTURE		7

#define DFINAL_BIND_DIFFUSE_TEXTURE			0
#define DFINAL_BIND_LIGHTMAP_TEXTURE		1
#define DFINAL_BIND_WORLDNORM_TEXTURE		2
#define DFINAL_BIND_SPECULAR_TEXTURE		3
#define DFINAL_BIND_DEPTH_TEXTURE			4
#define DFINAL_BIND_STENCIL_TEXTURE			5

typedef struct vertex3f_s
{
	vec3_t	v;
}vertex3f_t;

typedef struct triapivertex_s
{
	vec3_t	pos;
	vec2_t	texcoord;
	vec4_t	color;
}triapivertex_t;

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
	float r_linear_blend_shift;
	float r_lightscale;
	vec4 r_filtercolor;
	vec4 r_lightstylevalue[256 / 4];
	float r_linear_fog_shift;
	float r_linear_fog_shiftz;
	float padding2;
	float padding3;
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

class CGameResourceAsyncLoadTask : public IBaseInterface
{
public:
	ThreadWorkItemHandle_t m_hThreadWorkItem{};
	std::atomic<bool> m_IsDataReady{};

	virtual void StartAsyncTask() {};
	virtual void UploadResource() {};
};
