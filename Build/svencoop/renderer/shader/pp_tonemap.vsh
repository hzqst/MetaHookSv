#version 120

uniform sampler2D lumtex;
uniform float exposure;
varying float lum;

void main(void)
{
	lum = exposure / max(0.001, texture2D(lumtex, vec2(0.5, 0.5)).x);

	gl_TexCoord[0] = gl_MultiTexCoord0;
	gl_Position = ftransform();
}