#ifdef BINDLESS_ENABLED

#if defined(NV_BINDLESS_ENABLED)
	#extension GL_NV_bindless_texture : require
	#extension GL_NV_gpu_shader5 : require
#elif defined(INT64_BINDLESS_ENABLED)
	#extension GL_ARB_bindless_texture : require
	#extension GL_ARB_gpu_shader5 : require
	#extension GL_ARB_gpu_shader_int64 : require
#else
	#extension GL_ARB_bindless_texture : require
	#extension GL_ARB_gpu_shader5 : require
#endif

#endif

#if defined(OIT_BLEND_ENABLED)

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

#ifdef BINDLESS_ENABLED

#if defined(NV_BINDLESS_ENABLED)

	#define texture_handle_t uint64_t
	#define sampler_handle_t uint64_t

#elif defined(INT64_BINDLESS_ENABLED)

	#define texture_handle_t uint64_t
	#define sampler_handle_t uvec2

#else

	#define texture_handle_t uvec2
	#define sampler_handle_t uvec2

#endif

#endif

#define TEXTURE_SSBO_DIFFUSE 0
#define TEXTURE_SSBO_REPLACE 1
#define TEXTURE_SSBO_DETAIL 2
#define TEXTURE_SSBO_NORMAL 3
#define TEXTURE_SSBO_PARALLAX 4
#define TEXTURE_SSBO_SPECULAR 5
#define TEXTURE_SSBO_MAX 6

#define TEXTURE_SSBO_WATER_BASE 0
#define TEXTURE_SSBO_WATER_NORMAL 1
#define TEXTURE_SSBO_WATER_REFLECT 2
#define TEXTURE_SSBO_WATER_REFRACT 3
#define TEXTURE_SSBO_WATER_DEPTH 4

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

#define WSURF_DIFFUSE_TEXTURE		0
#define WSURF_REPLACE_TEXTURE		1
#define WSURF_DETAIL_TEXTURE		2
#define WSURF_NORMAL_TEXTURE		3
#define WSURF_PARALLAX_TEXTURE		4
#define WSURF_SPECULAR_TEXTURE		5
#define WSURF_MAX_TEXTURE			6

#define STUDIO_DIFFUSE_TEXTURE			0
#define STUDIO_REPLACE_TEXTURE			1
#define STUDIO_NORMAL_TEXTURE			2
#define STUDIO_PARALLAX_TEXTURE			3
#define STUDIO_SPECULAR_TEXTURE			4
#define STUDIO_MAX_TEXTURE				5

#define STUDIO_RESERVED_TEXTURE_STENCIL				6
#define STUDIO_RESERVED_TEXTURE_ANIMATED			7

#define STENCIL_MASK_ALL						0xFF
#define STENCIL_MASK_NONE						0
#define STENCIL_MASK_WORLD						1
#define STENCIL_MASK_NO_SHADOW					2
#define STENCIL_MASK_NO_BLOOM					8
#define STENCIL_MASK_HAS_OUTLINE				0x10
#define STENCIL_MASK_HAS_SHADOW					0x20
#define STENCIL_MASK_HAS_DECAL					0x40
#define STENCIL_MASK_HAS_FLATSHADE				0x80

#define STENCIL_MASK_HAS_FOG					STENCIL_MASK_WORLD

#define SPR_VP_PARALLEL_UPRIGHT 0
#define SPR_FACING_UPRIGHT 1
#define SPR_VP_PARALLEL 2
#define SPR_ORIENTED 3
#define SPR_VP_PARALLEL_ORIENTED 4

#define WATER_BIND_BASE_TEXTURE				0
#define WATER_BIND_NORMAL_TEXTURE			1
#define WATER_BIND_REFLECT_TEXTURE			2
#define WATER_BIND_REFLECT_DEPTH_TEXTURE	3
#define WATER_BIND_REFRACT_TEXTURE			4
#define WATER_BIND_REFRACT_DEPTH_TEXTURE	5

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

#define DSHADE_BINDING_POINT_DIFFUSE		0
#define DSHADE_BINDING_POINT_LIGHTMAP		1
#define DSHADE_BINDING_POINT_WORLDNORM		2
#define DSHADE_BINDING_POINT_SPECULAR		3
#define DSHADE_BINDING_POINT_DEPTH			4
#define DSHADE_BINDING_POINT_STENCIL		5
#define DSHADE_BINDING_POINT_CONE			6
#define DSHADE_BINDING_POINT_SHADOWMAP		7

#define DFINAL_BINDING_POINT_DIFFUSE			0
#define DFINAL_BINDING_POINT_LIGHTMAP			1
#define DFINAL_BINDING_POINT_WORLDNORM			2
#define DFINAL_BINDING_POINT_SPECULAR			3
#define DFINAL_BINDING_POINT_DEPTH				4
#define DFINAL_BINDING_POINT_STENCIL			5
#define DFINAL_BINDING_POINT_LINEARIZED_DEPTH	6

#define VERTEX_ATTRIBUTE_INDEX_POSITION 0
#define VERTEX_ATTRIBUTE_INDEX_NORMAL 1
#define VERTEX_ATTRIBUTE_INDEX_S_TANGENT 2
#define VERTEX_ATTRIBUTE_INDEX_T_TANGENT 3
#define VERTEX_ATTRIBUTE_INDEX_TEXCOORD 4
#define VERTEX_ATTRIBUTE_INDEX_LIGHTMAP_TEXCOORD 5
#define VERTEX_ATTRIBUTE_INDEX_REPLACETEXTURE_TEXCOORD 6
#define VERTEX_ATTRIBUTE_INDEX_DETAILTEXTURE_TEXCOORD 7
#define VERTEX_ATTRIBUTE_INDEX_NORMALTEXTURE_TEXCOORD 8
#define VERTEX_ATTRIBUTE_INDEX_PARALLAXTEXTURE_TEXCOORD 9
#define VERTEX_ATTRIBUTE_INDEX_SPECULARTEXTURE_TEXCOORD 10
#define VERTEX_ATTRIBUTE_INDEX_STYLES 11

struct camera_ubo_t {
	mat4 viewMatrix;
	mat4 projMatrix;
	mat4 invViewMatrix;
	mat4 invProjMatrix;
	vec4 viewport;
	vec4 frustum[4];
	vec4 viewpos;
	vec4 vpn;
	vec4 vright;
	vec4 vup;
	vec4 r_origin;
};

struct scene_ubo_t{

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
	float r_additive_shift;
	float r_lightscale;
	vec4 r_filtercolor;
	vec4 r_lightstylevalue[64];
};

struct dlight_ubo_t{
	vec4 origin_radius[256];
	vec4 color_minlight[256];
	uint active_dlights;
};

struct entity_ubo_t{
	mat4 entityMatrix;
	vec4 color;
	float scrollSpeed;
};

struct studio_ubo_t{
	float r_ambientlight;
	float r_shadelight;
	float r_scale;
	int r_numelight;
	vec4 r_plightvec;
	vec4 r_color;
	vec4 r_origin;
	vec4 entity_origin;
	vec4 r_elight_color[4];
	vec4 r_elight_origin[4];
	vec4 r_elight_radius;
	mat3x4 bonematrix[128];
	uvec4 r_clipbone;
};

//Scene level

layout (std140, binding = BINDING_POINT_SCENE_UBO) uniform SceneBlock
{
   scene_ubo_t SceneUBO;
};

layout(std140, binding = BINDING_POINT_CAMERA_UBO) uniform CameraBlock
{
	camera_ubo_t CameraUBO;
};

layout (std140, binding = BINDING_POINT_DLIGHT_UBO) uniform DLightBlock
{
   dlight_ubo_t DLightUBO;
};

#if defined(BINDLESS_ENABLED)

	layout (std430, binding = BINDING_POINT_DECAL_SSBO) coherent buffer DecalBlock
	{
		texture_handle_t DecalSSBO[];
	};

	layout (std430, binding = BINDING_POINT_SKYBOX_SSBO) coherent buffer SkyboxBlock
	{
		texture_handle_t SkyboxSSBO[];
	};

#endif

//Entity level

layout (std140, binding = BINDING_POINT_ENTITY_UBO) uniform EntityBlock
{
	entity_ubo_t EntityUBO;
};

#if defined(BINDLESS_ENABLED)

layout (std430, binding = BINDING_POINT_TEXTURE_SSBO) coherent buffer TextureBlock
{
	texture_handle_t TextureSSBO[];
};

#endif

layout (std140, binding = BINDING_POINT_STUDIO_UBO) uniform StudioBlock
{
	studio_ubo_t StudioUBO;
};

#if defined(OIT_BLEND_ENABLED)

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

#endif

#if defined(OIT_BLEND_ENABLED) && defined(IS_FRAGMENT_SHADER)

	void GatherFragment(inout vec4 color)
	{
		float x = gl_FragCoord.x;
		float y = gl_FragCoord.y;
		float viewportW = CameraUBO.viewport.x;
		uint linkedListSize = uint(CameraUBO.viewport.z);

		uint pixelIndex = uint(viewportW*y + x);

		FragmentNode frag;
		frag.color = packUnorm4x8(color);
		#ifdef ADDITIVE_BLEND_ENABLED
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

#elif defined(IS_FRAGMENT_SHADER)

	void GatherFragment(inout vec4 color)
	{

	}

#endif

//Pack vec3 into vec2

vec2 UnitVectorToHemiOctahedron(vec3 dir) {

	dir.y = max(dir.y, 0.0001);
	dir.xz /= dot(abs(dir), vec3(1.0));

	return clamp(0.5 * vec2(dir.x + dir.z, dir.x - dir.z) + 0.5, 0.0, 1.0);

}

//Unpack vec2 to vec3
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

vec3 GenerateViewPositionFromDepth(vec2 texCoord, float depth) {
	vec4 ndc = vec4(texCoord * 2.0 - 1.0, depth, 1.0);
	vec4 inversed = CameraUBO.invProjMatrix * ndc;
	return inversed.xyz / inversed.w;
}

vec3 GenerateWorldPositionFromDepth(vec2 texCoord, float depth)
{
	vec4 clipSpaceLocation;
	clipSpaceLocation.xy = texCoord * 2.0 - 1.0;
	clipSpaceLocation.z = depth * 2.0 - 1.0;
	clipSpaceLocation.w = 1.0;
	vec4 inversed = CameraUBO.invViewMatrix * CameraUBO.invProjMatrix * clipSpaceLocation;
	return inversed.xyz / inversed.w;
}

vec2 GenerateProjectedPosition(vec3 pos) {
	vec4 samplePosition = CameraUBO.projMatrix * vec4(pos, 1.0);
	samplePosition.xy = (samplePosition.xy / samplePosition.w) * 0.5 + 0.5;
	return samplePosition.xy;
}

//Color Space Conversion

vec4 GammaToLinear(vec4 color)
{
	color.rgb = pow(color.rgb, vec3(SceneUBO.v_gamma));
	return color;
}

vec3 GammaToLinear3(vec3 color)
{
	color = pow(color, vec3(SceneUBO.v_gamma));
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

vec3 TexGammaToGamma3(vec3 color)
{
	color = pow(color, vec3(SceneUBO.v_texgamma * SceneUBO.r_g));//r_g = 1.0 / v_gamma
	return color;
}

float TexGammaToGamma1(float color)
{
	color = pow(color, SceneUBO.v_texgamma * SceneUBO.r_g);//r_g = 1.0 / v_gamma
	return color;
}

vec4 LinearToGamma(vec4 color)
{
	color.rgb = pow(color.rgb, vec3(SceneUBO.r_g));//r_g = 1.0 / v_gamma
	return color;
}

vec3 LinearToGamma3(vec3 color)
{
	color.rgb = pow(color.rgb, vec3(SceneUBO.r_g));//r_g = 1.0 / v_gamma
	return color;
}

float LightGammaToGammaOverBrightInternal(float color)
{
	float fv = pow(color, SceneUBO.v_lightgamma);

	fv = fv * max(SceneUBO.v_brightness, 1.0);

	//if (fv > SceneUBO.r_g3)
	float fv1 = 0.125 + ((fv - SceneUBO.r_g3) / (1.0 - SceneUBO.r_g3)) * 0.875;
	//else 
	float fv2 = (fv / SceneUBO.r_g3) * 0.125;

	if (fv > SceneUBO.r_g3)
		fv = fv1;
	else
		fv = fv2;

	//fv = mix(fv1, fv2, step(fv, SceneUBO.r_g3));

	fv = pow(fv, SceneUBO.r_g);

	return fv;
}

//This was being applied for R_BuildLightMap and R_StudioLighting
float LightGammaToGammaInternal(float color)
{
	return clamp(LightGammaToGammaOverBrightInternal(color), 0.0, 1.0);
}

vec4 LightGammaToGamma(vec4 color)
{
	return vec4(LightGammaToGammaInternal(color.r), LightGammaToGammaInternal(color.g), LightGammaToGammaInternal(color.b), color.a);
}

float LightGammaToLinearOverBrightInternal(float color)
{
	float fv = pow(color, SceneUBO.v_lightgamma);

	fv = fv * max(SceneUBO.v_brightness, 1.0);

	/*
	fv = mix(
		0.125 + ((fv - SceneUBO.r_g3) / (1.0 - SceneUBO.r_g3)) * 0.875,
		(fv / SceneUBO.r_g3) * 0.125,
		step(fv, SceneUBO.r_g3));
	*/

	if (fv > SceneUBO.r_g3)
		fv = 0.125 + ((fv - SceneUBO.r_g3) / (1.0 - SceneUBO.r_g3)) * 0.875;
	else
		fv = (fv / SceneUBO.r_g3) * 0.125;

	return fv;
}

float LightGammaToLinearInternal(float color)
{
	return clamp(LightGammaToLinearOverBrightInternal(color), 0.0, 1.0);
}

vec4 LightGammaToLinear(vec4 color)
{
	return vec4(LightGammaToLinearInternal(color.r), LightGammaToLinearInternal(color.g), LightGammaToLinearInternal(color.b), color.a);
}

//Input: TexGammaSpace
//Output: LinearSpace or GammaSpace depending on GAMMA_BLEND_ENABLED
vec4 ProcessDiffuseColor(vec4 baseColor)
{
	#if defined(GAMMA_BLEND_ENABLED)

		baseColor = TexGammaToGamma(baseColor);

		//Clamp this up to 1.0 just in case overflow
		baseColor.r = clamp(baseColor.r, 0.0, 1.0);
		baseColor.g = clamp(baseColor.g, 0.0, 1.0);
		baseColor.b = clamp(baseColor.b, 0.0, 1.0);

	#else

		baseColor = TexGammaToLinear(baseColor);

	#endif

	return baseColor;
}

//Input: LightGammaSpace
//Output: LinearSpace or GammaSpace depending on GAMMA_BLEND_ENABLED
vec4 ProcessLightmapColor(vec4 lightmapColor)
{
	#if defined(GAMMA_BLEND_ENABLED)

		lightmapColor = LightGammaToGamma(lightmapColor);

		//Clamp this up to 1.0 just in case overflow
		lightmapColor.r = clamp(lightmapColor.r, 0.0, 1.0);
		lightmapColor.g = clamp(lightmapColor.g, 0.0, 1.0);
		lightmapColor.b = clamp(lightmapColor.b, 0.0, 1.0);
	#else

		lightmapColor = LightGammaToLinear(lightmapColor);

	#endif

	return lightmapColor;
}

//Input: GammaSpace
//Output: LinearSpace or GammaSpace depending on GAMMA_BLEND_ENABLED
vec4 ProcessOtherGammaColor(vec4 color)
{
	#if defined(GAMMA_BLEND_ENABLED)


	#else

		color = GammaToLinear(color);

	#endif

	return color;
}

vec3 ProcessOtherGammaColor3(vec3 color)
{
	#if defined(GAMMA_BLEND_ENABLED)

		

	#else

		color = GammaToLinear3(color);

	#endif

	return color;
}

//Input: LinearSpace
//Output: LinearSpace or GammaSpace depending on GAMMA_BLEND_ENABLED
vec4 ProcessOtherLinearColor(vec4 color)
{
	#if defined(GAMMA_BLEND_ENABLED)

		color = LinearToGamma(color);

	#else

		

	#endif

	return color;
}

#if defined(IS_FRAGMENT_SHADER)

	void ClipPlaneTest(vec3 worldpos, vec3 normal)
	{
		#if defined(CLIP_WATER_ENABLED)

			vec4 clipVec = vec4(worldpos.xyz, 1.0);
			vec4 clipPlane = SceneUBO.clipPlane;

			if(dot(clipVec, clipPlane) < 0)
				discard;

			clipPlane.w += 32.0;
			if(dot(clipVec, clipPlane) < 0 && dot(normalize(normal.xyz), -clipPlane.xyz) > 0.866)
				discard;

		#elif defined(CLIP_ENABLED)

			vec4 clipVec = vec4(worldpos.xyz - SceneUBO.clipPlane.xyz * 4.0, -1.0);
			vec4 clipPlane = SceneUBO.clipPlane;

			if(dot(clipVec, clipPlane) > 0)
				discard;

		#endif
	}

#else

	void ClipPlaneTest(vec3 worldpos, vec3 normal)
	{

	}

#endif

#if defined(LINEAR_FOG_ENABLED) && defined(IS_FRAGMENT_SHADER)

	vec4 CalcFogWithDistance(vec4 color, float z)
	{
		float fogFactor = ( SceneUBO.fogEnd - z ) / ( SceneUBO.fogEnd - SceneUBO.fogStart );

		fogFactor = clamp(fogFactor, 0, 1);

		vec3 fogColor = SceneUBO.fogColor.xyz;

		fogColor = ProcessOtherGammaColor3(fogColor);

#if defined(ADDITIVE_BLEND_ENABLED)
		fogColor = vec3(0, 0, 0);
#endif

		color.xyz = mix(fogColor, color.xyz, fogFactor );

		return color;
	}

	vec4 CalcFog(vec4 color)
	{
		return CalcFogWithDistance(color, gl_FragCoord.z / gl_FragCoord.w);
	}

#elif defined(EXP_FOG_ENABLED) && defined(IS_FRAGMENT_SHADER)

	vec4 CalcFogWithDistance(vec4 color, float z)
	{
		float f = SceneUBO.fogDensity * z;

		float fogFactor = exp( -f );

		fogFactor = clamp(fogFactor, 0.0, 1.0);

		vec3 fogColor = SceneUBO.fogColor.xyz;

		fogColor = ProcessOtherGammaColor3(fogColor);

#if defined(ADDITIVE_BLEND_ENABLED)
		fogColor = vec3(0, 0, 0);
#endif

		color.xyz = mix(fogColor, color.xyz, fogFactor );

		return color;
	}

	vec4 CalcFog(vec4 color)
	{
		return CalcFogWithDistance(color, gl_FragCoord.z / gl_FragCoord.w);
	}

#elif defined(EXP2_FOG_ENABLED) && defined(IS_FRAGMENT_SHADER)

	vec4 CalcFogWithDistance(vec4 color, float z)
	{
		float f = SceneUBO.fogDensity * z;
		float fogFactor = exp(-f*f);
		fogFactor = clamp(fogFactor, 0.0, 1.0);

		vec3 fogColor = SceneUBO.fogColor.xyz;

		fogColor = ProcessOtherGammaColor3(fogColor);

#if defined(ADDITIVE_BLEND_ENABLED)
		fogColor = vec3(0, 0, 0);
#endif

		color.xyz = mix(fogColor, color.xyz, fogFactor);

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

	vec4 CalcFogWithDistance(vec4 color, float z)
	{
		return color;
	}

#endif