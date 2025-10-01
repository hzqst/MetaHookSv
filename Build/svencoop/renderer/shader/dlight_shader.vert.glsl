#version 430

#include "common.h"

#ifdef VOLUME_ENABLED

layout(location = 0) in vec3 in_vertex;

uniform mat4 u_modelmatrix;

#endif

out vec3 v_fragpos;
out vec4 v_projpos;
out vec2 v_texcoord;

void main(void)
{
#ifdef VOLUME_ENABLED

	vec4 worldpos4 = u_modelmatrix * vec4(in_vertex, 1.0);

	gl_Position = GetCameraProjMatrix(0) * GetCameraWorldMatrix(0) * worldpos4;

	v_fragpos = worldpos4.xyz;

	v_projpos = gl_Position;

	v_texcoord = vec2(0);

#else
	uint idx = gl_VertexID % 4;

	vec2 vertices[4]= vec2[4](vec2(-1, -1), vec2(-1, 1), vec2(1, 1), vec2(1, -1));
	vec2 texcoords[4]= vec2[4](vec2(0, 0), vec2(0, 1), vec2(1, 1), vec2(1, 0));

	gl_Position = vec4(vertices[idx], 0, 1);

	v_fragpos = GetCameraFrustumPos(0, idx);

	v_projpos = gl_Position;

	v_texcoord = texcoords[idx];
#endif
}