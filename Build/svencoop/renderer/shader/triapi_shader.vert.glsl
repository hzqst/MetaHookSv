#version 430

#include "common.h"

layout(location = TRIAPI_VA_POSITION) in vec3 in_pos;
layout(location = TRIAPI_VA_TEXCOORD) in vec2 in_texcoord;
layout(location = TRIAPI_VA_COLOR) in vec4 in_color;

#if defined(TRIAPI_HUD_SPACE_ENABLED)
uniform mat4 u_hudMatrix;
#endif

out vec3 v_worldpos;
out vec2 v_diffusetexcoord;
out vec4 v_color;
out vec4 v_projpos;

void main()
{
	vec3 outvert = in_pos;
	
	v_worldpos = outvert;
	v_diffusetexcoord = in_texcoord;
	v_color = in_color;
	
#if defined(TRIAPI_HUD_SPACE_ENABLED)
	gl_Position = u_hudMatrix * vec4(outvert.xy, 0.0, 1.0);
#else
	gl_Position = GetCameraProjMatrix(0) * GetCameraWorldMatrix(0) * vec4(outvert, 1.0);
#endif
	
	v_projpos = gl_Position;
}
