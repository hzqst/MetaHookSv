#version 430

#include "common.h"

uniform sampler2D baseTex;
uniform sampler2D blurTex;
uniform float blurfactor;
uniform float darkness;
in float lum;
in vec2 texCoord;

layout(location=0) out vec4 out_Color;

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
	
	vec3 L = vColor * lum;

    // exposure
	vColor = L / (1 + L);

    // vignette effect (makes brightness drop off with distance from center)
	vColor = vignette(vColor, texCoord.xy); 

	//gamma correction
	vColor = LinearToGamma3(vColor);

	gl_FragColor = vec4(vColor, 1.0);
}