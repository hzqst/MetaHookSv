#version 430 compatibility

#include "common.h"

layout(binding = 0) uniform sampler2D diffuseTex;

in vec3 v_worldpos;
in vec2 v_diffusetexcoord;
in vec4 v_color;

layout(location = 0) out vec4 out_Diffuse;

void main()
{
	ClipPlaneTest(v_worldpos.xyz, -SceneUBO.vpn.xyz);

	vec4 diffuseColor = texture2D(diffuseTex, v_diffusetexcoord.xy);

	diffuseColor = TexGammaToLinear(diffuseColor);

	vec4 lightmapColor = v_color;

	lightmapColor.r = clamp(lightmapColor.r, 0.0, 1.0);
	lightmapColor.g = clamp(lightmapColor.g, 0.0, 1.0);
	lightmapColor.b = clamp(lightmapColor.b, 0.0, 1.0);
	lightmapColor.a = clamp(lightmapColor.a, 0.0, 1.0);

	lightmapColor = GammaToLinear(lightmapColor);

#if defined(OIT_ALPHA_BLEND_ENABLED) || defined(ALPHA_BLEND_ENABLED)

	vec4 finalColor = CalcFog(diffuseColor) * lightmapColor;

#else

	vec4 finalColor = diffuseColor * lightmapColor;

#endif

	#if defined(OIT_ALPHA_BLEND_ENABLED) || defined(OIT_ADDITIVE_BLEND_ENABLED) 
		
		GatherFragment(finalColor);

	#endif

	out_Diffuse = finalColor;
	
}