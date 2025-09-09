#version 430

#include "common.h"

layout(location = FILLEDRECT_VA_POSITION) in vec2 in_pos;
layout(location = FILLEDRECT_VA_COLOR) in vec4 in_color;
layout(location = FILLEDRECT_VA_MATRIX0) in vec4 in_matrix0;
layout(location = FILLEDRECT_VA_MATRIX1) in vec4 in_matrix1;
layout(location = FILLEDRECT_VA_MATRIX2) in vec4 in_matrix2;
layout(location = FILLEDRECT_VA_MATRIX3) in vec4 in_matrix3;

out vec4 v_color;

void main()
{
	v_color = in_color;
	
	mat4 combinedMatrix = mat4(
		in_matrix0,
		in_matrix1,
		in_matrix2,
		in_matrix3
	);

	gl_Position = combinedMatrix * vec4(in_pos.x, in_pos.y, 0.0, 1.0);
}