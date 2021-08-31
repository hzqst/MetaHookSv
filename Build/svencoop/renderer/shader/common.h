#ifdef BINDLESS_ENABLED

#ifdef UINT64_ENABLED
#extension GL_NV_bindless_texture : require
#extension GL_NV_gpu_shader5 : require
#else
#extension GL_ARB_bindless_texture : require
#endif

#endif

#if defined(OIT_ALPHA_BLEND_ENABLED) || defined(OIT_ADDITIVE_BLEND_ENABLED)

#ifdef IS_FRAGMENT_SHADER

// See https://www.khronos.org/registry/OpenGL/extensions/ARB/ARB_shader_image_load_store.txt
#extension GL_ARB_shader_image_load_store : require

// See https://www.khronos.org/registry/OpenGL/extensions/ARB/ARB_fragment_shader_interlock.txt
#extension GL_ARB_fragment_shader_interlock : require

// Use early z-test to cull transparent fragments occluded by opaque fragments.
// Additionaly, use fragment interlock.
layout(early_fragment_tests) in;

// gl_FragCoord will be used for pixel centers at integer coordinates.
// See https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/gl_FragCoord.xhtml
layout(pixel_center_integer) in vec4 gl_FragCoord;

#define MAX_NUM_NODES 16

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
#define BINDING_POINT_SKYBOX_SSBO 1
#define BINDING_POINT_DECAL_SSBO 1
#define BINDING_POINT_TEXTURE_SSBO 1
#define BINDING_POINT_ENTITY_UBO 2
#define BINDING_POINT_STUDIO_UBO 2
#define BINDING_POINT_OIT_FRAGMENT_SSBO 3
#define BINDING_POINT_OIT_NUMFRAGMENT_SSBO 4
#define BINDING_POINT_OIT_COUNTER_SSBO 5

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

struct spriteframe_ssbo_t{
	ivec4 type_width_height_texturenum;

	vec4 up_down_left_right;

#if defined(BINDLESS_ENABLED) && defined(UINT64_ENABLED)

	uint64_t texturehandle[1];

#else

	uvec2 texturehandle[1];

#endif
};

//Scene level

layout (std140, binding = BINDING_POINT_SCENE_UBO) uniform SceneBlock
{
   scene_ubo_t SceneUBO;
};

layout (std430, binding = BINDING_POINT_DECAL_SSBO) coherent buffer DecalBlock
{
#if defined(BINDLESS_ENABLED) && defined(UINT64_ENABLED)

	uint64_t DecalSSBO[];

#else

	uvec2 DecalSSBO[];

#endif
};

layout (std430, binding = BINDING_POINT_SKYBOX_SSBO) coherent buffer SkyboxBlock
{
#if defined(BINDLESS_ENABLED) && defined(UINT64_ENABLED)

	uint64_t SkyboxSSBO[];

#else

	uvec2 SkyboxSSBO[];

#endif
};

//Entity level

layout (std140, binding = BINDING_POINT_ENTITY_UBO) uniform EntityBlock
{
	entity_ubo_t EntityUBO;
};

layout (std430, binding = BINDING_POINT_TEXTURE_SSBO) coherent buffer TextureBlock
{
#if defined(BINDLESS_ENABLED) && defined(UINT64_ENABLED)

	uint64_t TextureSSBO[];

#else

	uvec2 TextureSSBO[];

#endif
};

layout (std140, binding = BINDING_POINT_STUDIO_UBO) uniform StudioBlock
{
	studio_ubo_t StudioUBO;
};

#if defined(OIT_ALPHA_BLEND_ENABLED) || defined(OIT_ADDITIVE_BLEND_ENABLED)

// A fragment node stores rendering information about one specific fragment
struct FragmentNode
{
    // RGBA color of the node
    uint color;
    // Depth value of the fragment (in view space)
    float depth;
    // The index of the next node in "nodes" array
    uint next;
};

// Fragment-and-link buffer (linked list). Stores "nodesPerPixel" number of fragments.
layout (std430, binding = BINDING_POINT_OIT_FRAGMENT_SSBO) coherent buffer FragmentNodes
{
    FragmentNode nodes[];
};

// Start-offset buffer (mapping pixels to first pixel in the buffer) of size viewportW*viewportH.
layout (std430, binding = BINDING_POINT_OIT_NUMFRAGMENT_SSBO) coherent buffer NumFragmentsBuffer
{
    uint numFragments[];
};

layout(binding = BINDING_POINT_OIT_COUNTER_SSBO, offset = 0) uniform atomic_uint fragCounter;

#ifdef IS_FRAGMENT_SHADER

void GatherFragment(inout vec4 color)
{
    uint x = uint(gl_FragCoord.x);
    uint y = uint(gl_FragCoord.y);
	uint viewportW = SceneUBO.viewport.x;
	uint linkedListSize = SceneUBO.viewport.z;

	uint pixelIndex = viewportW*y + x;

	FragmentNode frag;
	frag.color = packUnorm4x8(color);
	#ifdef OIT_ADDITIVE_BLEND_ENABLED
		frag.depth = -gl_FragCoord.z;
	#else
		frag.depth = gl_FragCoord.z;
	#endif
	
	frag.next = -1;

	uint insertIndex = atomicCounterIncrement(fragCounter);

	if (insertIndex < linkedListSize) {
        // Insert the fragment into the linked list
		frag.next = atomicExchange(numFragments[pixelIndex], insertIndex);
        nodes[insertIndex] = frag;
    }
	
	discard;
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

vec4 CalcFogWithDistance(vec4 color, float z)
{
	float fogFactor = ( SceneUBO.fogEnd - z ) / ( SceneUBO.fogEnd - SceneUBO.fogStart );
	fogFactor = clamp(fogFactor, 0.0, 1.0);

	vec3 finalColor = color.xyz;

	color.xyz = mix(SceneUBO.fogColor.xyz, finalColor, fogFactor );

	return color;
}

vec4 CalcFog(vec4 color)
{
	return CalcFogWithDistance(color, gl_FragCoord.z / gl_FragCoord.w);
}

#else

vec4 CalcFog(vec4 color)
{
	return color;
}

vec4 CalcFogWithLinearDepth(vec4 color, sampler2D linearDepthTex, vec2 texcoord)
{
	return color;
}

#endif

vec4 GammaToLinear(vec4 color)
{
	color.rgb = pow(color.rgb, vec3(SceneUBO.v_gamma));
	return color;
}

vec4 TexGammaToLinear(vec4 color)
{
	color.rgb = pow(color.rgb, vec3(SceneUBO.v_texgamma));
	return color;
}

//This was being applied for GL_Upload16
vec4 TexGammaToGamma(vec4 color)
{
	color.rgb = pow(color.rgb, vec3(SceneUBO.v_texgamma * SceneUBO.r_g));//r_g = 1.0 / v_gamma
	return color;
}

vec4 LinearToGamma(vec4 color)
{
	color.rgb = pow(color.rgb, vec3(SceneUBO.r_g));//r_g = 1.0 / v_gamma
	return color;
}

//This was being applied for R_BuildLightMap and R_StudioLighting
float LightGammaToGammaInternal(float color)
{
	float fv = pow(color, SceneUBO.v_lightgamma);

	fv = fv * max(SceneUBO.v_brightness, 1.0);

	//if (fv > SceneUBO.r_g3)
	float fv1 = 0.125 + ((fv - SceneUBO.r_g3) / (1.0 - SceneUBO.r_g3)) * 0.875;
	//else 
	float fv2 = (fv / SceneUBO.r_g3) * 0.125;

	fv = clamp(fv1, fv2, step(fv, SceneUBO.r_g3));

	return clamp(pow( fv, SceneUBO.r_g ), 0.0, 1.0);
}

vec4 LightGammaToGamma(vec4 color)
{
	return vec4(LightGammaToGammaInternal(color.r), LightGammaToGammaInternal(color.g), LightGammaToGammaInternal(color.b), color.a);
}

float LightGammaToLinearInternal(float color)
{
	float fv = pow(color, SceneUBO.v_lightgamma);

	fv = fv * max(SceneUBO.v_brightness, 1.0);

	//if (fv > SceneUBO.r_g3)
	float fv1 = 0.125 + ((fv - SceneUBO.r_g3) / (1.0 - SceneUBO.r_g3)) * 0.875;
	//else 
	float fv2 = (fv / SceneUBO.r_g3) * 0.125;

	fv = clamp(fv1, fv2, step(fv, SceneUBO.r_g3));

	return clamp(fv, 0.0, 1.0);
}

vec4 LightGammaToLinear(vec4 color)
{
	return vec4(LightGammaToLinearInternal(color.r), LightGammaToLinearInternal(color.g), LightGammaToLinearInternal(color.b), color.a);
}

#if defined(CLIP_ENABLED) && defined(IS_FRAGMENT_SHADER)
void ClipPlaneTest(vec3 worldpos, vec3 normal)
{
	vec4 clipVec = vec4(worldpos.xyz, 1);
	vec4 clipPlane = SceneUBO.clipPlane;
	if(dot(clipVec, clipPlane) < 0)
		discard;

	clipPlane.w += 32.0;
	if(dot(clipVec, clipPlane) < 0 && dot(normalize(normal.xyz), -clipPlane.xyz) > 0.866)
		discard;
}
#else
void ClipPlaneTest(vec3 worldpos, vec3 normal)
{

}
#endif