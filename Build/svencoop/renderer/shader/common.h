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
layout(early_fragment_tests, pixel_interlock_ordered) in;

// gl_FragCoord will be used for pixel centers at integer coordinates.
// See https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/gl_FragCoord.xhtml
layout(pixel_center_integer) in vec4 gl_FragCoord;

#define MAX_NUM_NODES 8

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
#define BINDING_POINT_OIT_NUMFRAGMENT_SSBO 5

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
struct FragmentNode
{
    // RGBA color of the node
    uint color;
    // Depth value of the fragment (in view space)
    float depth;
};

// Fragment-and-link buffer (linked list). Stores "nodesPerPixel" number of fragments.
layout (std430, binding = BINDING_POINT_OIT_FRAGMENT_SSBO) coherent buffer FragmentNodes
{
    FragmentNode nodes[];
};

// Start-offset buffer (mapping pixels to first pixel in the buffer) of size viewportW*viewportH.
layout (std430, binding = BINDING_POINT_OIT_NUMFRAGMENT_SSBO) coherent buffer NumFragmentsBuffer
{
    uint numFragmentsBuffer[];
};

#ifdef IS_FRAGMENT_SHADER

#if defined(ADRESSING_MORTON_CODE_8x8)
// Space-filling and locality-preserving curve mapping pixel positions in an 8x8 tile to
// a linearized memory offset in the tile.
uint mortonCodeLookupTable[64] = {
    0,  1,  4,  5,  16, 17, 20, 21,
    2,  3,  6,  7,  18, 19, 22, 23,
    8,  9,  12, 13, 24, 25, 28, 29,
    10, 11, 14, 15, 26, 27, 30, 31,
    32, 33, 36, 37, 48, 49, 52, 53,
    34, 35, 38, 39, 50, 51, 54, 55,
    40, 41, 44, 45, 56, 57, 60, 61,
    42, 43, 46, 47, 58, 59, 62, 63
};
#endif

// Address 1D structured buffers as tiled to better data exploit locality
// "OIT to Volumetric Shadow Mapping, 101 Uses for Raster Ordered Views using DirectX 12",
// by Leigh Davies (Intel), March 05, 2015
uint addrGen(uvec2 addr2D, uint viewportW)
{
#if defined(ADRESSING_MORTON_CODE_8x8)
    uint surfaceWidth = viewportW >> 3U; // / 8U
    uvec2 tileAddr2D = addr2D >> 3U; // / 8U
    uint tileAddr1D = (tileAddr2D.x + surfaceWidth * tileAddr2D.y) << 6U; // * 64U
    uvec2 pixelAddr2D = addr2D & 7U;
    uint pixelAddr1D = pixelAddr2D.x + (pixelAddr2D.y << 3U);
    return tileAddr1D | mortonCodeLookupTable[pixelAddr1D];
#elif defined(ADDRESSING_TILED_2x2)
    uint surfaceWidth = viewportW >> 1U; // / 2U
    uvec2 tileAddr2D = addr2D >> 1U; // / 2U
    uint tileAddr1D = (tileAddr2D.x + surfaceWidth * tileAddr2D.y) << 2U; // * 4U
    uvec2 pixelAddr2D = addr2D & 1U;
    uint pixelAddr1D = (pixelAddr2D.x) + (pixelAddr2D.y << 1U);
    return tileAddr1D | pixelAddr1D;
#elif defined(ADDRESSING_TILED_2x8)
    uint surfaceWidth = viewportW >> 1U; // / 2U;
    uvec2 tileAddr2D = addr2D / uvec2(2U, 8U);
    uint tileAddr1D = (tileAddr2D.x + surfaceWidth * tileAddr2D.y) << 4U; // * 16U;
    uvec2 pixelAddr2D = addr2D & uvec2(1U, 7U);
    uint pixelAddr1D = pixelAddr2D.x + pixelAddr2D.y * 2U;
    return tileAddr1D | pixelAddr1D;
#elif defined(ADDRESSING_TILED_NxM)
    uint surfaceWidth = viewportW / TILE_N;
    uvec2 tileAddr2D = addr2D / uvec2(TILE_N, TILE_M);
    uint tileAddr1D = (tileAddr2D.x + surfaceWidth * tileAddr2D.y) * (TILE_N * TILE_M);
    uvec2 pixelAddr2D = addr2D & uvec2(TILE_N-1, TILE_M-1);
    uint pixelAddr1D = pixelAddr2D.x + pixelAddr2D.y * TILE_N;
    return tileAddr1D | pixelAddr1D;
#else
    return addr2D.x + viewportW * addr2D.y;
#endif
}

// For use with Image Load/Store
ivec2 addrGen2D(ivec2 addr2D, uint viewportW)
{
    int addr1D = int(addrGen(addr2D, viewportW));
    return ivec2(addr1D % viewportW, addr1D / viewportW);
}

void GatherFragment(vec4 color)
{
#ifdef OIT_ALPHA_BLEND_ENABLED
	if (color.a < 0.01) {
        discard;
    }
#endif

    uint x = uint(gl_FragCoord.x);
    uint y = uint(gl_FragCoord.y);
	uint viewportW = SceneUBO.viewport.x;

	uint pixelIndex = addrGen(uvec2(x,y), viewportW);
	// Fragment index (in nodes buffer):
    uint index = MAX_NUM_NODES * pixelIndex;

	//float z = gl_FragCoord.z / gl_FragCoord.w;

	FragmentNode frag;
    frag.color = packUnorm4x8(color);
    frag.depth = gl_FragCoord.z;

	#ifdef OIT_ADDITIVE_BLEND_ENABLED

		frag.depth = -frag.depth;

	#endif

	memoryBarrierBuffer();

	uint numFragments = numFragmentsBuffer[pixelIndex];
    for (uint i = 0; i < numFragments; i++)
    {
        if (abs(frag.depth) < abs(nodes[index].depth))
        {
            FragmentNode temp = frag;
            frag = nodes[index];
            nodes[index] = temp;
        }
        index++;
    }

	if (numFragments < MAX_NUM_NODES) {
		numFragmentsBuffer[pixelIndex]++;
		nodes[index] = frag;
	} else {

#ifdef OIT_ADDITIVE_BLEND_ENABLED

		// Additive blend with last fragment
		vec4 colorDst = unpackUnorm4x8(nodes[index-1].color);
		vec4 colorSrc = color;

		vec4 colorOut;
		colorOut.rgb = colorDst.rgb + colorSrc.rgb;
        colorOut.a = colorDst.a + colorSrc.a;

		nodes[index-1].color = packUnorm4x8(colorOut);

#else

		// Alpha blend with last fragment
		vec4 colorDst = unpackUnorm4x8(nodes[index-1].color);
		vec4 colorSrc = color;

		vec4 colorOut;
		colorOut.rgb = colorDst.a * colorDst.rgb + (1.0 - colorDst.a) * colorSrc.a * colorSrc.rgb;
        colorOut.a = colorDst.a + (1.0 - colorDst.a) * colorSrc.a;

		nodes[index-1].color = packUnorm4x8(vec4(colorOut.rgb / colorOut.a, colorOut.a));

#endif

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