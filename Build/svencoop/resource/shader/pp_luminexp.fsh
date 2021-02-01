#version 120

uniform sampler2D tex;

void main() 
{
    float sample = 0.0;

	sample += texture2D( tex,  gl_TexCoord[0].xy ).x;
	sample += texture2D( tex,  gl_TexCoord[0].zw ).x;
	sample += texture2D( tex,  gl_TexCoord[1].xy ).x;
	sample += texture2D( tex,  gl_TexCoord[1].zw ).x;

	sample *= 1.0 / 4.0;

	sample = max(exp(sample), 0.1);

	gl_FragColor = vec4(sample, sample, sample, 1.0);
}