#version 460 compatibility

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

	lightmapColor = GammaToLinear(lightmapColor);

	vec4 finalColor = diffuseColor * lightmapColor;

	finalColor = CalcFog(finalColor);

	#if defined(OIT_ALPHA_BLEND_ENABLED) || defined(OIT_ADDITIVE_BLEND_ENABLED) 
		
		GatherFragment(finalColor);

	#endif

	out_Diffuse = finalColor;
	
}