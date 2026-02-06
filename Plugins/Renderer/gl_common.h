#pragma once

#ifndef GL_COMMON_H
#define GL_COMMON_H

#include "qgl.h"
#include <stdint.h>

#define OFFSET(type, variable) ((const void*)&(((type*)NULL)->variable))

#define BUFFER_OFFSET(i) ((unsigned int *)NULL + (i))

#define DRAW_CLASSIFY_WORLD				0x1
#define DRAW_CLASSIFY_SKYBOX			0x2
#define DRAW_CLASSIFY_OPAQUE_ENTITIES	0x4
#define DRAW_CLASSIFY_TRANS_ENTITIES	0x8
#define DRAW_CLASSIFY_PARTICLES			0x10
#define DRAW_CLASSIFY_DECAL				0x20
#define DRAW_CLASSIFY_WATER				0x40
#define DRAW_CLASSIFY_LIGHTMAP			0x80

#define DRAW_CLASSIFY_ALL				(DRAW_CLASSIFY_WORLD |\
										DRAW_CLASSIFY_SKYBOX | \
										DRAW_CLASSIFY_OPAQUE_ENTITIES | \
										DRAW_CLASSIFY_TRANS_ENTITIES |\
										DRAW_CLASSIFY_PARTICLES |\
										DRAW_CLASSIFY_DECAL | \
										DRAW_CLASSIFY_WATER | \
										DRAW_CLASSIFY_LIGHTMAP\
)

#define LUMIN1x1_BUFFERS 3
#define DOWNSAMPLE_BUFFERS 2
#define LUMIN_BUFFERS 3
#define BLUR_BUFFERS 3

class CCompileShaderArgs
{
public:
	const char* vsfile{};
	const char* gsfile{};
	const char* fsfile{};
	const char* vsdefine{};
	const char* gsdefine{};
	const char* fsdefine{};
};

typedef struct FBO_Container_s
{
	GLuint s_hBackBufferFBO;
	GLuint unused;
	GLuint s_hBackBufferDepthView;
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
#define WSURF_VA_NORMAL 1
#define WSURF_VA_TEXCOORD 2
#define WSURF_VA_LIGHTMAP_TEXCOORD 3
#define WSURF_VA_S_TANGENT 4
#define WSURF_VA_T_TANGENT 5
#define WSURF_VA_SMOOTHNORMAL 6
#define WSURF_VA_PACKED_MATID 7
#define WSURF_VA_STYLES 8
#define WSURF_VA_DIFFUSESCALE 9

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
#define STUDIO_BIND_TEXTURE_ANIMATED			5
#define STUDIO_BIND_TEXTURE_STENCIL				6
#define STUDIO_BIND_TEXTURE_MIX_DIFFUSE			7
#define STUDIO_BIND_TEXTURE_DEPTH				8

#define STENCIL_MASK_ALL						0xFF
#define STENCIL_MASK_NONE						0

//Main view
#define STENCIL_MASK_NO_FOG						0x1
#define STENCIL_MASK_NO_LIGHTING				0x2
#define STENCIL_MASK_NO_BLOOM					0x4
#define STENCIL_MASK_NO_GLOW_BLUR				0x8
#define STENCIL_MASK_HAS_FLATSHADE				0x10
#define STENCIL_MASK_HAS_DECAL					0x20
#define STENCIL_MASK_HAS_OUTLINE				0x40
#define STENCIL_MASK_NO_GLOW_COLOR				0x40

//Studio view
#define STENCIL_MASK_HAS_SHADOW					0x1
#define STENCIL_MASK_HAS_FACE					0x2

#define WATER_BIND_BASE_TEXTURE				0
#define WATER_BIND_NORMAL_TEXTURE			1
#define WATER_BIND_REFRACT_TEXTURE			2
#define WATER_BIND_REFRACT_DEPTH_TEXTURE	3
#define WATER_BIND_REFLECT_TEXTURE			4
#define WATER_BIND_REFLECT_DEPTH_TEXTURE	5

#define DSHADE_BIND_DIFFUSE_TEXTURE					0
#define DSHADE_BIND_LIGHTMAP_TEXTURE				1
#define DSHADE_BIND_WORLDNORM_TEXTURE				2
#define DSHADE_BIND_SPECULAR_TEXTURE				3
#define DSHADE_BIND_DEPTH_TEXTURE					4
#define DSHADE_BIND_STENCIL_TEXTURE					5
#define DSHADE_BIND_CONE_TEXTURE					6
#define DSHADE_BIND_STATIC_SHADOW_TEXTURE			7
#define DSHADE_BIND_DYNAMIC_SHADOW_TEXTURE			8
#define DSHADE_BIND_STATIC_CUBEMAP_SHADOW_TEXTURE	9
#define DSHADE_BIND_DYNAMIC_CUBEMAP_SHADOW_TEXTURE	10
#define DSHADE_BIND_CSM_TEXTURE						11

#define DFINAL_BIND_DIFFUSE_TEXTURE			0
#define DFINAL_BIND_LIGHTMAP_TEXTURE		1
#define DFINAL_BIND_WORLDNORM_TEXTURE		2
#define DFINAL_BIND_SPECULAR_TEXTURE		3
#define DFINAL_BIND_DEPTH_TEXTURE			4
#define DFINAL_BIND_STENCIL_TEXTURE			5

#define CSM_LEVELS 4

#define DRAW_TEXTURED_RECT_ALPHA_BLEND_ENABLED 0x1ull
#define DRAW_TEXTURED_RECT_ADDITIVE_BLEND_ENABLED 0x2ull
#define DRAW_TEXTURED_RECT_ALPHA_BASED_ADDITIVE_ENABLED 0x4ull
#define DRAW_TEXTURED_RECT_SCISSOR_ENABLED 0x8ull
#define DRAW_TEXTURED_RECT_ALPHA_TEST_ENABLED 0x10ull
#define DRAW_TEXTURED_RECT_MASK_TEXTURE_ENABLED 0x20ull

#define DRAW_FILLED_RECT_ALPHA_BLEND_ENABLED 0x1ull
#define DRAW_FILLED_RECT_ADDITIVE_BLEND_ENABLED 0x2ull
#define DRAW_FILLED_RECT_ALPHA_BASED_ADDITIVE_ENABLED 0x4ull
#define DRAW_FILLED_RECT_ZERO_SRC_ALPHA_BLEND_ENABLED 0x8ull
#define DRAW_FILLED_RECT_SCISSOR_ENABLED 0x8ull
#define DRAW_FILLED_RECT_LINE_ENABLED 0x10ull

#define GBUFFER_INDEX_DIFFUSE		0
#define GBUFFER_INDEX_LIGHTMAP		1
#define GBUFFER_INDEX_WORLDNORM		2
#define GBUFFER_INDEX_SPECULAR		3
#define GBUFFER_INDEX_MAX			4

#define GBUFFER_INTERNAL_FORMAT_DIFFUSE			GL_RGB16F
#define GBUFFER_INTERNAL_FORMAT_LIGHTMAP		GL_RGB16F
#define GBUFFER_INTERNAL_FORMAT_WORLDNORM		GL_RGB16F
#define GBUFFER_INTERNAL_FORMAT_SPECULAR		GL_RGB8

#define GBUFFER_MASK_DIFFUSE		(1<<GBUFFER_INDEX_DIFFUSE)
#define GBUFFER_MASK_LIGHTMAP		(1<<GBUFFER_INDEX_LIGHTMAP)
#define GBUFFER_MASK_WORLDNORM		(1<<GBUFFER_INDEX_WORLDNORM)
#define GBUFFER_MASK_SPECULAR		(1<<GBUFFER_INDEX_SPECULAR)

#define GBUFFER_MASK_ALL			(GBUFFER_MASK_DIFFUSE | GBUFFER_MASK_LIGHTMAP | GBUFFER_MASK_WORLDNORM | GBUFFER_MASK_SPECULAR)

#define DLIGHT_SPOT_ENABLED								0x1ull
#define DLIGHT_POINT_ENABLED							0x2ull
#define DLIGHT_VOLUME_ENABLED							0x4ull
#define DLIGHT_CONE_TEXTURE_ENABLED						0x8ull
#define DLIGHT_DIRECTIONAL_ENABLED						0x10ull
#define DLIGHT_STATIC_SHADOW_TEXTURE_ENABLED			0x20ull
#define DLIGHT_DYNAMIC_SHADOW_TEXTURE_ENABLED			0x40ull
#define DLIGHT_STATIC_CUBEMAP_SHADOW_TEXTURE_ENABLED	0x80ull
#define DLIGHT_DYNAMIC_CUBEMAP_SHADOW_TEXTURE_ENABLED	0x100ull
#define DLIGHT_CSM_SHADOW_TEXTURE_ENABLED				0x200ull
#define DLIGHT_PCF_ENABLED								0x400ull

#define DFINAL_LINEAR_FOG_ENABLED				0x1ull
#define DFINAL_EXP_FOG_ENABLED					0x2ull
#define DFINAL_EXP2_FOG_ENABLED					0x4ull
#define DFINAL_SKY_FOG_ENABLED					0x8ull
#define DFINAL_SSR_ENABLED						0x10ull
#define DFINAL_SSR_ADAPTIVE_STEP_ENABLED		0x20ull
#define DFINAL_SSR_EXPONENTIAL_STEP_ENABLED		0x40ull
#define DFINAL_SSR_BINARY_SEARCH_ENABLED		0x80ull
#define DFINAL_LINEAR_FOG_SHIFT_ENABLED			0x100ull

#define PORTAL_OVERLAY_TEXTURE_ENABLED				0x1ull
#define PORTAL_TEXCOORD_ENABLED						0x2ull
#define PORTAL_REVERSE_TEXCOORD_ENABLED				0x4ull
#define PORTAL_GAMMA_BLEND_ENABLED					0x8ull

#define WATER_LEGACY_ENABLED				0x1ull
#define WATER_UNDERWATER_ENABLED			0x2ull
#define WATER_GBUFFER_ENABLED				0x4ull
#define WATER_DEPTH_ENABLED					0x8ull
#define WATER_REFRACT_ENABLED				0x10ull
#define WATER_LINEAR_FOG_ENABLED			0x20ull
#define WATER_EXP_FOG_ENABLED				0x40ull
#define WATER_EXP2_FOG_ENABLED				0x80ull
#define WATER_ALPHA_BLEND_ENABLED			0x100ull
#define WATER_ADDITIVE_BLEND_ENABLED		0x200ull
#define WATER_OIT_BLEND_ENABLED				0x400ull
#define WATER_GAMMA_BLEND_ENABLED			0x800ull
#define WATER_LINEAR_FOG_SHIFT_ENABLED		0x1000ull

#define WSURF_DIFFUSE_ENABLED				0x1ull
#define WSURF_LIGHTMAP_ENABLED				0x2ull
#define WSURF_DETAILTEXTURE_ENABLED			0x4ull
#define WSURF_NORMALTEXTURE_ENABLED			0x8ull
#define WSURF_PARALLAXTEXTURE_ENABLED		0x10ull
#define WSURF_SPECULARTEXTURE_ENABLED		0x20ull
#define WSURF_LINEAR_FOG_ENABLED			0x40ull
#define WSURF_EXP_FOG_ENABLED				0x80ull
#define WSURF_EXP2_FOG_ENABLED				0x100ull
#define WSURF_GBUFFER_ENABLED				0x200ull
#define WSURF_SHADOW_CASTER_ENABLED			0x400ull
#define WSURF_SKYBOX_ENABLED				0x800ull
#define WSURF_DECAL_ENABLED					0x1000ull
#define WSURF_CLIP_ENABLED					0x2000ull
#define WSURF_CLIP_WATER_ENABLED			0x4000ull
#define WSURF_ALPHA_BLEND_ENABLED			0x8000ull
#define WSURF_ADDITIVE_BLEND_ENABLED		0x10000ull
#define WSURF_OIT_BLEND_ENABLED				0x20000ull
#define WSURF_GAMMA_BLEND_ENABLED			0x40000ull
#define WSURF_FULLBRIGHT_ENABLED			0x80000ull
#define WSURF_COLOR_FILTER_ENABLED			0x100000ull
#define WSURF_LIGHTMAP_INDEX_0_ENABLED		0x200000ull
#define WSURF_LIGHTMAP_INDEX_1_ENABLED		0x400000ull
#define WSURF_LIGHTMAP_INDEX_2_ENABLED		0x800000ull
#define WSURF_LIGHTMAP_INDEX_3_ENABLED		0x1000000ull
#define WSURF_LEGACY_DLIGHT_ENABLED			0x2000000ull
#define WSURF_ALPHA_SOLID_ENABLED			0x4000000ull
#define WSURF_LINEAR_FOG_SHIFT_ENABLED		0x8000000ull
#define WSURF_REVERT_NORMAL_ENABLED			0x10000000ull
#define WSURF_MULTIVIEW_ENABLED				0x20000000ull
#define WSURF_LINEAR_DEPTH_ENABLED			0x40000000ull
#define WSURF_GLOW_COLOR_ENABLED			0x80000000ull
#define WSURF_STENCIL_NO_GLOW_BLUR_ENABLED	0x100000000ull
#define WSURF_STENCIL_NO_GLOW_COLOR_ENABLED	0x200000000ull
#define WSURF_DOUBLE_FACE_ENABLED			0x400000000ull

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

#define STUDIO_GBUFFER_ENABLED					0x80000ull
#define STUDIO_LINEAR_FOG_ENABLED				0x100000ull
#define STUDIO_EXP_FOG_ENABLED					0x200000ull
#define STUDIO_EXP2_FOG_ENABLED					0x400000ull
#define STUDIO_LINEAR_FOG_SHIFT_ENABLED			0x800000ull
#define STUDIO_SHADOW_CASTER_ENABLED			0x1000000ull
#define STUDIO_GLOW_SHELL_ENABLED				0x2000000ull
#define STUDIO_OUTLINE_ENABLED					0x4000000ull
#define STUDIO_HAIR_SHADOW_ENABLED				0x8000000ull
#define STUDIO_HAIR_FACE_COLOR_MIX_ENABLED		0x10000000ull
#define STUDIO_CLIP_WATER_ENABLED				0x20000000ull
#define STUDIO_CLIP_ENABLED						0x40000000ull
#define STUDIO_ALPHA_BLEND_ENABLED				0x80000000ull
#define STUDIO_ADDITIVE_BLEND_ENABLED			0x100000000ull
#define STUDIO_OIT_BLEND_ENABLED				0x200000000ull
#define STUDIO_GAMMA_BLEND_ENABLED				0x400000000ull
#define STUDIO_ADDITIVE_RENDER_MODE_ENABLED		0x800000000ull
#define STUDIO_NORMALTEXTURE_ENABLED			0x1000000000ull
#define STUDIO_PARALLAXTEXTURE_ENABLED			0x2000000000ull
#define STUDIO_SPECULARTEXTURE_ENABLED			0x4000000000ull
#define STUDIO_PACKED_DIFFUSETEXTURE_ENABLED	0x8000000000ull
#define STUDIO_PACKED_NORMALTEXTURE_ENABLED		0x10000000000ull
#define STUDIO_PACKED_PARALLAXTEXTURE_ENABLED	0x20000000000ull
#define STUDIO_PACKED_SPECULARTEXTURE_ENABLED	0x40000000000ull
#define STUDIO_ANIMATED_TEXTURE_ENABLED			0x80000000000ull
#define STUDIO_REVERT_NORMAL_ENABLED			0x100000000000ull
#define STUDIO_STENCIL_TEXTURE_ENABLED			0x200000000000ull
#define STUDIO_MIX_DIFFUSE_TEXTURE_ENABLED		0x400000000000ull
#define STUDIO_DEPTH_TEXTURE_ENABLED			0x800000000000ull
#define STUDIO_CLIP_BONE_ENABLED				0x1000000000000ull
#define STUDIO_LEGACY_DLIGHT_ENABLED			0x2000000000000ull
#define STUDIO_LEGACY_ELIGHT_ENABLED			0x4000000000000ull
#define STUDIO_CLIP_NEARPLANE_ENABLED			0x8000000000000ull
#define STUDIO_STENCIL_NO_GLOW_BLUR_ENABLED		0x10000000000000ull
#define STUDIO_STENCIL_NO_GLOW_COLOR_ENABLED	0x20000000000000ull
#define STUDIO_GLOW_COLOR_ENABLED				0x40000000000000ull
#define STUDIO_MULTIVIEW_ENABLED				0x80000000000000ull
#define STUDIO_LINEAR_DEPTH_ENABLED				0x100000000000000ull
#define STUDIO_DEBUG_ENABLED					0x200000000000000ull

#define STUDIO_PACKED_TEXTURE_ALLBITS	(STUDIO_PACKED_DIFFUSETEXTURE_ENABLED | STUDIO_PACKED_NORMALTEXTURE_ENABLED | STUDIO_PACKED_PARALLAXTEXTURE_ENABLED | STUDIO_PACKED_SPECULARTEXTURE_ENABLED)

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
	vec3_t	smoothnormal;
}brushvertextbn_t;

typedef struct brushinstancedata_s
{
	uint16_t packed_matId[2];
	byte	styles[4];
	float	diffusescale;//1.0f/texwidth, for SURF_DRAWTILED
}brushinstancedata_t;

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
	vec4_t frustum[4]{};
	vec4_t viewport{};
	vec4_t viewpos{};
	vec4_t vpn{};
	vec4_t vright_znear{};
	vec4_t vup_zfar{};
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
	float r_alphamin;
	float r_linear_blend_shift;
	float r_lightscale;
	float r_linear_fog_shift;
	float r_linear_fog_shiftz;
	vec4 r_filtercolor;
	vec4 r_lightstylevalue[256 / 4];
	float r_lightmap_scale;
	float r_lightmap_pow;
	float padding;
	float padding2;
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
	mat4 r_entityMatrix;
	vec4 r_color;
	float r_scrollSpeed;
	float r_scale;
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

#endif //GL_COMMON_H