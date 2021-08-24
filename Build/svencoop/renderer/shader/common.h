#ifdef BINDLESS_ENABLED

#ifdef UINT64_ENABLED
#extension GL_NV_bindless_texture : require
#extension GL_NV_gpu_shader5 : require
#else
#extension GL_ARB_bindless_texture : require
#extension GL_ARB_gpu_shader_int64 : require
#endif

#endif

#if defined(OIT_ALPHA_BLEND_ENABLED) || defined(OIT_ADDITIVE_BLEND_ENABLED)

// See https://www.khronos.org/registry/OpenGL/extensions/ARB/ARB_shader_image_load_store.txt
#extension GL_ARB_shader_image_load_store : require

// See https://www.khronos.org/registry/OpenGL/extensions/ARB/ARB_fragment_shader_interlock.txt
#extension GL_ARB_fragment_shader_interlock : require

#ifdef IS_FRAGMENT_SHADER

// Use early z-test to cull transparent fragments occluded by opaque fragments.
// Additionaly, use fragment interlock.
layout(early_fragment_tests) in;

// gl_FragCoord will be used for pixel centers at integer coordinates.
// See https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/gl_FragCoord.xhtml
layout(pixel_center_integer) in vec4 gl_FragCoord;

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

#define BINDING_POINT_SCENE_UBO 0
#define BINDING_POINT_DECAL_SSBO 1
#define BINDING_POINT_TEXTURE_SSBO 1
#define BINDING_POINT_SPRITEFRAME_SSBO 1
#define BINDING_POINT_SPRITEENTRY_SSBO 2
#define BINDING_POINT_ENTITY_UBO 3
#define BINDING_POINT_STUDIO_UBO 3
#define BINDING_POINT_OIT_FRAGMENT_SSBO 4
#define BINDING_POINT_OIT_STARTOFFSET_SSBO 5
#define BINDING_POINT_OIT_ATOMIC_COUNTER 6

#define MAX_DEPTH_COMPLEXITY 8

#define SPR_VP_PARALLEL_UPRIGHT 0
#define SPR_FACING_UPRIGHT 1
#define SPR_VP_PARALLEL 2
#define SPR_ORIENTED 3
#define SPR_VP_PARALLEL_ORIENTED 4

struct scene_ubo_t{
	mat4 viewMatrix;
	mat4 projMatrix;
	mat4 invViewMatrix;
	mat4 invProjMatrix;
	mat4 shadowMatrix[3];
	ivec4 viewport;//viewport.z=linkListSize
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
	float r_g1;
	float r_g3;
	float v_brightness;
	float v_lightgamma;
	float v_lambert;
};

struct entity_ubo_t{
	mat4 entityMatrix;
	vec4 color;
	float scrollSpeed;
};

struct studio_ubo_t{
	float r_ambientlight;
	float r_shadelight;
	float r_blend;
	float r_scale;
	vec4 r_plightvec;
	vec4 r_colormix;
	vec4 r_origin;
	vec4 entity_origin;
	mat3x4 bonematrix[128];
};

struct texture_ssbo_t{

#if defined(BINDLESS_ENABLED) && defined(UINT64_ENABLED)

	uint64_t handles[5 * 1];

#else

	uvec2 handles[5 * 1];

#endif

};

struct spriteframe_ssbo_t{
	ivec4 type_width_height_texturenum;

	vec4 up_down_left_right;

#if defined(BINDLESS_ENABLED) && defined(UINT64_ENABLED)

	uint64_t texturehandle[2];

#else

	uvec2 texturehandle[2];

#endif
};

struct spriteframes_ssbo_t{
	spriteframe_ssbo_t frames[1];
};

struct spriteentry_ssbo_t
{
	vec4 color;
	vec4 origin;//origin[3] = scale
	vec4 angles;
	ivec4 frameindex;
};

struct spriteentries_ssbo_t{
	spriteentry_ssbo_t entries[1];
};

//Scene level

layout (std140, binding = BINDING_POINT_SCENE_UBO) uniform SceneBlock
{
   scene_ubo_t SceneUBO;
};

layout (std430, binding = BINDING_POINT_DECAL_SSBO) buffer DecalBlock
{
	texture_ssbo_t DecalSSBO;
};

layout (std430, binding = BINDING_POINT_SPRITEFRAME_SSBO) buffer SpriteFrameBlock
{
	spriteframes_ssbo_t SpriteFrameSSBO;
};

layout (std430, binding = BINDING_POINT_SPRITEENTRY_SSBO) buffer SpriteEntryBlock
{
	spriteentries_ssbo_t SpriteEntrySSBO;
};

//Entity level

layout (std140, binding = BINDING_POINT_ENTITY_UBO) uniform EntityBlock
{
	entity_ubo_t EntityUBO;
};

layout (std430, binding = BINDING_POINT_TEXTURE_SSBO) buffer TextureBlock
{
	texture_ssbo_t TextureSSBO;
};

layout (std140, binding = BINDING_POINT_STUDIO_UBO) uniform StudioBlock
{
	studio_ubo_t StudioUBO;
};

#if defined(OIT_ALPHA_BLEND_ENABLED) || defined(OIT_ADDITIVE_BLEND_ENABLED)

// A fragment node stores rendering information about one specific fragment
struct LinkedListFragmentNode
{
    // RGBA color of the node
    uint color;
    // Depth value of the fragment (in view space)
    float depth;
    // The index of the next node in "nodes" array
    uint next;
};

// Fragment-and-link buffer (linked list). Stores "nodesPerPixel" number of fragments.
layout (std430, binding = BINDING_POINT_OIT_FRAGMENT_SSBO) coherent buffer FragmentBuffer
{
    LinkedListFragmentNode OITFragmentSSBO[];
};

// Start-offset buffer (mapping pixels to first pixel in the buffer) of size viewportW*viewportH.
layout (std430, binding = BINDING_POINT_OIT_STARTOFFSET_SSBO) coherent buffer StartOffsetBuffer
{
    uint OITStartOffsetSSBO[];
};

layout(binding = BINDING_POINT_OIT_ATOMIC_COUNTER, offset = 0) uniform atomic_uint OITFragCounter;

#define MAX_NUM_FRAGS 32

#ifdef IS_FRAGMENT_SHADER
void GatherFragment(vec4 color)
{
    int x = int(gl_FragCoord.x);
    int y = int(gl_FragCoord.y);

	int viewportW = SceneUBO.viewport.x;
	int linkListSize = SceneUBO.viewport.z;

	int pixelIndex = viewportW * y + x;

	float z = gl_FragCoord.z / gl_FragCoord.w;

	LinkedListFragmentNode frag;
	frag.color = packUnorm4x8(color);
	frag.depth = gl_FragCoord.z;
	frag.next = -1;

	#ifdef OIT_ADDITIVE_BLEND_ENABLED

		frag.depth = -frag.depth;

	#endif

	uint insertIndex = atomicCounterIncrement(OITFragCounter);

	if (insertIndex < linkListSize) {
		// Insert the fragment into the linked list
		frag.next = atomicExchange(OITStartOffsetSSBO[pixelIndex], insertIndex);
		OITFragmentSSBO[insertIndex] = frag;
	}
}
#endif

#endif

vec2 UnitVectorToHemiOctahedron(vec3 dir) {

	dir.y = max(dir.y, 0.0001);
	dir.xz /= dot(abs(dir), vec3(1.0));

	return clamp(0.5 * vec2(dir.x + dir.z, dir.x - dir.z) + 0.5, 0.0, 1.0);

}

vec3 HemiOctahedronToUnitVector(vec2 coord) {

	coord = 2.0 * coord - 1.0;
	coord = 0.5 * vec2(coord.x + coord.y, coord.x - coord.y);

	float y = 1.0 - dot(vec2(1.0), abs(coord));
	return normalize(vec3(coord.x, y + 0.0001, coord.y));

}

vec2 UnitVectorToOctahedron(vec3 dir) {

    dir.xz /= dot(abs(dir), vec3(1.0));

	// Lower hemisphere
	if (dir.y < 0.0) {
		vec2 orig = dir.xz;
		dir.x = (orig.x >= 0.0 ? 1.0 : -1.0) * (1.0 - abs(orig.y));
        dir.z = (orig.y >= 0.0 ? 1.0 : -1.0) * (1.0 - abs(orig.x));
	}

	return clamp(0.5 * vec2(dir.x, dir.z) + 0.5, 0.0, 1.0);

}

vec3 OctahedronToUnitVector(vec2 coord) {

	coord = 2.0 * coord - 1.0;
	float y = 1.0 - dot(abs(coord), vec2(1.0));

	// Lower hemisphere
	if (y < 0.0) {
		vec2 orig = coord;
		coord.x = (orig.x >= 0.0 ? 1.0 : -1.0) * (1.0 - abs(orig.y));
		coord.y = (orig.y >= 0.0 ? 1.0 : -1.0) * (1.0 - abs(orig.x));
	}

	return normalize(vec3(coord.x, y + 0.0001, coord.y));

}

#if defined(LINEAR_FOG_ENABLED) && defined(IS_FRAGMENT_SHADER)

vec4 CalcFog(vec4 color)
{
	float z = gl_FragCoord.z / gl_FragCoord.w;
	float fogFactor = ( SceneUBO.fogEnd - z ) / ( SceneUBO.fogEnd - SceneUBO.fogStart );
	fogFactor = clamp(fogFactor, 0.0, 1.0);

	vec3 finalColor = color.xyz;

	color.xyz = mix(SceneUBO.fogColor.xyz, finalColor, fogFactor );

	return color;
}

#else

vec4 CalcFog(vec4 color)
{
	return color;
}

#endif