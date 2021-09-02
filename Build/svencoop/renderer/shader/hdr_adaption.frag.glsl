#version 430

#include "common.h"

uniform float frametime;

layout(binding=0) uniform sampler2D currentTex;
layout(binding=1) uniform sampler2D adaptedTex;

layout(location=0) out float out_Color;

void main()
{
    float currentLum = texture(currentTex, vec2(0.5f, 0.5f)).x;
    float adaptedLum = texture(adaptedTex, vec2(0.5f, 0.5f)).x;

	out_Color = (adaptedLum + (currentLum - adaptedLum) * ( 1.0 - pow( 0.98, frametime ) ));
}