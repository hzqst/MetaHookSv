#pragma once

#include "qgl.h"

#define OFFSET(type, variable) ((const void*)&(((type*)NULL)->variable))

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
	char szFrameBufferName[64]{};
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

#define TEXTUREDRECT_VA_POSITION		0
#define TEXTUREDRECT_VA_TEXCOORD		1
#define TEXTUREDRECT_VA_COLOR			2
#define TEXTUREDRECT_VA_MATRIX0			3
#define TEXTUREDRECT_VA_MATRIX1			4
#define TEXTUREDRECT_VA_MATRIX2			5
#define TEXTUREDRECT_VA_MATRIX3			6

#define FILLEDRECT_VA_POSITION			0
#define FILLEDRECT_VA_COLOR				1
#define FILLEDRECT_VA_MATRIX0			2
#define FILLEDRECT_VA_MATRIX1			3
#define FILLEDRECT_VA_MATRIX2			4
#define FILLEDRECT_VA_MATRIX3			5

#define TRIAPI_VA_POSITION		0
#define TRIAPI_VA_TEXCOORD		1
#define TRIAPI_VA_COLOR			2

#define SPRITE_VA_UP_DOWN_LEFT_RIGHT 0
#define SPRITE_VA_COLOR 1
#define SPRITE_VA_ORIGIN 2
#define SPRITE_VA_ANGLES 3
#define SPRITE_VA_SCALE 4
#define SPRITE_VA_LERP 5

#define MAX_NUM_NODES 16

#define BINDING_POINT_SCENE_UBO 0
#define BINDING_POINT_CAMERA_UBO 1
#define BINDING_POINT_DLIGHT_UBO 2

#define BINDING_POINT_ENTITY_UBO 3
#define BINDING_POINT_STUDIO_UBO 3

#define BINDING_POINT_MATERIAL_SSBO 4

#define BINDING_POINT_OIT_FRAGMENT_SSBO 5
#define BINDING_POINT_OIT_NUMFRAGMENT_SSBO 6
#define BINDING_POINT_OIT_COUNTER_SSBO 7

#define WSURF_VA_POSITION 0
#define WSURF_VA_TEXCOORD 1
#define WSURF_VA_LIGHTMAP_TEXCOORD 2
#define WSURF_VA_NORMAL 3
#define WSURF_VA_S_TANGENT 4
#define WSURF_VA_T_TANGENT 5
#define WSURF_VA_TEXTURENUM 6
#define WSURF_VA_STYLES 7
#define WSURF_VA_MATID 8

#define WSURF_BIND_DIFFUSE_TEXTURE 0
#define WSURF_BIND_DETAIL_TEXTURE 1
#define WSURF_BIND_NORMAL_TEXTURE 2
#define WSURF_BIND_PARALLAX_TEXTURE 3
#define WSURF_BIND_SPECULAR_TEXTURE 4
#define WSURF_BIND_LIGHTMAP_TEXTURE_0 5
#define WSURF_BIND_LIGHTMAP_TEXTURE_1 6
#define WSURF_BIND_LIGHTMAP_TEXTURE_2 7
#define WSURF_BIND_LIGHTMAP_TEXTURE_3 8

#define STUDIO_BIND_TEXTURE_DIFFUSE				0
#define STUDIO_BIND_TEXTURE_NORMAL				2
#define STUDIO_BIND_TEXTURE_PARALLAX			3
#define STUDIO_BIND_TEXTURE_SPECULAR			4
#define STUDIO_BIND_TEXTURE_STENCIL				5
#define STUDIO_BIND_TEXTURE_ANIMATED			6
#define STUDIO_BIND_TEXTURE_SHADOW_DIFFUSE		7

#define STENCIL_MASK_ALL						0xFF
#define STENCIL_MASK_NONE						0

//Main view
#define STENCIL_MASK_NO_FOG						0x1
#define STENCIL_MASK_NO_LIGHTING				0x2
#define STENCIL_MASK_NO_BLOOM					0x4
#define STENCIL_MASK_NO_GLOW					0x8
#define STENCIL_MASK_HAS_FLATSHADE				0x10
#define STENCIL_MASK_HAS_DECAL					0x20
#define STENCIL_MASK_HAS_OUTLINE				0x40

//Studio view
#define STENCIL_MASK_HAS_SHADOW					0x1
#define STENCIL_MASK_HAS_FACE					0x2

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
#define DSHADE_BIND_CSM_TEXTURE				8
#define DSHADE_BIND_CUBEMAP_SHADOW_TEXTURE	9

#define DFINAL_BIND_DIFFUSE_TEXTURE			0
#define DFINAL_BIND_LIGHTMAP_TEXTURE		1
#define DFINAL_BIND_WORLDNORM_TEXTURE		2
#define DFINAL_BIND_SPECULAR_TEXTURE		3
#define DFINAL_BIND_DEPTH_TEXTURE			4
#define DFINAL_BIND_STENCIL_TEXTURE			5

#define CSM_RESOLUTION 4096.0
#define CSM_LEVELS 4

typedef struct vertex3f_s
{
	vec3_t	v;
}vertex3f_t;

typedef struct rect_instance_data_s
{
	mat4 matrix;
}rect_instance_data_t;

typedef struct texturedrectvertex_s
{
	vec2_t pos;
	vec2_t texcoord;
	vec4_t col;
}texturedrectvertex_t;

typedef struct filledrectvertex_s
{
	vec2_t pos;
	vec4_t col;
}filledrectvertex_t;

typedef struct triapivertex_s
{
	vec3_t	pos;
	vec2_t	texcoord;
	vec4_t	color;
}triapivertex_t;

typedef struct decalvertex_s
{
	vec3_t	pos;
	vec2_t	texcoord;
	vec2_t	lightmaptexcoord;
}decalvertex_t;

typedef struct decalvertextbn_s
{
	vec3_t	normal;
	vec3_t	s_tangent;
	vec3_t	t_tangent;
}decalvertextbn_t;

typedef struct decalinstancedata_s
{
	vec2_t	lightmaptexturenum;
	byte	styles[4];
	uint32_t matId;
}decalinstancedata_t;

typedef struct world_material_s
{
	vec2_t	diffuseScale;
	vec2_t	detailScale;
	vec2_t	normalScale;
	vec2_t	parallaxScale;
	vec2_t	specularScale;
}world_material_t;

typedef struct brushvertex_s
{
	vec3_t	pos;
	vec2_t	texcoord;
	vec2_t	lightmaptexcoord;
}brushvertex_t;

typedef struct brushvertextbn_s
{
	vec3_t	normal;
	vec3_t	s_tangent;
	vec3_t	t_tangent;
}brushvertextbn_t;

typedef struct brushinstancedata_s
{
	float	lightmaptexturenum_texcoordscale[2];//lightmaptexcoord[2]=lightmaptexnum //texcoord[2]=1.0f/texwidth, for SURF_DRAWTILED
	byte	styles[4];
	uint32_t matId;
}brushinstancedata_t;

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

typedef struct camera_view_s
{
	mat4 worldMatrix{};
	mat4 projMatrix{};
	mat4 invWorldMatrix{};
	mat4 invProjMatrix{};
	vec4_t viewport{};
	vec4_t frustum[4]{};
	vec4_t viewpos{};
	vec4_t vpn{};
	vec4_t vright{};
	vec4_t vup{};
}camera_view_t;

typedef struct camera_ubo_s
{
	camera_view_t views[6];
	int numViews;
	int padding;
	int padding2;
	int padding3;
}camera_ubo_t;

static_assert((sizeof(camera_ubo_t) % 16) == 0, "Size check");

//viewport.z=linkListSize
typedef struct scene_ubo_s
{
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
	float r_lightmap_scale;
	float r_lightmap_pow;
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
	float scale;
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
	virtual bool RunTask() { return false; };
	virtual void UploadResource() {};
};
