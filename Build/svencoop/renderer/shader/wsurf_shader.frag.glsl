#version 430

#extension GL_EXT_texture_array : require
//#extension GL_EXT_gpu_shader4 : require

#include "common.h"

uniform float u_parallaxScale;

layout(binding = WSURF_BIND_DIFFUSE_TEXTURE) uniform sampler2D diffuseTex;
layout(binding = WSURF_BIND_DETAIL_TEXTURE) uniform sampler2D detailTex;
layout(binding = WSURF_BIND_NORMAL_TEXTURE) uniform sampler2D normalTex;
layout(binding = WSURF_BIND_PARALLAX_TEXTURE) uniform sampler2D parallaxTex;
layout(binding = WSURF_BIND_SPECULAR_TEXTURE) uniform sampler2D specularTex;
layout(binding = WSURF_BIND_LIGHTMAP_TEXTURE_0) uniform sampler2DArray lightmapTexArray_0;
layout(binding = WSURF_BIND_LIGHTMAP_TEXTURE_1) uniform sampler2DArray lightmapTexArray_1;
layout(binding = WSURF_BIND_LIGHTMAP_TEXTURE_2) uniform sampler2DArray lightmapTexArray_2;
layout(binding = WSURF_BIND_LIGHTMAP_TEXTURE_3) uniform sampler2DArray lightmapTexArray_3;

in vec3 v_worldpos;
in vec3 v_normal;
in vec3 v_tangent;
in vec3 v_bitangent;
in vec2 v_diffusetexcoord;
in vec3 v_lightmaptexcoord;
in vec2 v_detailtexcoord;
in vec2 v_normaltexcoord;
in vec2 v_parallaxtexcoord;
in vec2 v_speculartexcoord;
in vec4 v_shadowcoord[3];
in vec4 v_projpos;

#if defined(SKYBOX_ENABLED)

#elif defined(DECAL_ENABLED)
	flat in uvec4 v_styles;
#else
	flat in uvec4 v_styles;
#endif

float ConvertStyleToLightStyle(uint style)
{
	return SceneUBO.r_lightstylevalue[style / 4][style % 4];
}

#if defined(LEGACY_DLIGHT_ENABLED)

vec4 R_AddLegacyDynamicLight(vec4 color)
{
	for(uint i = 0;i < DLightUBO.active_dlights; ++i)
	{
		vec3 origin = DLightUBO.origin_radius[i].xyz;
		float radius = DLightUBO.origin_radius[i].w;
		vec3 delta = origin - v_worldpos.xyz;
		float surface_dist = dot(delta, v_normal);
		float effective_radius = radius - abs(surface_dist);
		float dist = length(delta);

		vec3 lightcolor = DLightUBO.color_minlight[i].xyz;
		//float minlight = DLightUBO.color_minlight[i].w; //unused
		
		color.xyz += clamp((effective_radius - dist) / 256.0, 0.0, 1.0) * lightcolor.xyz;
	}

	return color;
}

#endif

#if defined(GBUFFER_ENABLED)

	#if defined(DECAL_ENABLED)

		//Decal only affects diffuse, worldnorm, specular channel
		
		layout(location = 0) out vec4 out_Diffuse;
		layout(location = 1) out vec4 out_WorldNorm;
		layout(location = 2) out vec4 out_Specular;

	#else

		layout(location = 0) out vec4 out_Diffuse;
		layout(location = 1) out vec4 out_Lightmap;
		layout(location = 2) out vec4 out_WorldNorm;
		layout(location = 3) out vec4 out_Specular;

	#endif

#else

	layout(location = 0) out vec4 out_Diffuse;

#endif

#if defined(NORMALTEXTURE_ENABLED)

vec3 NormalMapping(vec3 T, vec3 B, vec3 N, vec2 baseTexcoord)
{
    // Create TBN matrix. from tangent to world space
    mat3 TBN = mat3(normalize(T), normalize(B), normalize(N));

	vec2 vNormTexcoord = vec2(baseTexcoord.x * v_normaltexcoord.x, baseTexcoord.y * v_normaltexcoord.y);

    // Sample tangent space normal vector from normal map and remap it from [0, 1] to [-1, 1] range.
    vec3 n = texture(normalTex, vNormTexcoord).xyz;
    n = normalize(n * 2.0 - 1.0);

    // Multiple normal by the TBN matrix to transform the normal from tangent space to world space.
    n = normalize(TBN * n);

    return n;
}

#endif

#if defined(PARALLAXTEXTURE_ENABLED)

vec2 ParallaxMapping(vec3 T, vec3 B, vec3 N, vec3 viewDirWorld, vec2 baseTexcoord)
{
    // Create TBN matrix.
    mat3 TBN = mat3(normalize(T), normalize(B), normalize(N));

	//Multiple viewDir by the TBN matrix to transform the normal from tangent space to world space.
	vec3 viewDir = normalize(transpose(TBN) * viewDirWorld);

	const float minLayers = 20;
	const float maxLayers = 40;	
	float numLayers = mix(maxLayers, minLayers, abs(dot(vec3(0.0f, 0.0f, 1.0f), viewDir)));

    float layerDepth = 1.0 / numLayers;

    float currentLayerDepth = 0.0;

    vec2 p = viewDir.xy / viewDir.z * u_parallaxScale;

    vec2 deltaTexCoords = p / numLayers;

	vec2 mainTexCoods = baseTexcoord;

    vec2 currentTexCoords = mainTexCoods;

	vec2 ddx = dFdx(mainTexCoods);
	vec2 ddy = dFdy(mainTexCoods);

    float currentDepthMapValue = 1.0 - textureGrad(parallaxTex, vec2(currentTexCoords.x * v_parallaxtexcoord.x, currentTexCoords.y * v_parallaxtexcoord.y), ddx, ddy ).r;

    while(currentLayerDepth < currentDepthMapValue)
    {
        currentTexCoords -= deltaTexCoords;
        currentDepthMapValue = 1.0 - textureGrad(parallaxTex, vec2(currentTexCoords.x * v_parallaxtexcoord.x, currentTexCoords.y * v_parallaxtexcoord.y), ddx, ddy ).r;
        currentLayerDepth += layerDepth;
    }

	vec2 prevTexCoords = currentTexCoords + deltaTexCoords;

	// get depth after and before collision for linear interpolation
	float afterDepth  = currentDepthMapValue - currentLayerDepth;
	float beforeDepth = 1.0 - textureGrad(parallaxTex, vec2(prevTexCoords.x * v_parallaxtexcoord.x, prevTexCoords.y * v_parallaxtexcoord.y), ddx, ddy ).r - currentLayerDepth + layerDepth;
	 
	// interpolation of texture coordinates
	float weight = afterDepth / (afterDepth - beforeDepth);
	vec2 finalTexCoords = mix(currentTexCoords, prevTexCoords, weight);

	return finalTexCoords;
}

#endif

void main()
{
	ClipPlaneTest(v_worldpos.xyz, v_normal.xyz);
	
	vec2 screenTexCoord = v_projpos.xy / v_projpos.w * 0.5 + 0.5;

	vec2 baseTexcoord = vec2(0.0, 0.0);

#if defined(DIFFUSE_ENABLED)

	baseTexcoord = v_diffusetexcoord.xy;

	#if defined(PARALLAXTEXTURE_ENABLED)

		vec3 viewDir = normalize(v_worldpos.xyz - GetCameraViewPos(0));

		vec4 diffuseColor = texture(diffuseTex, ParallaxMapping(v_tangent, v_bitangent, v_normal, viewDir, baseTexcoord));

	#else

		vec4 diffuseColor = texture(diffuseTex, baseTexcoord);

	#endif
	
	#if defined(DECAL_ENABLED)
		
		//always
		if (diffuseColor.a < 0.001)
		   discard;

	#elif defined(ALPHA_SOLID_ENABLED)
		
		if(diffuseColor.a < SceneUBO.r_alphamin)
			discard;

	#endif

	diffuseColor = ProcessDiffuseColor(diffuseColor);

#else

	vec4 diffuseColor = vec4(1.0, 1.0, 1.0, 1.0);

#endif

#if defined(FULLBRIGHT_ENABLED)
	
	vec4 lightmapColor = vec4(1.0, 1.0, 1.0, 1.0);

#elif defined(LIGHTMAP_ENABLED)

	vec4 lightmapColor = vec4(0.0, 0.0, 0.0, 0.0);

#if defined(LIGHTMAP_INDEX_0_ENABLED)
	lightmapColor += texture(lightmapTexArray_0, v_lightmaptexcoord ) * ConvertStyleToLightStyle(v_styles.x);
#endif

#if defined(LIGHTMAP_INDEX_1_ENABLED)
	lightmapColor += texture(lightmapTexArray_1, v_lightmaptexcoord ) * ConvertStyleToLightStyle(v_styles.y);
#endif

#if defined(LIGHTMAP_INDEX_2_ENABLED)
	lightmapColor += texture(lightmapTexArray_2, v_lightmaptexcoord ) * ConvertStyleToLightStyle(v_styles.z);
#endif

#if defined(LIGHTMAP_INDEX_3_ENABLED)
	lightmapColor += texture(lightmapTexArray_3, v_lightmaptexcoord ) * ConvertStyleToLightStyle(v_styles.w);
#endif

	lightmapColor.x = pow(lightmapColor.x, SceneUBO.r_lightmap_pow);
	lightmapColor.y = pow(lightmapColor.y, SceneUBO.r_lightmap_pow);
	lightmapColor.z = pow(lightmapColor.z, SceneUBO.r_lightmap_pow);
	
	lightmapColor *= SceneUBO.r_lightmap_scale;

#if defined(LEGACY_DLIGHT_ENABLED)

	lightmapColor = R_AddLegacyDynamicLight(lightmapColor);

#endif

	lightmapColor *= SceneUBO.r_lightscale;

	lightmapColor.a = 1.0;

	lightmapColor = ProcessLightmapColor(lightmapColor);

#else

	vec4 lightmapColor = vec4(0.0, 0.0, 0.0, 1.0);

#endif

#if defined(COLOR_FILTER_ENABLED)
	lightmapColor.x *= (SceneUBO.r_filtercolor.x * SceneUBO.r_filtercolor.a);
	lightmapColor.y *= (SceneUBO.r_filtercolor.y * SceneUBO.r_filtercolor.a);
	lightmapColor.z *= (SceneUBO.r_filtercolor.z * SceneUBO.r_filtercolor.a);

	lightmapColor.x = clamp(lightmapColor.x, 0.0, 1.0);
	lightmapColor.y = clamp(lightmapColor.y, 0.0, 1.0);
	lightmapColor.z = clamp(lightmapColor.z, 0.0, 1.0);
#endif

#if defined(NORMALTEXTURE_ENABLED)

	vec3 vNormal = NormalMapping(v_tangent, v_bitangent, v_normal, baseTexcoord);

#else

	vec3 vNormal = normalize(v_normal.xyz);

#endif

#if defined(DETAILTEXTURE_ENABLED)

	vec2 detailTexCoord = vec2(baseTexcoord.x * v_detailtexcoord.x, baseTexcoord.y * v_detailtexcoord.y);
	vec4 detailColor = texture(detailTex, detailTexCoord);
    detailColor.xyz *= 2.0;
    detailColor.a = 1.0;

	detailColor = ProcessDiffuseColor(detailColor);

#else

	vec4 detailColor = vec4(1.0, 1.0, 1.0, 1.0);

#endif

vec4 entityColor = ProcessOtherGammaColor(EntityUBO.color);

float flDistanceToFragment = distance(v_worldpos.xyz, GetCameraViewPos(0));

#if defined(SHADOW_CASTER_ENABLED)

	out_Diffuse.xyz = v_worldpos.xyz;
	out_Diffuse.w = gl_FragCoord.z;

#else

	#if defined(GBUFFER_ENABLED)

		vec4 specularColor = vec4(0.0);

		vec2 vOctNormal = UnitVectorToOctahedron(vNormal);

		flDistanceToFragment = distance(v_worldpos.xyz, GetCameraViewPos(0));

		#if defined(SPECULARTEXTURE_ENABLED)
		
			vec2 specularTexCoord = vec2(baseTexcoord.x * v_speculartexcoord.x, baseTexcoord.y * v_speculartexcoord.y);
			specularColor.xy = texture(specularTex, specularTexCoord).xy;

		#endif

		#if defined(DECAL_ENABLED)

			out_Diffuse = ProcessLinearBlendShift(diffuseColor * detailColor * entityColor);
			out_WorldNorm = vec4(vOctNormal.x, vOctNormal.y, flDistanceToFragment, out_Diffuse.a);
			out_Specular = specularColor;

		#else

			out_Diffuse = diffuseColor * detailColor * entityColor;
			out_Lightmap = lightmapColor;
			out_WorldNorm = vec4(vOctNormal.x, vOctNormal.y, flDistanceToFragment, out_Diffuse.a);
			out_Specular = specularColor;

		#endif

	#else

		vec4 color = CalcFog(
			ProcessLinearBlendShift(diffuseColor * lightmapColor * detailColor * entityColor), 
			flDistanceToFragment
		);

		GatherFragment(color);

		out_Diffuse = color;

	#endif

#endif
}