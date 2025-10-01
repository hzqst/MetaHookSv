#version 430

#include "common.h"

layout (location = SPRITE_VA_UP_DOWN_LEFT_RIGHT) uniform vec4 in_up_down_left_right;
layout (location = SPRITE_VA_COLOR) uniform vec4 in_color;
layout (location = SPRITE_VA_ORIGIN) uniform vec3 in_origin;
layout (location = SPRITE_VA_ANGLES) uniform vec3 in_angles;
layout (location = SPRITE_VA_SCALE) uniform float in_scale;
layout (location = SPRITE_VA_LERP) uniform float in_lerp;

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
	
	#if defined(ALPHA_TEST_ENABLED)
        float alpha = baseColor.a;
        if (alpha < 0.001)
            discard;
	#endif

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
	
	float flDistanceToFragment = distance(v_worldpos.xyz, GetCameraViewPos(0));

#if defined(GBUFFER_ENABLED)

	vec2 vOctNormal = UnitVectorToOctahedron(vNormal);

	out_Diffuse = baseColor;
	out_Lightmap = lightmapColor;
	out_WorldNorm = vec4(vOctNormal.x, vOctNormal.y, flDistanceToFragment, 0.0);
	out_Specular = vec4(0.0);

#else

	vec4 finalColor = CalcFog(
		ProcessLinearBlendShift(baseColor * lightmapColor),
		flDistanceToFragment
		);

	#if defined(OIT_BLEND_ENABLED)
		
		GatherFragment(finalColor);

	#endif

	out_Diffuse = finalColor;	

#endif
}