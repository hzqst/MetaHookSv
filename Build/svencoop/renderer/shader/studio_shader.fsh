#version 410

#include "common.h"

uniform sampler2D diffuseTex;

#if defined(STUDIO_NF_CELSHADE_FACE) && !defined(HAIR_SHADOW_ENABLED) && defined(TEXTURE_VIEW_AVAILABLE)
uniform usampler2D stencilTex;
#endif

/* celshade */

uniform float r_celshade_midpoint;
uniform float r_celshade_softness;
uniform vec3 r_celshade_shadow_color;
uniform float r_rimlight_power;
uniform float r_rimlight_smooth;
uniform vec2 r_rimlight_smooth2;
uniform vec3 r_rimlight_color;
uniform float r_rimdark_power;
uniform float r_rimdark_smooth;
uniform vec2 r_rimdark_smooth2;
uniform vec3 r_rimdark_color;
uniform float r_hair_specular_exp;
uniform float r_hair_specular_exp2;
uniform vec3 r_hair_specular_intensity;
uniform vec3 r_hair_specular_intensity2;
uniform vec4 r_hair_specular_noise;
uniform vec4 r_hair_specular_noise2;
uniform vec2 r_hair_specular_smooth;
uniform float r_outline_dark;
uniform vec2 r_uvscale;

in vec3 v_worldpos;
in vec3 v_normal;
in vec4 v_color;
in vec2 v_texcoord;
in vec4 v_projpos;

layout(location = 0) out vec4 out_Diffuse;
layout(location = 1) out vec4 out_Lightmap;
layout(location = 2) out vec4 out_WorldNorm;
layout(location = 3) out vec4 out_Specular;
layout(location = 4) out vec4 out_Additive;

#ifdef STUDIO_NF_CELSHADE

vec3 ShiftTangent(vec3 T, vec3 N, float uvX, vec4 noise)
{
    return normalize(T + N * (sin(uvX * noise.x) * noise.z + sin(uvX * noise.y + 1.2) * noise.w));
}

float StrandSpecular(vec3 T, vec3 H, float exponent)
{
	float dotTH = dot(T, H);
    float sinTH = max(0.01, sqrt(1.0 - dotTH * dotTH));
    float dirAtten = smoothstep(-1.0, 0.0, dotTH);
    return dirAtten * pow(sinTH, exponent);
}

vec3 CelShade(vec3 normalWS, vec3 lightdirWS)
{
	vec3 N = normalWS;
    vec3 L = lightdirWS;
	vec3 V = normalize(v_worldpos.xyz - SceneUBO.viewpos.xyz);
	vec3 UP = vec3(0.0, 0.0, -1.0);
	vec3 BiT = cross(N, UP);
	vec3 T = cross(N, BiT);

	L.z *= 0.01;
	L = normalize(L);

    float NoL = dot(-N,L);

    // N dot L
    float litOrShadowArea = smoothstep(r_celshade_midpoint - r_celshade_softness, r_celshade_midpoint + r_celshade_softness, NoL);

#if defined(STUDIO_NF_CELSHADE_FACE) && !defined(HAIR_SHADOW_ENABLED) && defined(TEXTURE_VIEW_AVAILABLE)

	litOrShadowArea = mix(0.5, 1.0, litOrShadowArea);

	vec2 vBaseTexCoord = v_projpos.xy / v_projpos.w * 0.5 + 0.5;
	uint stencilValue = texture(stencilTex, vBaseTexCoord).r;

   	if(stencilValue == 6)
    {
		litOrShadowArea = 0.0;
	}

#endif

    vec3 litOrShadowColor = mix(r_celshade_shadow_color.xyz, vec3(1.0, 1.0, 1.0), litOrShadowArea);

	vec3 rimLightColor = vec3(0.0);
	vec3 rimDarkColor = vec3(0.0);
	vec3 specularColor = vec3(0.0);

#ifndef STUDIO_NF_CELSHADE_FACE
	//Rim light
	float lambertD = max(0, -NoL);
    float lambertF = max(0, NoL);
    float rim = 1.0 - clamp(dot(V, -N), 0.0, 1.0);

	float rimDot = pow(rim, r_rimlight_power);
	rimDot = lambertF * rimDot;
	float rimIntensity = smoothstep(0, r_rimlight_smooth, rimDot);
	rimLightColor = pow(rimIntensity, 5.0) * r_rimlight_color.xyz;

	rimLightColor.x = rimLightColor.x * smoothstep(r_rimlight_smooth2.x, r_rimlight_smooth2.y, v_color.x);
	rimLightColor.y = rimLightColor.y * smoothstep(r_rimlight_smooth2.x, r_rimlight_smooth2.y, v_color.y);
	rimLightColor.z = rimLightColor.z * smoothstep(r_rimlight_smooth2.x, r_rimlight_smooth2.y, v_color.z);

	rimDot = pow(rim, r_rimdark_power);
    rimDot = lambertD * rimDot;
	rimIntensity = smoothstep(0, r_rimdark_smooth, rimDot);
    rimDarkColor = pow(rimIntensity, 5.0) * r_rimdark_color.xyz;

	rimDarkColor.x = rimDarkColor.x * smoothstep(r_rimdark_smooth2.x, r_rimdark_smooth2.y, v_color.x);
	rimDarkColor.y = rimDarkColor.y * smoothstep(r_rimdark_smooth2.x, r_rimdark_smooth2.y, v_color.y);
	rimDarkColor.z = rimDarkColor.z * smoothstep(r_rimdark_smooth2.x, r_rimdark_smooth2.y, v_color.z);
#endif

#if defined(STUDIO_NF_CELSHADE_HAIR)
	
	vec2 texcoord = v_texcoord * r_uvscale;

	vec3 kajiyaSpecular = vec3(0.0);
    vec3 shiftedTangent1 = ShiftTangent(T, N, texcoord.x, r_hair_specular_noise);
    vec3 shiftedTangent2 = ShiftTangent(T, N, texcoord.x, r_hair_specular_noise2);
	
	vec3 V2 = SceneUBO.vpn.xyz;

    vec3 HforStrandSpecular = normalize(L + vec3(0.01, 0.0, 0.0) + V2);
    kajiyaSpecular += r_hair_specular_intensity * StrandSpecular(shiftedTangent1, HforStrandSpecular, r_hair_specular_exp);
    kajiyaSpecular += r_hair_specular_intensity2 * StrandSpecular(shiftedTangent2, HforStrandSpecular, r_hair_specular_exp2);

	kajiyaSpecular.x = kajiyaSpecular.x * smoothstep(r_hair_specular_smooth.x, r_hair_specular_smooth.y, v_color.x);
	kajiyaSpecular.y = kajiyaSpecular.y * smoothstep(r_hair_specular_smooth.x, r_hair_specular_smooth.y, v_color.y);
	kajiyaSpecular.z = kajiyaSpecular.z * smoothstep(r_hair_specular_smooth.x, r_hair_specular_smooth.y, v_color.z);

	specularColor += kajiyaSpecular;

#elif defined(STUDIO_NF_CELSHADE_HAIR_H)
	
	vec2 texcoord = v_texcoord * r_uvscale;

	vec3 kajiyaSpecular = vec3(0.0);
    vec3 shiftedTangent1 = ShiftTangent(BiT, N, texcoord.y, r_hair_specular_noise);
    vec3 shiftedTangent2 = ShiftTangent(BiT, N, texcoord.y, r_hair_specular_noise2);
	
	vec3 V2 = SceneUBO.vpn.xyz;

    vec3 HforStrandSpecular = normalize(-L + vec3(0.01, 0.0, 0.0) + V2);
    kajiyaSpecular += r_hair_specular_intensity * StrandSpecular(shiftedTangent1, HforStrandSpecular, r_hair_specular_exp);
    kajiyaSpecular += r_hair_specular_intensity2 * StrandSpecular(shiftedTangent2, HforStrandSpecular, r_hair_specular_exp2);

	kajiyaSpecular.x = kajiyaSpecular.x * smoothstep(r_hair_specular_smooth.x, r_hair_specular_smooth.y, v_color.x);
	kajiyaSpecular.y = kajiyaSpecular.y * smoothstep(r_hair_specular_smooth.x, r_hair_specular_smooth.y, v_color.y);
	kajiyaSpecular.z = kajiyaSpecular.z * smoothstep(r_hair_specular_smooth.x, r_hair_specular_smooth.y, v_color.z);

	specularColor += kajiyaSpecular;

#endif

	return v_color.xyz * litOrShadowColor + rimLightColor + rimDarkColor + specularColor;
}

#endif

void main(void)
{
#if !defined(SHADOW_CASTER_ENABLED) && !defined(HAIR_SHADOW_ENABLED)

	vec3 vNormal = normalize(v_normal.xyz);

	ClipPlaneTest(v_worldpos.xyz, vNormal);

	vec2 texcoord = v_texcoord * r_uvscale;

	vec4 diffuseColor = texture2D(diffuseTex, texcoord);

	diffuseColor = TexGammaToLinear(diffuseColor);

	//The light colors are already in linear space
	vec4 lightmapColor = v_color;

	#ifdef STUDIO_NF_CELSHADE
		lightmapColor.xyz = CelShade(vNormal, StudioUBO.r_plightvec.xyz);
	#endif

	#ifdef STUDIO_NF_MASKED
		if(diffuseColor.a < 0.5)
			discard;
	#endif

	#ifdef OUTLINE_ENABLED
		diffuseColor *= r_outline_dark;
	#endif

#endif

#if defined(SHADOW_CASTER_ENABLED)

	out_Diffuse = vec4(StudioUBO.entity_origin.x, StudioUBO.entity_origin.y, StudioUBO.entity_origin.z, gl_FragCoord.z);

#elif defined(HAIR_SHADOW_ENABLED)

	out_Diffuse = vec4(1.0, 1.0, 1.0, 1.0);

#else

	#ifdef GBUFFER_ENABLED

		#ifdef TRANSPARENT_ENABLED

			#ifdef STUDIO_NF_ADDITIVE

				out_Diffuse = diffuseColor * lightmapColor;

			#else

				out_Diffuse = diffuseColor;
				out_Lightmap = lightmapColor;

			#endif

		#else

			vec2 vOctNormal = UnitVectorToOctahedron(vNormal);

			float flDistanceToFragment = distance(v_worldpos.xyz, SceneUBO.viewpos.xyz);

			out_Diffuse = diffuseColor;
			out_Lightmap = lightmapColor;
			out_WorldNorm = vec4(vOctNormal.x, vOctNormal.y, flDistanceToFragment, 0.0);
			out_Specular = vec4(0.0);
			out_Additive = vec4(0.0);

		#endif

	#else

		vec4 color = CalcFog(diffuseColor * lightmapColor);

		#if defined(OIT_ALPHA_BLEND_ENABLED) || defined(OIT_ADDITIVE_BLEND_ENABLED) 
			
			GatherFragment(color);

		#endif

		out_Diffuse = color;

	#endif

#endif
}