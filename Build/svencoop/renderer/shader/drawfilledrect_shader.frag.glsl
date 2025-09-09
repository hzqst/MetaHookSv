#version 430

#include "common.h"

layout(binding = 0) uniform sampler2D diffuseTex;

in vec4 v_color;

layout(location = 0) out vec4 out_Diffuse;

void main()
{
	out_Diffuse = v_color;
}