#version 430

#include "common.h"

layout(location = TRIAPI_VA_POSITION) in vec3 in_pos;
layout(location = TRIAPI_VA_TEXCOORD) in vec2 in_texcoord;
layout(location = TRIAPI_VA_COLOR) in vec4 in_color;

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
	
	gl_Position = CameraUBO.projMatrix * CameraUBO.viewMatrix * vec4(outvert, 1.0);
	v_projpos = gl_Position;
}