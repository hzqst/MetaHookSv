#version 460 compatibility

#include "common.h"

layout(binding = 0) uniform sampler2D diffuseTex;

in vec3 v_worldpos;
in vec2 v_diffusetexcoord;
in vec4 v_color;

layout(location = 0) out vec4 out_Diffuse;

void main()
{
	vec4 diffuseColor = texture2D(diffuseTex, v_diffusetexcoord.xy);

	vec4 color = diffuseColor * v_color;

	#if defined(OIT_ALPHA_BLEND_ENABLED) || defined(OIT_ADDITIVE_BLEND_ENABLED) 
		
		beginInvocationInterlockARB();
		GatherFragment(color);
		endInvocationInterlockARB();

	#else

		out_Diffuse = color;

	#endif
}