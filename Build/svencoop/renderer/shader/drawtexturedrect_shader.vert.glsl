#version 430

#include "common.h"

layout(location = TEXTUREDRECT_VA_POSITION) in vec2 in_pos;
layout(location = TEXTUREDRECT_VA_TEXCOORD) in vec2 in_texcoord;
layout(location = TEXTUREDRECT_VA_COLOR) in vec4 in_color;
layout(location = TEXTUREDRECT_VA_MATRIX0) in vec4 in_matrix0;
layout(location = TEXTUREDRECT_VA_MATRIX1) in vec4 in_matrix1;
layout(location = TEXTUREDRECT_VA_MATRIX2) in vec4 in_matrix2;
layout(location = TEXTUREDRECT_VA_MATRIX3) in vec4 in_matrix3;

out vec2 v_diffusetexcoord;
out vec4 v_color;

void main()
{
	v_diffusetexcoord = in_texcoord;
	v_color = in_color;
	
	mat4 combinedMatrix = mat4(
		in_matrix0,
		in_matrix1,
		in_matrix2,
		in_matrix3
	);

	gl_Position = combinedMatrix * vec4(in_pos.x, in_pos.y, 0.0, 1.0);
}