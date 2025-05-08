#version 430

#include "common.h"

layout (location = 0) uniform ivec2 width_height;
layout (location = 1) uniform vec4 up_down_left_right;
layout (location = 2) uniform vec4 in_color;
layout (location = 3) uniform vec3 in_origin;
layout (location = 4) uniform vec3 in_angles;
layout (location = 5) uniform float in_scale;
layout (location = 6) uniform float in_lerp;

layout(binding = 0) uniform sampler2D baseTex;

#if defined(LERP_ENABLED)
layout(binding = 1) uniform sampler2D oldTex;
#endif

in vec3 v_worldpos;
in vec3 v_normal;
in vec4 v_color;
in vec2 v_texcoord;
in vec4 v_projpos;

layout(location = 0) out vec4 out_Diffuse;

#if defined(GBUFFER_ENABLED)
layout(location = 1) out vec4 out_Lightmap;
layout(location = 2) out vec4 out_WorldNorm;
layout(location = 3) out vec4 out_Specular;
#endif

void main(void)
{
	ClipPlaneTest(v_worldpos.xyz, v_normal.xyz);

	vec4 baseColor = texture(baseTex, v_texcoord);

	baseColor = ProcessDiffuseColor(baseColor);

	#if defined(LERP_ENABLED)
		
		vec4 oldColor = texture(oldTex, v_texcoord);

		oldColor = ProcessDiffuseColor(oldColor);

		baseColor = mix(oldColor, baseColor, in_lerp);

	#endif

	vec4 lightmapColor = v_color;
	
	lightmapColor = ProcessOtherGammaColor(lightmapColor);

	lightmapColor.r = clamp(lightmapColor.r, 0.0, 1.0);
	lightmapColor.g = clamp(lightmapColor.g, 0.0, 1.0);
	lightmapColor.b = clamp(lightmapColor.b, 0.0, 1.0);
	lightmapColor.a = clamp(lightmapColor.a, 0.0, 1.0);

	vec3 vNormal = normalize(v_normal.xyz);

#if defined(GBUFFER_ENABLED)

	vec2 vOctNormal = UnitVectorToOctahedron(vNormal);

	float flDistanceToFragment = distance(v_worldpos.xyz, CameraUBO.viewpos.xyz);

	out_Diffuse = baseColor;
	out_Lightmap = lightmapColor;
	out_WorldNorm = vec4(vOctNormal.x, vOctNormal.y, flDistanceToFragment, 0.0);
	out_Specular = vec4(0.0);

#else

	#if !defined(ADDITIVE_BLEND_ENABLED)
		vec4 finalColor = CalcFog(baseColor * lightmapColor);
	#else
		vec4 finalColor = baseColor * lightmapColor;
	#endif

	#if defined(OIT_BLEND_ENABLED)
		
		GatherFragment(finalColor);

	#endif

	out_Diffuse = finalColor;	

#endif
}