#version 430

#include "common.h"

uniform mat4 u_entityMatrix;

layout(location = 0) in vec3 in_vertex;
layout(location = 1) in vec3 in_normal;
layout(location = 4) in vec2 in_diffusetexcoord;

out vec4 v_projpos;
out vec3 v_worldpos;
out vec3 v_normal;
out vec2 v_diffusetexcoord;

void main()
{
	vec3 vertpos = in_vertex;

	v_diffusetexcoord = vec2(in_diffusetexcoord.x, in_diffusetexcoord.y);

	vec4 worldpos4 = u_entityMatrix * vec4(vertpos, 1.0);
    v_worldpos = worldpos4.xyz;

	vec4 normal4 = vec4(in_normal, 0.0);
	v_normal = normalize((u_entityMatrix * normal4).xyz);

	gl_Position = SceneUBO.projMatrix * SceneUBO.viewMatrix * worldpos4;

	v_projpos = gl_Position;
}