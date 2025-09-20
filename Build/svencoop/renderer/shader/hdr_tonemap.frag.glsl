#version 430

#include "common.h"

layout(binding=0) uniform sampler2D baseTex;
layout(binding=1) uniform sampler2D blurTex;
layout(binding=2) uniform sampler2D lumTex;

layout(location=0) uniform float blurfactor;
layout(location=1) uniform float darkness;
layout(location=2) uniform float exposure;

in vec2 texCoord;

layout(location=0) out vec4 out_Color;

float SampleLumTexture()
{
	return texture(lumTex, vec2(0.5, 0.5)).x;
}

float CalcLum()
{
	return exposure / max(0.001, SampleLumTexture());
}

vec3 vignette(vec3 c, vec2 win_bias)
{
    // convert window coord to [-1, 1] range
    vec2 wpos = 2.0*(win_bias - vec2(0.5, 0.5)); 

    // calculate distance from origin
    float r = length(wpos);
    r = 1.0 - smoothstep(0.9, 1.9, r);
	return c * r;
}

void main() 
{
	vec3 baseColor = texture(baseTex, texCoord.xy).xyz;
	vec3 blurColor = texture(blurTex, texCoord.xy).xyz * 0.33;
    vec3 vColor = mix(baseColor, blurColor, blurfactor);

	vColor = pow(vColor, vec3(darkness));
	
	vec3 L = vColor * CalcLum();

    // exposure
	vColor = L / (1 + L);

    // vignette effect (makes brightness drop off with distance from center)
	vColor = vignette(vColor, texCoord.xy); 

	//gamma correction
	vColor = LinearToGamma3(vColor);

	out_Color = vec4(vColor, 1.0);
}