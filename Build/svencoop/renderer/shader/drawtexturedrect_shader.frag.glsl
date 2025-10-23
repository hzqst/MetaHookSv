#version 430

#include "common.h"

layout(binding = 0) uniform sampler2D diffuseTex;

#if defined(MASK_TEXTURE_ENABLED)
layout(binding = 1) uniform sampler2D maskTex;
#endif

in vec2 v_diffusetexcoord;
in vec4 v_color;

layout(location = 0) out vec4 out_Diffuse;

void main()
{
	vec4 baseColor = texture(diffuseTex, v_diffusetexcoord.xy);

	#if defined(ALPHA_TEST_ENABLED)
        float alpha = baseColor.a;
        if (alpha < 0.001)
            discard;
	#endif

	vec4 finalColor = baseColor * v_color;
	
	#if defined(MASK_TEXTURE_ENABLED)
		vec4 maskColor = texture(maskTex, v_diffusetexcoord.xy);

		finalColor.a *= maskColor.r;
	#endif

	out_Diffuse = finalColor;
}