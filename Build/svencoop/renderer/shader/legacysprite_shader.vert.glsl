#version 430 compatibility

#include "common.h"

out vec3 v_worldpos;
out vec2 v_diffusetexcoord;
out vec4 v_color;
out vec4 v_projpos;

void main()
{
	vec3 outvert = gl_Vertex.xyz;
	
	v_worldpos = outvert;
	v_diffusetexcoord = gl_MultiTexCoord0.xy;

	v_color = gl_Color;
	gl_Position = CameraUBO.projMatrix * CameraUBO.viewMatrix * vec4(outvert, 1.0);
}