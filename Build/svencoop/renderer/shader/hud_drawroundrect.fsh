//Fragment Shader by hzqst

#version 120
uniform sampler2D base;
uniform vec2 center;
uniform float radius;
uniform float blurdist = 2.0;
varying vec2 worldpos;

void main()
{
	vec4 vBaseColor = texture2D(base, gl_TexCoord[0].xy) * gl_Color;
	float fDist = length(worldpos-center);
	
	gl_FragColor = vBaseColor;
	if(fDist > radius+blurdist)
	{
		gl_FragColor.a = 0.0;
	}
	else if(fDist > radius)
	{
		gl_FragColor.a *= 1.0 - (fDist - radius) / blurdist;
	}
}