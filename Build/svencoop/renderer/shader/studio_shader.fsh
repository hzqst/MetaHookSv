#version 430

#include "common.h"

#ifndef BINDLESS_ENABLED
layout(binding = 0) uniform sampler2D diffuseTex;
#endif

/* celshade */

uniform float r_celshade_midpoint;
uniform float r_celshade_softness;
uniform vec3 r_celshade_shadow_color;
uniform float r_rimlight_power;
uniform float r_rimlight_smooth;
uniform vec3 r_rimlight_color;
uniform float r_rimdark_power;
uniform float r_rimdark_smooth;
uniform vec3 r_rimdark_color;
uniform float r_outline_dark;

in vec3 v_worldpos;
in vec3 v_normal;
in vec4 v_color;
in vec2 v_texcoord;

layout(location = 0) out vec4 out_Diffuse;
layout(location = 1) out vec4 out_Lightmap;
layout(location = 2) out vec4 out_WorldNorm;
layout(location = 3) out vec4 out_Specular;
layout(location = 4) out vec4 out_Additive;

#ifdef STUDIO_NF_CELSHADE

vec3 CelShade(vec3 normalWS, vec3 lightdirWS)
{
	vec3 N = normalWS;
	vec3 V = normalize(v_worldpos.xyz - SceneUBO.viewpos.xyz);
    vec3 L = lightdirWS;

	L.z *= 0.01;
	L = normalize(L);

    float NoL = dot(-N,L);

    // N dot L
    float litOrShadowArea = smoothstep(r_celshade_midpoint - r_celshade_softness, r_celshade_midpoint + r_celshade_softness, NoL);

#ifdef STUDIO_NF_CELSHADE_FACE
	litOrShadowArea = mix(0.5, 1.0, litOrShadowArea);
#endif

    vec3 litOrShadowColor = mix(r_celshade_shadow_color.xyz, vec3(1.0, 1.0, 1.0), litOrShadowArea);

	vec3 rimColor = vec3(0.0);
	vec3 rimDarkColor = vec3(0.0);

#ifndef STUDIO_NF_CELSHADE_FACE
	//Rim light
	float lambertD = max(0, -NoL);
    float lambertF = max(0, NoL);
    float rim = 1.0 - clamp(dot(V, -N), 0.0, 1.0);

	float rimDot = pow(rim, r_rimlight_power);
	rimDot = lambertF * rimDot;
	float rimIntensity = smoothstep(0, r_rimlight_smooth, rimDot);
	rimColor = pow(rimIntensity, 5.0) * r_rimlight_color;

	rimDot = pow(rim, r_rimdark_power);
    rimDot = lambertD * rimDot;
	rimIntensity = smoothstep(0, r_rimdark_smooth, rimDot);
    rimDarkColor = pow(rimIntensity, 5.0) * r_rimdark_color;
#endif

	return v_color.xyz * litOrShadowColor + rimColor + rimDarkColor;
}

#endif

void main(void)
{
#ifndef SHADOW_CASTER_ENABLED

	#ifdef STUDIO_NF_CHROME

		#ifdef GLOW_SHELL_ENABLED
			ivec2 textureSize2d = textureSize(diffuseTex, 0);
			vec2 texcoord_scale = vec2(1.0 / float(textureSize2d.x), 1.0 / float(textureSize2d.y));
			vec2 texcoord = v_texcoord * texcoord_scale * vec2(1.0 / 32.0f, 1.0 / 32.0f);
		#else
			vec2 texcoord_scale = vec2(1.0 / 2048.0, 1.0 / 2048.0);
			vec2 texcoord = v_texcoord * texcoord_scale;
		#endif

	#else

		ivec2 textureSize2d = textureSize(diffuseTex, 0);
		vec2 texcoord_scale = vec2(1.0 / float(textureSize2d.x), 1.0 / float(textureSize2d.y));
		vec2 texcoord = v_texcoord * texcoord_scale;

	#endif

	vec4 diffuseColor = texture2D(diffuseTex, texcoord);

	vec3 vNormal = normalize(v_normal.xyz);

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

#ifdef CLIP_ENABLED
	vec4 clipVec = vec4(v_worldpos.xyz, 1);
	vec4 clipPlane = SceneUBO.clipPlane;
	if(dot(clipVec, clipPlane) < 0)
		discard;

	clipPlane.w += 32.0;
	if(dot(clipVec, clipPlane) < 0 && dot(v_normal.xyz, -clipPlane.xyz) > 0.866)
		discard;
#endif

#ifdef SHADOW_CASTER_ENABLED

	out_Diffuse = vec4(StudioUBO.entity_origin.x, StudioUBO.entity_origin.y, StudioUBO.entity_origin.z, gl_FragCoord.z);

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