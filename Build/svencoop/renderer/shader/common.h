#ifdef BINDLESS_ENABLED

#ifdef UINT64_ENABLE
#extension GL_NV_bindless_texture : require
#extension GL_NV_gpu_shader5 : require
#else
#extension GL_ARB_bindless_texture : require
#extension GL_ARB_gpu_shader_int64 : require
#endif

#endif

#define TEXTURE_SSBO_DIFFUSE 0
#define TEXTURE_SSBO_DETAIL 1
#define TEXTURE_SSBO_NORMAL 2
#define TEXTURE_SSBO_PARALLAX 3
#define TEXTURE_SSBO_SPECULAR 4

#define TEXTURE_SSBO_WATER_BASE 0
#define TEXTURE_SSBO_WATER_NORMAL 1
#define TEXTURE_SSBO_WATER_REFLECT 2
#define TEXTURE_SSBO_WATER_REFRACT 3
#define TEXTURE_SSBO_WATER_DEPTH 4

struct scene_ubo_t{
	mat4 viewMatrix;
	mat4 projMatrix;
	mat4 invViewMatrix;
	mat4 invProjMatrix;
	mat4 shadowMatrix[3];
	vec4 viewpos;
	vec4 fogColor;
	float fogStart;
	float fogEnd;
	float time;
	float padding;
	vec4 shadowDirection;
	vec4 shadowColor;
	vec4 shadowFade;
	vec4 clipPlane;
};

struct entity_ubo_t{
	mat4 entityMatrix;
	vec4 color;
	float scrollSpeed;
};

struct texture_ssbo_t{

#if defined(BINDLESS_ENABLED) && defined(UINT64_ENABLE)

	uint64_t handles[5 * 1];

#else

	uvec2 handles[5 * 1];

#endif

};

//Scene level

layout (std140, binding = 0) uniform SceneBlock
{
   scene_ubo_t SceneUBO;
};

layout (std430, binding = 1) buffer DecalBlock
{
    texture_ssbo_t DecalSSBO;
};

//Entity level

layout (std140, binding = 2) uniform EntityBlock
{
   entity_ubo_t EntityUBO;
};

layout (std430, binding = 3) buffer TextureBlock
{
    texture_ssbo_t TextureSSBO;
};