//Fragment Shader by hzqst

#version 120
uniform sampler2D base;
uniform vec3 src_col;
varying vec2 worldpos;

void main()
{
	vec4 vBaseColor = texture2D(base, gl_TexCoord[0].xy);

	if(src_col.r == vBaseColor.r && src_col.g == vBaseColor.g && src_col.b == vBaseColor.b)
		vBaseColor = gl_Color;

	gl_FragColor = vBaseColor;
}