#version 430

#extension GL_EXT_texture_array : require
#extension GL_EXT_gpu_shader4 : require

#ifdef BINDLESS_ENABLED
#extension GL_ARB_shader_draw_parameters : require
#endif

#include "common.h"

uniform float u_parallaxScale;

#ifndef BINDLESS_ENABLED

layout(binding = 0) uniform sampler2D diffuseTex;
layout(binding = 1) uniform sampler2DArray lightmapTexArray;
layout(binding = 2) uniform sampler2D detailTex;
layout(binding = 3) uniform sampler2D normalTex;
layout(binding = 4) uniform sampler2D parallaxTex;
layout(binding = 5) uniform sampler2D specularTex;
layout(binding = 6) uniform sampler2DArray shadowmapTexArray;

#else

layout(binding = 1) uniform sampler2DArray lightmapTexArray;
layout(binding = 6) uniform sampler2DArray shadowmapTexArray;

#endif

in vec3 v_worldpos;
in vec3 v_normal;
in vec3 v_tangent;
in vec3 v_bitangent;
in vec2 v_diffusetexcoord;
in vec3 v_lightmaptexcoord;
in vec2 v_replacetexcoord;
in vec2 v_detailtexcoord;
in vec2 v_normaltexcoord;
in vec2 v_parallaxtexcoord;
in vec2 v_speculartexcoord;
in vec4 v_shadowcoord[3];

#ifdef BINDLESS_ENABLED

	#if defined(SKYBOX_ENABLED)
		flat in int v_drawid;
	#elif defined(DECAL_ENABLED)
		flat in int v_decalindex;
	#else
		flat in int v_texindex;
	#endif

#endif

#if defined(SKYBOX_ENABLED)

#elif defined(DECAL_ENABLED)
	flat in uvec4 v_styles;
#else
	flat in uvec4 v_styles;
#endif

#ifdef GBUFFER_ENABLED

	#if defined(DECAL_ENABLED)

		//Decal only affects diffuse and worldnorm channel
		
		layout(location = 0) out vec4 out_Diffuse;
		layout(location = 1) out vec4 out_WorldNorm;

	#else

		layout(location = 0) out vec4 out_Diffuse;
		layout(location = 1) out vec4 out_Lightmap;
		layout(location = 2) out vec4 out_WorldNorm;
		layout(location = 3) out vec4 out_Specular;
		layout(location = 4) out vec4 out_Additive;

	#endif

#else

	layout(location = 0) out vec4 out_Diffuse;

#endif

#ifdef BINDLESS_ENABLED

	texture_handle_t GetCurrentTextureHandle(int type)
	{
		#if defined(SKYBOX_ENABLED)
			return SkyboxSSBO[v_drawid];
		#elif defined(DECAL_ENABLED)
			return DecalSSBO[v_decalindex * TEXTURE_SSBO_MAX + type];
		#else
			return TextureSSBO[v_texindex * TEXTURE_SSBO_MAX + type];
		#endif
	}

#endif

#ifdef NORMALTEXTURE_ENABLED

vec3 NormalMapping(vec3 T, vec3 B, vec3 N, vec2 baseTexcoord)
{
#ifdef BINDLESS_ENABLED
	sampler2D normalTex = sampler2D(GetCurrentTextureHandle(TEXTURE_SSBO_NORMAL));
#endif

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

#ifdef PARALLAXTEXTURE_ENABLED

vec2 ParallaxMapping(vec3 T, vec3 B, vec3 N, vec3 viewDirWorld, vec2 baseTexcoord)
{
#ifdef BINDLESS_ENABLED
	sampler2D parallaxTex = sampler2D(GetCurrentTextureHandle(TEXTURE_SSBO_PARALLAX));
#endif

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

float ShadowCompareDepth(vec4 coord, vec2 off, float layer)
{
	vec4 newcoord = coord + vec4(off.x * SHADOW_TEXTURE_OFFSET, off.y * SHADOW_TEXTURE_OFFSET, 0.0, 0.0);

	float depth0 = texture(shadowmapTexArray, vec3(newcoord.xy / newcoord.w, layer) ).a;

	float depth1 = newcoord.z / newcoord.w;

	return depth0 < depth1 ? 0.0 : 1.0;
}

vec3 ShadowGetWorldPosition(vec4 coord, float layer)
{
	return texture(shadowmapTexArray, vec3(coord.xy / coord.w, layer) ).xyz;
}

float CalcShadowIntensityInternal(vec3 worldpos, int ilayer, float layer, float shadow_high, float shadow_medium, float shadow_low)
{
	float shadow_intensity = 1.0;

	vec3 scene = worldpos.xyz;
	
	vec3 caster = ShadowGetWorldPosition(v_shadowcoord[ilayer], layer);

	float dist = abs(caster.z - scene.z);
	float distlerp = (dist - SceneUBO.shadowFade.x) / SceneUBO.shadowFade.y;
	shadow_intensity *= 1.0 - clamp(distlerp, 0.0, 1.0);

	shadow_high = 1.0 - shadow_high;
	shadow_medium = 1.0 - shadow_medium;
	shadow_low = 1.0 - shadow_low;

	float shadow_final = shadow_high + shadow_medium + shadow_low;
	shadow_final = clamp(shadow_final, 0.0, 1.0) * shadow_intensity;

	return shadow_final;//0 = shadow, 1 = no shadow
}

float CalcShadowIntensity(vec3 worldpos, vec3 norm, vec3 lightdir)
{
	float shadow_final = 0.0;
	if(dot(norm.xyz, lightdir.xyz) < 0.0) 
	{
		float shadow_high = 1.0;

		#ifdef SHADOWMAP_HIGH_ENABLED
			shadow_high = 0.0;
			shadow_high += ShadowCompareDepth(v_shadowcoord[0], vec2(0.0,0.0), 0.0) * 0.25;
			shadow_high += ShadowCompareDepth(v_shadowcoord[0], vec2( -1.0, -1.0), 0.0) * 0.0625;
			shadow_high += ShadowCompareDepth(v_shadowcoord[0], vec2( -1.0, 0.0), 0.0) * 0.125;
			shadow_high += ShadowCompareDepth(v_shadowcoord[0], vec2( -1.0, 1.0), 0.0) * 0.0625;
			shadow_high += ShadowCompareDepth(v_shadowcoord[0], vec2( 0.0, -1.0), 0.0) * 0.125;
			shadow_high += ShadowCompareDepth(v_shadowcoord[0], vec2( 0.0, 1.0), 0.0) * 0.125;
			shadow_high += ShadowCompareDepth(v_shadowcoord[0], vec2( 1.0, -1.0), 0.0) * 0.0625;
			shadow_high += ShadowCompareDepth(v_shadowcoord[0], vec2( 1.0, 0.0), 0.0) * 0.125;
			shadow_high += ShadowCompareDepth(v_shadowcoord[0], vec2( 1.0, 1.0), 0.0) * 0.0625;
		#endif

		float shadow_medium = 1.0;

		#ifdef SHADOWMAP_MEDIUM_ENABLED
			shadow_medium = 0.0;
			shadow_medium += ShadowCompareDepth(v_shadowcoord[1], vec2(0.0,0.0), 1.0);
			shadow_medium += ShadowCompareDepth(v_shadowcoord[1], vec2(0.035,0.0), 1.0);
			shadow_medium += ShadowCompareDepth(v_shadowcoord[1], vec2(-0.035,0.0), 1.0);
			shadow_medium += ShadowCompareDepth(v_shadowcoord[1], vec2(0.0,0.035), 1.0);
			shadow_medium += ShadowCompareDepth(v_shadowcoord[1], vec2(0.0,-0.035), 1.0);
			shadow_medium *= 0.2;
		#endif

		float shadow_low = 1.0;

		#ifdef SHADOWMAP_LOW_ENABLED
			shadow_low = 0.0;
			shadow_low += ShadowCompareDepth(v_shadowcoord[2], vec2(0.0,0.0), 2.0);
			shadow_low += ShadowCompareDepth(v_shadowcoord[2], vec2(0.035,0.0), 2.0);
			shadow_low += ShadowCompareDepth(v_shadowcoord[2], vec2(-0.035,0.0), 2.0);
			shadow_low += ShadowCompareDepth(v_shadowcoord[2], vec2(0.0,0.035), 2.0);
			shadow_low += ShadowCompareDepth(v_shadowcoord[2], vec2(0.0,-0.035), 2.0);
			shadow_low *= 0.2;
		#endif

		if(false)
		{
			//nothing here
		}
		
	#ifdef SHADOWMAP_HIGH_ENABLED
		else if(shadow_high < 0.95)
		{
			shadow_final = CalcShadowIntensityInternal(worldpos, 0, 0.0, shadow_high, shadow_medium, shadow_low);
		}
	#endif

	#ifdef SHADOWMAP_MEDIUM_ENABLED
		else if(shadow_medium < 0.95)
		{
			shadow_final = CalcShadowIntensityInternal(worldpos, 1, 1.0, shadow_high, shadow_medium, shadow_low);
		}
	#endif

	#ifdef SHADOWMAP_LOW_ENABLED
		else if(shadow_low < 0.95)
		{
			shadow_final = CalcShadowIntensityInternal(worldpos, 2, 2.0, shadow_high, shadow_medium, shadow_low);
		}
	#endif
	}
	return shadow_final;
}

float CalcShadowIntensityLumFadeout(vec4 lightmapColor, float intensity)
{
	float lightmapLum = 0.299 * lightmapColor.x + 0.587 * lightmapColor.y + 0.114 * lightmapColor.z;
	float shadowLerp = (lightmapLum - SceneUBO.shadowFade.w) / (SceneUBO.shadowFade.z - SceneUBO.shadowFade.w);
	float shadowIntensity = intensity * clamp(shadowLerp, 0.0, 1.0);
	shadowIntensity *= SceneUBO.shadowColor.a;

	return shadowIntensity;
}

void main()
{
#ifndef SKYBOX_ENABLED
	ClipPlaneTest(v_worldpos.xyz, v_normal.xyz);
#endif

	vec2 baseTexcoord = vec2(0.0, 0.0);

#ifdef DIFFUSE_ENABLED

	#ifdef REPLACETEXTURE_ENABLED

		#ifdef BINDLESS_ENABLED
			sampler2D diffuseTex = sampler2D(GetCurrentTextureHandle(TEXTURE_SSBO_REPLACE));
		#endif

		baseTexcoord = vec2(v_diffusetexcoord.x * v_replacetexcoord.x, v_diffusetexcoord.y * v_replacetexcoord.y);

	#else

		#ifdef BINDLESS_ENABLED	
			sampler2D diffuseTex = sampler2D(GetCurrentTextureHandle(TEXTURE_SSBO_DIFFUSE));
		#endif

		baseTexcoord = v_diffusetexcoord.xy;

	#endif

	#ifdef PARALLAXTEXTURE_ENABLED

		vec3 viewDir = normalize(v_worldpos.xyz - SceneUBO.viewpos.xyz);

		vec4 diffuseColor = texture(diffuseTex, ParallaxMapping(v_tangent, v_bitangent, v_normal, viewDir, baseTexcoord));

	#else

		vec4 diffuseColor = texture(diffuseTex, baseTexcoord);

	#endif

	//dynamic gamma correction will corrupt the alpha-blend routine that decal rendering was using when there was pre-applied texgamma correction.
	#if !defined(DECAL_ENABLED)
		diffuseColor = TexGammaToLinear(diffuseColor);
	#else
		diffuseColor = TexGammaToLinear(diffuseColor);

		//so we have to shift the alpha a little bit up...
		#ifndef TRANSPARENT_ENABLED
			diffuseColor.a = pow(diffuseColor.a, SceneUBO.r_alpha_shift);
		#else
			//Don't do alpha adjustment for decals on transparent surfaces.
		#endif
	#endif

#else

	vec4 diffuseColor = vec4(1.0, 1.0, 1.0, 1.0);

#endif

#if defined(LIGHTMAP_ENABLED) && !defined(FULLBRIGHT_ENABLED)

	vec4 lightmapColor = vec4(0.0, 0.0, 0.0, 0.0);

	lightmapColor += texture(lightmapTexArray, vec3(v_lightmaptexcoord.x,v_lightmaptexcoord.y, v_lightmaptexcoord.z * 4.0 + 0.0) ) * SceneUBO.r_lightstylevalue[v_styles.x];
	lightmapColor += texture(lightmapTexArray, vec3(v_lightmaptexcoord.x,v_lightmaptexcoord.y, v_lightmaptexcoord.z * 4.0 + 1.0) ) * SceneUBO.r_lightstylevalue[v_styles.y];
	lightmapColor += texture(lightmapTexArray, vec3(v_lightmaptexcoord.x,v_lightmaptexcoord.y, v_lightmaptexcoord.z * 4.0 + 2.0) ) * SceneUBO.r_lightstylevalue[v_styles.z];
	lightmapColor += texture(lightmapTexArray, vec3(v_lightmaptexcoord.x,v_lightmaptexcoord.y, v_lightmaptexcoord.z * 4.0 + 3.0) ) * SceneUBO.r_lightstylevalue[v_styles.w];

	lightmapColor.rgb *= SceneUBO.r_lightscale;

	lightmapColor.a = 1.0;

	lightmapColor = LightGammaToLinear(lightmapColor);

#else

	vec4 lightmapColor = vec4(1.0, 1.0, 1.0, 1.0);

#endif

#ifdef COLOR_FILTER_ENABLED
	lightmapColor.x *= (SceneUBO.r_filtercolor.x * SceneUBO.r_filtercolor.a);
	lightmapColor.y *= (SceneUBO.r_filtercolor.y * SceneUBO.r_filtercolor.a);
	lightmapColor.z *= (SceneUBO.r_filtercolor.z * SceneUBO.r_filtercolor.a);

	lightmapColor.x = clamp(lightmapColor.x, 0.0, 1.0);
	lightmapColor.y = clamp(lightmapColor.y, 0.0, 1.0);
	lightmapColor.z = clamp(lightmapColor.z, 0.0, 1.0);
#endif

#ifdef NORMALTEXTURE_ENABLED

	vec3 vNormal = NormalMapping(v_tangent, v_bitangent, v_normal, baseTexcoord);

#else

	vec3 vNormal = normalize(v_normal.xyz);

#endif

#ifdef DETAILTEXTURE_ENABLED

	#ifdef BINDLESS_ENABLED
		sampler2D detailTex = sampler2D(GetCurrentTextureHandle(TEXTURE_SSBO_DETAIL));
	#endif

	vec2 detailTexCoord = vec2(baseTexcoord.x * v_detailtexcoord.x, baseTexcoord.y * v_detailtexcoord.y);
	vec4 detailColor = texture(detailTex, detailTexCoord);
    detailColor.xyz *= 2.0;
    detailColor.a = 1.0;

	detailColor = TexGammaToLinear(detailColor);

#else

	vec4 detailColor = vec4(1.0, 1.0, 1.0, 1.0);

#endif

#ifdef SHADOW_CASTER_ENABLED

	out_Diffuse.xyz = v_worldpos.xyz;
	out_Diffuse.w = gl_FragCoord.z;

#else

	#ifdef SHADOWMAP_ENABLED

		float shadowIntensity = CalcShadowIntensity(v_worldpos, vNormal, SceneUBO.shadowDirection.xyz);

	#endif

	#ifdef GBUFFER_ENABLED

		vec4 specularColor = vec4(0.0);

		vec2 vOctNormal = UnitVectorToOctahedron(vNormal);

		float flDistanceToFragment = distance(v_worldpos.xyz, SceneUBO.viewpos.xyz);

		#ifdef SPECULARTEXTURE_ENABLED
		
			#ifdef BINDLESS_ENABLED
				sampler2D specularTex = sampler2D(GetCurrentTextureHandle(TEXTURE_SSBO_SPECULAR));
			#endif

			vec2 specularTexCoord = vec2(baseTexcoord.x * v_speculartexcoord.x, baseTexcoord.y * v_speculartexcoord.y);
			specularColor.xy = texture(specularTex, specularTexCoord).xy;

		#endif

		#ifdef SHADOWMAP_ENABLED
			specularColor.z = shadowIntensity;
		#endif

		#ifdef DECAL_ENABLED

			out_Diffuse = diffuseColor * detailColor;
			out_WorldNorm = vec4(vOctNormal.x, vOctNormal.y, flDistanceToFragment, 0.0);

		#else

			out_Diffuse = diffuseColor * detailColor;
			out_Lightmap = lightmapColor;
			out_WorldNorm = vec4(vOctNormal.x, vOctNormal.y, flDistanceToFragment, 0.0);
			out_Specular = specularColor;
			out_Additive = vec4(0.0);

		#endif

	#else

	#ifdef SHADOWMAP_ENABLED

		shadowIntensity = CalcShadowIntensityLumFadeout(lightmapColor, shadowIntensity);
		lightmapColor.xyz *= (1.0 - shadowIntensity);

	#endif

		#ifdef TRANSPARENT_ENABLED
			vec4 color = CalcFog(diffuseColor * lightmapColor * detailColor * EntityUBO.color);
		#else
			vec4 color = CalcFog(diffuseColor * lightmapColor * detailColor);
		#endif

		#if defined(OIT_ALPHA_BLEND_ENABLED) || defined(OIT_ADDITIVE_BLEND_ENABLED) 

			GatherFragment(color);

		#endif

		out_Diffuse = color;

	#endif

#endif
}